#include "ModelNodeBase.h"

namespace model
{

ModelNodeBase::ModelNodeBase() :
    _attachedToShaders(false)
{}

void ModelNodeBase::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    auto identity = Matrix4::getIdentity();

    for (const auto& surface : _renderableSurfaces)
    {
        collector.addHighlightRenderable(*surface, identity);
    }
}

void ModelNodeBase::destroyRenderableSurfaces()
{
    detachFromShaders();

    _renderableSurfaces.clear();
}

void ModelNodeBase::detachFromShaders()
{
    // Detach any existing surfaces. In case we need them again,
    // the node will re-attach in the next pre-render phase
    for (auto& surface : _renderableSurfaces)
    {
        surface->detach();
    }

    _attachedToShaders = false;
}

void ModelNodeBase::attachToShaders()
{
    // Refuse to attach without a render entity
    if (_attachedToShaders || !_renderEntity) return;

    auto renderSystem = _renderSystem.lock();

    if (!renderSystem) return;

    for (auto& surface : _renderableSurfaces)
    {
        auto shader = renderSystem->capture(surface->getSurface().getActiveMaterial());

        // Skip filtered materials
        //TODO if (!shader->isVisible()) continue;

        // Solid mode
        surface->attachToShader(shader);

        // For orthoview rendering we need the entity's wireframe shader
        surface->attachToShader(_renderEntity->getWireShader());

        // Attach to the render entity for lighting mode rendering
        surface->attachToEntity(_renderEntity, shader);
    }

    _attachedToShaders = true;
}

void ModelNodeBase::queueRenderableUpdate()
{
    for (auto& surface : _renderableSurfaces)
    {
        surface->queueUpdate();
    }
}

}
