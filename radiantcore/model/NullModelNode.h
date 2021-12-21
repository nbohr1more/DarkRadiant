#pragma once

#include "Bounded.h"
#include "irenderable.h"

#include "NullModel.h"
#include "render/RenderableBox.h"

namespace model {

class NullModelNode;
typedef std::shared_ptr<NullModelNode> NullModelNodePtr;

class NullModelNode final :
	public scene::Node,
	public SelectionTestable,
	public ModelNode
{
private:
	NullModelPtr _nullModel;
    render::RenderableBox _renderableBox;

public:
	// Default constructor, allocates a new NullModel
	NullModelNode();

	// Alternative constructor, uses the given nullModel
	NullModelNode(const NullModelPtr& nullModel);

	std::string name() const override;
	Type getNodeType() const override;

	// Accessor to the singleton instance
	static NullModelNodePtr InstancePtr();

	const IModel& getIModel() const override;
	IModel& getIModel() override;
	bool hasModifiedScale() override;
	Vector3 getModelScale() override;

	void testSelect(Selector& selector, SelectionTest& test) override;

	void renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const override;
	void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	// Bounded implementation
	const AABB& localAABB() const override;
};

} // namespace model
