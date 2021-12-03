/**
 * \file
 * These are the renderables that are used in the PatchNode/Patch class to draw
 * the patch onto the screen.
 */
#pragma once

#include <iterator>
#include "igl.h"
#include "imodelsurface.h"

#include "PatchTesselation.h"

#include "render/VertexBuffer.h"
#include "render/IndexedVertexBuffer.h"

#if 0
/// Helper class to render a PatchTesselation in solid mode
class RenderablePatchSolid :
	public OpenGLRenderable
#ifdef RENDERABLE_GEOMETRY
    , public RenderableGeometry
#endif
{
    // Geometry source
	PatchTesselation& _tess;

    // VertexBuffer for rendering
    typedef render::IndexedVertexBuffer<ArbitraryMeshVertex> VertexBuffer_T;
    mutable VertexBuffer_T _vertexBuf;

    mutable bool _needsUpdate;

    // The render indices to render the mesh vertices as QUADS
    std::vector<unsigned int> _indices;

public:
	RenderablePatchSolid(PatchTesselation& tess);

	void render(const RenderInfo& info) const;

    void queueUpdate();

#ifdef RENDERABLE_GEOMETRY
    Type getType() const override;
    const Vector3& getFirstVertex() override;
    std::size_t getVertexStride() override;
    const unsigned int& getFirstIndex() override;
    std::size_t getNumIndices() override;

private:
    void updateIndices();
#endif
};
#endif

// Renders a vertex' normal/tangent/bitangent vector (for debugging purposes)
class RenderablePatchVectorsNTB :
	public OpenGLRenderable
{
private:
    std::vector<VertexCb> _vertices;
	const PatchTesselation& _tess;

	ShaderPtr _shader;

public:
	const ShaderPtr& getShader() const;

	RenderablePatchVectorsNTB(const PatchTesselation& tess);

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void render(const RenderInfo& info) const;

	void render(IRenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
};

class ITesselationIndexer
{
public:
    virtual ~ITesselationIndexer() {}

    virtual render::GeometryType getType() const = 0;

    // The number of indices generated by this indexer for the given tesselation
    virtual std::size_t getNumIndices(const PatchTesselation& tess) const = 0;

    // Generate the indices for the given tesselation, assigning them to the given insert iterator
    virtual void generateIndices(const PatchTesselation& tess, std::back_insert_iterator<std::vector<unsigned int>> outputIt) const = 0;
};

class TesselationIndexer_Triangles :
    public ITesselationIndexer
{
public:
    render::GeometryType getType() const override
    {
        return render::GeometryType::Triangles;
    }

    std::size_t getNumIndices(const PatchTesselation& tess) const override
    {
        return (tess.height - 1) * (tess.width - 1) * 6; // 6 => 2 triangles per quad
    }

    void generateIndices(const PatchTesselation& tess, std::back_insert_iterator<std::vector<unsigned int>> outputIt) const override
    {
        // Generate the indices to define the triangles in clockwise order
        for (std::size_t h = 0; h < tess.height - 1; ++h)
        {
            auto rowOffset = h * tess.width;

            for (std::size_t w = 0; w < tess.width - 1; ++w)
            {
                outputIt = static_cast<unsigned int>(rowOffset + w + tess.width);
                outputIt = static_cast<unsigned int>(rowOffset + w + 1);
                outputIt = static_cast<unsigned int>(rowOffset + w);
                
                outputIt = static_cast<unsigned int>(rowOffset + w + tess.width);
                outputIt = static_cast<unsigned int>(rowOffset + w + tess.width + 1);
                outputIt = static_cast<unsigned int>(rowOffset + w + 1);
            }
        }
    }
};

class TesselationIndexer_Quads :
    public ITesselationIndexer
{
public:
    render::GeometryType getType() const override
    {
        return render::GeometryType::Quads;
    }

    std::size_t getNumIndices(const PatchTesselation& tess) const override
    {
        return (tess.height - 1) * (tess.width - 1) * 4; // 4 indices per quad
    }

    void generateIndices(const PatchTesselation& tess, std::back_insert_iterator<std::vector<unsigned int>> outputIt) const override
    {
        for (std::size_t h = 0; h < tess.height - 1; ++h)
        {
            auto rowOffset = h * tess.width;

            for (std::size_t w = 0; w < tess.width - 1; ++w)
            {
                outputIt = static_cast<unsigned int>(rowOffset + w);
                outputIt = static_cast<unsigned int>(rowOffset + w + tess.width);
                outputIt = static_cast<unsigned int>(rowOffset + w + tess.width + 1);
                outputIt = static_cast<unsigned int>(rowOffset + w + 1);
            }
        }
    }
};

template<typename TesselationIndexerT>
class RenderablePatchTesselation :
    public OpenGLRenderable
{
private:
    static_assert(std::is_base_of_v<ITesselationIndexer, TesselationIndexerT>, "Indexer must implement ITesselationIndexer");
    TesselationIndexerT _indexer;

    const PatchTesselation& _tess;
    bool _needsUpdate;
    ShaderPtr _shader;
    std::size_t _size;

    render::IGeometryRenderer::Slot _surfaceSlot;

public:
    RenderablePatchTesselation(const PatchTesselation& tess) :
        _tess(tess),
        _needsUpdate(true),
        _surfaceSlot(render::IGeometryRenderer::InvalidSlot),
        _size(0)
    {}
    
    void clear()
    {
        if (!_shader || _surfaceSlot == render::IGeometryRenderer::InvalidSlot) return;

        _shader->removeGeometry(_surfaceSlot);
        _shader.reset();

        _surfaceSlot = render::IGeometryRenderer::InvalidSlot;
        _size = 0;
    }

    void queueUpdate()
    {
        _needsUpdate = true;
    }

    void update(const ShaderPtr& shader)
    {
        bool shaderChanged = _shader != shader;

        if (!_needsUpdate && !shaderChanged) return;

        _needsUpdate = false;
        auto sizeChanged = _tess.vertices.size() != _size;

        if (_shader && _surfaceSlot != render::IGeometryRenderer::InvalidSlot && (shaderChanged || sizeChanged))
        {
            clear();
        }

        _shader = shader;
        _size = _tess.vertices.size();

        // Generate the index array
        std::vector<unsigned int> indices;
        indices.reserve(_indexer.getNumIndices(_tess));

        _indexer.generateIndices(_tess, std::back_inserter(indices));

        if (_surfaceSlot == render::IGeometryRenderer::InvalidSlot)
        {
            _surfaceSlot = shader->addGeometry(_indexer.getType(), _tess.vertices, indices);
        }
        else
        {
            shader->updateGeometry(_surfaceSlot, _tess.vertices, indices);
        }
    }

    void render(const RenderInfo& info) const override
    {
        if (_surfaceSlot != render::IGeometryRenderer::InvalidSlot && _shader)
        {
            _shader->renderGeometry(_surfaceSlot);
        }
    }
};
