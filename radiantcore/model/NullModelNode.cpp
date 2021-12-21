#include "NullModelNode.h"

#include "math/Frustum.h"

namespace model {

NullModelNode::NullModelNode() :
	_nullModel(new NullModel),
    _renderableBox(localAABB(), worldAABB().getOrigin())
{}

NullModelNode::NullModelNode(const NullModelPtr& nullModel) :
	_nullModel(nullModel),
    _renderableBox(localAABB(), worldAABB().getOrigin())
{}

std::string NullModelNode::name() const
{
	return "nullmodel";
}

scene::INode::Type NullModelNode::getNodeType() const
{
	return Type::Model;
}

NullModelNodePtr NullModelNode::InstancePtr()
{
	static NullModelNodePtr _nullModelNode;

	if (_nullModelNode == NULL) {
		// Not yet instantiated, create a new NullModel
		_nullModelNode = NullModelNodePtr(new NullModelNode);
	}

	return _nullModelNode;
}

const IModel& NullModelNode::getIModel() const
{
	return *_nullModel;
}

IModel& NullModelNode::getIModel()
{
	return *_nullModel;
}

bool NullModelNode::hasModifiedScale()
{
	return false;
}

Vector3 NullModelNode::getModelScale()
{
	return Vector3(1,1,1);
}

void NullModelNode::testSelect(Selector& selector, SelectionTest& test) {
	_nullModel->testSelect(selector, test, localToWorld());
}

void NullModelNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const {
	_nullModel->renderSolid(collector, volume, localToWorld());
}

void NullModelNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const {
	_nullModel->renderWireframe(collector, volume, localToWorld());
}

void NullModelNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    if (collector.supportsFullMaterials())
    {
        renderSolid(collector, volume);
    }
    else
    {
        renderWireframe(collector, volume);
    }
}

void NullModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_nullModel->setRenderSystem(renderSystem);
}

const AABB& NullModelNode::localAABB() const {
	return _nullModel->localAABB();
}

} // namespace model
