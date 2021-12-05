#include "LightNode.h"

#include "itextstream.h"
#include "icolourscheme.h"
#include "../EntitySettings.h"
#include <functional>

#include "registry/CachedKey.h"

namespace entity {

// --------- LightNode implementation ------------------------------------

LightNode::LightNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	_light(_spawnArgs,
		   *this,
		   Callback(std::bind(&scene::Node::transformChanged, this)),
		   Callback(std::bind(&scene::Node::boundsChanged, this)),
		   Callback(std::bind(&LightNode::onLightRadiusChanged, this))),
	_lightCenterInstance(_light.getDoom3Radius().m_centerTransformed, std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightTargetInstance(_light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightRightInstance(_light.rightTransformed(), _light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightUpInstance(_light.upTransformed(), _light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightStartInstance(_light.startTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightEndInstance(_light.endTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_dragPlanes(std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
    _renderableOctagon(*this),
    _renderableLightVolume(*this),
    _showLightVolumeWhenUnselected(EntitySettings::InstancePtr()->getShowAllLightRadii()),
    _overrideColKey(colours::RKEY_OVERRIDE_LIGHTCOL)
{}

LightNode::LightNode(const LightNode& other) :
	EntityNode(other),
	ILightNode(other),
	_light(other._light,
		   *this,
           _spawnArgs,
           Callback(std::bind(&Node::transformChanged, this)),
		   Callback(std::bind(&Node::boundsChanged, this)),
		   Callback(std::bind(&LightNode::onLightRadiusChanged, this))),
	_lightCenterInstance(_light.getDoom3Radius().m_centerTransformed, std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightTargetInstance(_light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightRightInstance(_light.rightTransformed(), _light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightUpInstance(_light.upTransformed(), _light.targetTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightStartInstance(_light.startTransformed(), std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
	_lightEndInstance(_light.endTransformed(), std::bind(&LightNode::selectedChangedComponent, this,std::placeholders:: _1)),
	_dragPlanes(std::bind(&LightNode::selectedChangedComponent, this, std::placeholders::_1)),
    _renderableOctagon(*this),
    _renderableLightVolume(*this),
    _showLightVolumeWhenUnselected(other._showLightVolumeWhenUnselected),
    _overrideColKey(colours::RKEY_OVERRIDE_LIGHTCOL)
{}

LightNodePtr LightNode::Create(const IEntityClassPtr& eclass)
{
	LightNodePtr instance(new LightNode(eclass));
	instance->construct();

	return instance;
}

void LightNode::construct()
{
	EntityNode::construct();

	_light.construct();
}

bool LightNode::isProjected() const
{
    return _light.isProjected();
}

const Frustum& LightNode::getLightFrustum() const
{
    if (!_light.isProjected()) throw std::logic_error("getLightFrustum can be called on projected lights only");

    return _light._frustum;
}

const Vector3& LightNode::getLightStart() const
{
    if (!_light.isProjected()) throw std::logic_error("getLightStart can be called on projected lights only");

    return _light._lightStartTransformed;
}

const Vector3& LightNode::getLightRadius() const
{
    if (_light.isProjected()) throw std::logic_error("getLightRadius can be called on point lights only");

    return _light.m_doom3Radius.m_radiusTransformed;
}

// Snappable implementation
void LightNode::snapto(float snap) {
	_light.snapto(snap);
}

AABB LightNode::getSelectAABB() const
{
    // Use the light origin as select AAB centerpoint
    Vector3 extents;
	default_extents(extents);

	return AABB(_light.getLightOrigin(), extents);
}

void LightNode::onLightRadiusChanged()
{
    // Light radius changed, mark bounds as dirty
    boundsChanged();
}

const AABB& LightNode::localAABB() const {
	return _light.localAABB();
}

float LightNode::getShaderParm(int parmNum) const
{
	return EntityNode::getShaderParm(parmNum);
}

void LightNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// Call the base class first
	EntityNode::onRemoveFromScene(root);

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
	setSelectedComponents(false, selection::ComponentSelectionMode::Face);

    _renderableOctagon.clear();
    _renderableLightVolume.clear();
}

void LightNode::testSelect(Selector& selector, SelectionTest& test)
{
    // Generic entity selection
    EntityNode::testSelect(selector, test);

    // Light specific selection
    test.BeginMesh(localToWorld());
    SelectionIntersection best;
    aabb_testselect(_light._lightBox, test, best);
    if (best.isValid())
    {
        selector.addIntersection(best);
    }
}

// greebo: Returns true if drag planes or one or more light vertices are selected
bool LightNode::isSelectedComponents() const {
	return (_dragPlanes.isSelected() || _lightCenterInstance.isSelected() ||
			_lightTargetInstance.isSelected() || _lightRightInstance.isSelected() ||
			_lightUpInstance.isSelected() || _lightStartInstance.isSelected() ||
			_lightEndInstance.isSelected() );
}

// greebo: Selects/deselects all components, depending on the chosen componentmode
void LightNode::setSelectedComponents(bool select, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Face) {
		_dragPlanes.setSelected(false);
	}

	if (mode == selection::ComponentSelectionMode::Vertex) {
		_lightCenterInstance.setSelected(false);
		_lightTargetInstance.setSelected(false);
		_lightRightInstance.setSelected(false);
		_lightUpInstance.setSelected(false);
		_lightStartInstance.setSelected(false);
		_lightEndInstance.setSelected(false);
	}
}

void LightNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		_lightCenterInstance.invertSelected();
		_lightTargetInstance.invertSelected();
		_lightRightInstance.invertSelected();
		_lightUpInstance.invertSelected();
		_lightStartInstance.invertSelected();
		_lightEndInstance.invertSelected();
	}
}

void LightNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
    {
        // Use the full rotation matrix for the test
        test.BeginMesh(localToWorld());

		if (_light.isProjected())
        {
			// Test the projection components for selection
			_lightTargetInstance.testSelect(selector, test);
			_lightRightInstance.testSelect(selector, test);
			_lightUpInstance.testSelect(selector, test);
			_lightStartInstance.testSelect(selector, test);
			_lightEndInstance.testSelect(selector, test);
		}
		else
        {
			// Test if the light center is hit by the click
			_lightCenterInstance.testSelect(selector, test);
		}
	}
}

const AABB& LightNode::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	if (_light.isProjected()) {
		// Include the according vertices in the AABB
		m_aabb_component.includePoint(_lightTargetInstance.getVertex());
		m_aabb_component.includePoint(_lightRightInstance.getVertex());
		m_aabb_component.includePoint(_lightUpInstance.getVertex());
		m_aabb_component.includePoint(_lightStartInstance.getVertex());
		m_aabb_component.includePoint(_lightEndInstance.getVertex());
	}
	else {
		// Just include the light center, this is the only vertex that may be out of the light volume
		m_aabb_component.includePoint(_lightCenterInstance.getVertex());
	}

	return m_aabb_component;
}

void LightNode::snapComponents(float snap) {
	if (_light.isProjected()) {
		// Check, if any components are selected and snap the selected ones to the grid
		if (isSelectedComponents()) {
			if (_lightTargetInstance.isSelected()) {
				_light.targetTransformed().snap(snap);
			}
			if (_lightRightInstance.isSelected()) {
				_light.rightTransformed().snap(snap);
			}
			if (_lightUpInstance.isSelected()) {
				_light.upTransformed().snap(snap);
			}

			if (_light.useStartEnd()) {
				if (_lightEndInstance.isSelected()) {
					_light.endTransformed().snap(snap);
				}

				if (_lightStartInstance.isSelected()) {
					_light.startTransformed().snap(snap);
				}
			}
		}
		else {
			// None are selected, snap them all
			_light.targetTransformed().snap(snap);
			_light.rightTransformed().snap(snap);
			_light.upTransformed().snap(snap);

			if (_light.useStartEnd()) {
				_light.endTransformed().snap(snap);
				_light.startTransformed().snap(snap);
			}
		}
	}
	else {
		// There is only one vertex for point lights, namely the light_center, always snap it
		_light.getDoom3Radius().m_centerTransformed.snap(snap);
	}

	_light.freezeTransform();
}

void LightNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());
	// greebo: Make sure to use the local lightAABB() for the selection test, excluding the light center
	AABB localLightAABB(Vector3(0,0,0), _light.getDoom3Radius().m_radiusTransformed);
	_dragPlanes.selectPlanes(localLightAABB, selector, test, selectedPlaneCallback);
}

void LightNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	AABB localLightAABB(Vector3(0,0,0), _light.getDoom3Radius().m_radiusTransformed);
	_dragPlanes.selectReversedPlanes(localLightAABB, selector, selectedPlanes);
}

scene::INodePtr LightNode::clone() const
{
	LightNodePtr node(new LightNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

void LightNode::selectedChangedComponent(const ISelectable& selectable) {
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

void LightNode::onPreRender(const VolumeTest& volume)
{
    // Pick the colour shader according to our settings
    const auto& colourShader = _overrideColKey.get() ? getColourShader() : _colourKey.getColourShader();
    _renderableOctagon.update(colourShader);

    // Depending on the selected status or the entity settings, we need to update the wireframe volume
    if (_showLightVolumeWhenUnselected || isSelected())
    {
        if (isProjected())
        {
            _light.updateProjection();
        }

        _renderableLightVolume.update(colourShader);
    }
    else
    {
        // Light volume is not visible, hide it
        _renderableLightVolume.clear();
    }
}

void LightNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
    // Submit self to the renderer as an actual light source
    collector.addLight(_light);

    EntityNode::renderSolid(collector, volume);

    // Render the visible representation of the light entity (origin, bounds etc)
    const bool lightIsSelected = isSelected();
    renderInactiveComponents(collector, volume, lightIsSelected);
}

void LightNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
    EntityNode::renderWireframe(collector, volume);

    const bool lightIsSelected = isSelected();
    renderInactiveComponents(collector, volume, lightIsSelected);
}

void LightNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addHighlightRenderable(_renderableOctagon, Matrix4::getIdentity());
    collector.addHighlightRenderable(_renderableLightVolume, Matrix4::getIdentity());

    EntityNode::renderHighlights(collector, volume);
}

void LightNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

    // Clear the geometry from any previous shader
    _renderableOctagon.clear();
    _renderableLightVolume.clear();

	// The renderable vertices are maintaining shader objects, acquire/free them now
	_light.setRenderSystem(renderSystem);

	_lightCenterInstance.setRenderSystem(renderSystem);
	_lightTargetInstance.setRenderSystem(renderSystem);
	_lightRightInstance.setRenderSystem(renderSystem);
	_lightUpInstance.setRenderSystem(renderSystem);
	_lightStartInstance.setRenderSystem(renderSystem);
	_lightEndInstance.setRenderSystem(renderSystem);
}

// Renders the components of this light instance
void LightNode::renderComponents(IRenderableCollector& collector, const VolumeTest& volume) const
{
	// Render the components (light center) as selected/deselected, if we are in the according mode
	if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
	{
		if (_light.isProjected())
		{
			// A projected light

			EntitySettings& settings = *EntitySettings::InstancePtr();

			const Vector3& colourStartEndSelected = settings.getLightVertexColour(LightEditVertexType::StartEndSelected);
			const Vector3& colourStartEndDeselected = settings.getLightVertexColour(LightEditVertexType::StartEndDeselected);
			const Vector3& colourVertexSelected = settings.getLightVertexColour(LightEditVertexType::Selected);
			const Vector3& colourVertexDeselected = settings.getLightVertexColour(LightEditVertexType::Deselected);

			// Update the colour of the light center dot
			const_cast<Light&>(_light).colourLightTarget() = (_lightTargetInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightRight() = (_lightRightInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;
			const_cast<Light&>(_light).colourLightUp() = (_lightUpInstance.isSelected()) ? colourVertexSelected : colourVertexDeselected;

			const_cast<Light&>(_light).colourLightStart() = (_lightStartInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;
			const_cast<Light&>(_light).colourLightEnd() = (_lightEndInstance.isSelected()) ? colourStartEndSelected : colourStartEndDeselected;

			// Render the projection points
			_light.renderProjectionPoints(collector, volume, localToWorld());
		}
		else
		{
			// A point light

			// Update the colour of the light center dot
			if (_lightCenterInstance.isSelected())
			{
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(
					EntitySettings::InstancePtr()->getLightVertexColour(LightEditVertexType::Selected));
				_light.renderLightCentre(collector, volume, localToWorld());
			}
			else
			{
				const_cast<Light&>(_light).getDoom3Radius().setCenterColour(
					EntitySettings::InstancePtr()->getLightVertexColour(LightEditVertexType::Deselected));
				_light.renderLightCentre(collector, volume, localToWorld());
			}
		}
	}
}

void LightNode::renderInactiveComponents(IRenderableCollector& collector, const VolumeTest& volume, const bool selected) const
{
	// greebo: We are not in component selection mode (and the light is still selected),
	// check if we should draw the center of the light anyway
	if (selected
		&& GlobalSelectionSystem().ComponentMode() != selection::ComponentSelectionMode::Vertex
		&& EntitySettings::InstancePtr()->getAlwaysShowLightVertices())
	{
		if (_light.isProjected())
		{
			EntitySettings& settings = *EntitySettings::InstancePtr();
			const Vector3& colourStartEndInactive = settings.getLightVertexColour(LightEditVertexType::StartEndDeselected);
			const Vector3& colourVertexInactive = settings.getLightVertexColour(LightEditVertexType::Deselected);

			const_cast<Light&>(_light).colourLightStart() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightEnd() = colourStartEndInactive;
			const_cast<Light&>(_light).colourLightTarget() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightRight() = colourVertexInactive;
			const_cast<Light&>(_light).colourLightUp() = colourVertexInactive;

			// Render the projection points
			_light.renderProjectionPoints(collector, volume, localToWorld());
		}
		else
		{
			const Vector3& colourVertexInactive = EntitySettings::InstancePtr()->getLightVertexColour(LightEditVertexType::Inactive);

			const_cast<Light&>(_light).getDoom3Radius().setCenterColour(colourVertexInactive);
			_light.renderLightCentre(collector, volume, localToWorld());
		}
	}
}

void LightNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
    {
		_light.translate(getTranslation());
		_light.rotate(getRotation());
	}
	else
    {
		// Check if the light center is selected, if yes, transform it, if not, it's a drag plane operation
		if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
        {
			// When the user is mouse-moving a vertex in the orthoviews he/she is operating
            // in world space. It's expected that the selected vertex follows the mouse.
            // Since the editable light vertices are measured in local coordinates
            // we have to calculate the new position in world space first and then transform
            // the point back into local space.

            if (_lightCenterInstance.isSelected())
            {
                // Retrieve the translation and apply it to the temporary light center variable
                Vector3 newWorldPos = localToWorld().transformPoint(_light.getDoom3Radius().m_center) + getTranslation();
                _light.getDoom3Radius().m_centerTransformed = localToWorld().getFullInverse().transformPoint(newWorldPos);
            }

			if (_lightTargetInstance.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_light.target()) + getTranslation();
                _light.targetTransformed() = localToWorld().getFullInverse().transformPoint(newWorldPos);
			}

            if (_lightStartInstance.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_light.start()) + getTranslation();
                Vector3 newLightStart = localToWorld().getFullInverse().transformPoint(newWorldPos);

                // Assign the light start, perform the boundary checks
                _light.setLightStart(newLightStart);
            }

            if (_lightEndInstance.isSelected())
            {
                Vector3 newWorldPos = localToWorld().transformPoint(_light.end()) + getTranslation();
                _light.endTransformed() = localToWorld().getFullInverse().transformPoint(newWorldPos);

                _light.ensureLightStartConstraints();
            }

            // Even more footwork needs to be done for light_up and light_right since these
            // are measured relatively to the light_target position.

            // Extend the regular local2World by the additional light_target transform
            Matrix4 local2World = localToWorld();
            local2World.translateBy(_light._lightTarget);
            Matrix4 world2Local = local2World.getFullInverse();

			if (_lightRightInstance.isSelected())
            {
                Vector3 newWorldPos = local2World.transformPoint(_light.right()) + getTranslation();
                _light.rightTransformed() = world2Local.transformPoint(newWorldPos);
			}

			if (_lightUpInstance.isSelected())
            {
                Vector3 newWorldPos = local2World.transformPoint(_light.up()) + getTranslation();
                _light.upTransformed() = world2Local.transformPoint(newWorldPos);
			}

			// If this is a projected light, then it is likely for the according vertices to have changed, so update the projection
			if (_light.isProjected())
            {
				// Call projection changed, so that the recalculation can be triggered (call for projection() would be ignored otherwise)
				_light.projectionChanged();

				// Recalculate the frustum
                _light.updateProjection();
			}
		}
		else
        {
			// Ordinary Drag manipulator
			// greebo: To evaluate the drag operation use a fresh AABB as starting point.
			// We don't use the aabb() or localABB() methods, those return the bounding box 
            // including the light center, which may be positioned way out of the volume
            _dragPlanes.m_bounds = AABB(_light._originTransformed, _light.m_doom3Radius.m_radiusTransformed);

			_light.setLightRadius(_dragPlanes.evaluateResize(getTranslation(), rotation()));
		}
	}
}

const Matrix4& LightNode::rotation() const {
	return _light.rotation();
}

void LightNode::_onTransformationChanged()
{
	_light.revertTransform();
	evaluateTransform();
	_light.updateOrigin();

    _renderableOctagon.queueUpdate();
    _renderableLightVolume.queueUpdate();
}

void LightNode::_applyTransformation()
{
	_light.revertTransform();
	evaluateTransform();
	_light.freezeTransform();
}

const Vector3& LightNode::getUntransformedOrigin()
{
    return _light.getUntransformedOrigin();
}

void LightNode::onVisibilityChanged(bool isVisibleNow)
{
    EntityNode::onVisibilityChanged(isVisibleNow);

    if (isVisibleNow)
    {
        _renderableOctagon.queueUpdate();
        _renderableLightVolume.queueUpdate();
    }
    else
    {
        _renderableLightVolume.clear();
        _renderableOctagon.clear();
    }
}

void LightNode::onEntitySettingsChanged()
{
    _showLightVolumeWhenUnselected = EntitySettings::InstancePtr()->getShowAllLightRadii();
    _renderableLightVolume.queueUpdate();
}

} // namespace entity
