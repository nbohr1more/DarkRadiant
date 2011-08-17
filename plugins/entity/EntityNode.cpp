#include "EntityNode.h"

#include "EntitySettings.h"
#include "target/RenderableTargetInstances.h"

namespace entity {

EntityNode::EntityNode(const IEntityClassPtr& eclass) :
	TargetableNode(_entity, *this),
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity),
	_shaderParms(_keyObservers, _colourKey)
{
	construct();
}

EntityNode::EntityNode(const EntityNode& other) :
	IEntityNode(other),
	SelectableNode(other),
	Namespaced(other),
	TargetableNode(_entity, *this),
	Nameable(other),
	Transformable(other),
	MatrixTransform(other),
	scene::Cloneable(other),
	IEntityClass::Observer(other),
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity),
	_shaderParms(_keyObservers, _colourKey)
{
	construct();
}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	_eclass->addObserver(this);

	TargetableNode::construct();

	addKeyObserver("name", _nameKey);
	addKeyObserver("_color", _colourKey);

	_shaderParms.addKeyObservers();
}

void EntityNode::destruct()
{
	_shaderParms.removeKeyObservers();

	removeKeyObserver("_color", _colourKey);
	removeKeyObserver("name", _nameKey);

	TargetableNode::destruct();

	_eclass->removeObserver(this);
}

void EntityNode::addKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.insert(key, observer);
}

void EntityNode::removeKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.erase(key, observer);
}

Entity& EntityNode::getEntity()
{
	return _entity;
}

float EntityNode::getShaderParm(int parmNum) const
{
	return _shaderParms.getParmValue(parmNum);
}

std::string EntityNode::getName() const {
	return _namespaceManager.getName();
}

void EntityNode::setNamespace(INamespace* space) {
	_namespaceManager.setNamespace(space);
}

INamespace* EntityNode::getNamespace() const {
	return _namespaceManager.getNamespace();
}

void EntityNode::connectNameObservers() {
	_namespaceManager.connectNameObservers();
}

void EntityNode::disconnectNameObservers() {
	_namespaceManager.disconnectNameObservers();
}

void EntityNode::attachNames() {
	_namespaceManager.attachNames();
}

void EntityNode::detachNames() {
	_namespaceManager.detachNames();
}

void EntityNode::changeName(const std::string& newName) {
	_namespaceManager.changeName(newName);
}

void EntityNode::onInsertIntoScene()
{
	_entity.instanceAttach(scene::findMapFile(getSelf()));

	// Register our TargetableNode, now that we're in the scene
	RenderableTargetInstances::Instance().attach(*this);

	SelectableNode::onInsertIntoScene();
}

void EntityNode::onRemoveFromScene()
{
	SelectableNode::onRemoveFromScene();

	RenderableTargetInstances::Instance().detach(*this);
	_entity.instanceDetach(scene::findMapFile(getSelf()));
}

void EntityNode::onChildAdded(const scene::INodePtr& child)
{
	Node::onChildAdded(child);

	child->setRenderEntity(boost::dynamic_pointer_cast<IRenderEntity>(getSelf()));
}

void EntityNode::onChildRemoved(const scene::INodePtr& child)
{
	Node::onChildRemoved(child);

	child->setRenderEntity(IRenderEntityPtr());
}

std::string EntityNode::name() const
{
	return _nameKey.name();
}

void EntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Nothing so far (FIXME)
}

void EntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Submit renderable text name if required
	if (collector.getStyle() == RenderableCollector::eWireframeOnly &&
		EntitySettings::InstancePtr()->renderEntityNames())
    {
		collector.SetState(_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
		collector.addRenderable(_renderableName, localToWorld());
	}
}

bool EntityNode::isHighlighted() const
{
	return isSelected();
}

void EntityNode::onEClassReload()
{
	// Let the keyobservers reload their values
	_keyObservers.refreshObservers();
}

const Vector3& EntityNode::getColour() const
{
	return _colourKey.getColour();
}

const ShaderPtr& EntityNode::getColourShader() const
{
	return _colourKey.getWireShader();
}

} // namespace entity
