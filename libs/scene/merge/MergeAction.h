#pragma once

#include <memory>
#include "ientity.h"
#include "imapmerge.h"
#include "../Clone.h"
#include "../scenelib.h"

namespace scene
{ 

namespace merge
{

// Represents a single step of a merge process, like adding a brush,
// removing an entity, setting a keyvalue, etc.
class MergeAction : 
    public virtual IMergeAction
{
private:
    ActionType _type;

    bool _isActive;

protected:
    MergeAction(ActionType type) :
        _type(type),
        _isActive(true)
    {}

public:
    using Ptr = std::shared_ptr<MergeAction>;

    ActionType getType() const override
    {
        return _type;
    }

    virtual void activate() override
    {
        _isActive = true;
    }

    virtual void deactivate() override
    {
        _isActive = false;
    }

    virtual bool isActive() const override
    {
        return _isActive;
    }
};

// Various implementations of the above MergeAction base type following

class RemoveNodeFromParentAction :
    public MergeAction
{
private:
    INodePtr _nodeToRemove;

protected:
    RemoveNodeFromParentAction(const INodePtr& nodeToRemove, ActionType type) :
        MergeAction(type),
        _nodeToRemove(nodeToRemove)
    {
        assert(_nodeToRemove);
    }

public:
    const INodePtr& getNodeToRemove() const
    {
        return _nodeToRemove;
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        removeNodeFromParent(_nodeToRemove);
    }

    INodePtr getAffectedNode() override
    {
        return getNodeToRemove();
    }
};

class RemoveChildAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveChildAction(const INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveChildNode)
    {}
};

class RemoveEntityAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveEntityAction(const INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveEntity)
    {}
};

class AddCloneToParentAction :
    public MergeAction
{
private:
    INodePtr _node;
    INodePtr _parent;
    INodePtr _cloneToBeInserted;

protected:
    // Will add the given node to the parent when applyChanges() is called
    AddCloneToParentAction(const INodePtr& node, const INodePtr& parent, ActionType type) :
        MergeAction(type),
        _node(node),
        _parent(parent)
    {
        assert(_node);
        assert(Node_getCloneable(node));

        // No post-clone callback since we don't care about selection groups right now
        _cloneToBeInserted = cloneNodeIncludingDescendants(_node, PostCloneCallback());

        if (!_cloneToBeInserted)
        {
            throw std::runtime_error("Node " + _node->name() + " is not cloneable");
        }

        // Reset all layers of the clone to the active one
        auto activeLayer = parent->getRootNode()->getLayerManager().getActiveLayer();

        _cloneToBeInserted->moveToLayer(activeLayer);
        _cloneToBeInserted->foreachNode([=](const INodePtr& child) 
        { 
            child->moveToLayer(activeLayer); return true; 
        });
    }

public:
    void applyChanges() override
    {
        if (!isActive()) return;

        addNodeToContainer(_cloneToBeInserted, _parent);
    }

    const INodePtr& getParent() const
    {
        return _parent;
    }

    const INodePtr& getSourceNodeToAdd() const
    {
        return _node;
    }

    INodePtr getAffectedNode() override
    {
        return _cloneToBeInserted;
    }
};

class AddEntityAction :
    public AddCloneToParentAction
{
public:
    AddEntityAction(const INodePtr& node, const IMapRootNodePtr& targetRoot) :
        AddCloneToParentAction(node, targetRoot, ActionType::AddEntity)
    {}
};

class AddChildAction :
    public AddCloneToParentAction
{
public:
    AddChildAction(const INodePtr& node, const INodePtr& parent) :
        AddCloneToParentAction(node, parent, ActionType::AddChildNode)
    {}
};

class SetEntityKeyValueAction :
    public MergeAction,
    public virtual IEntityKeyValueMergeAction
{
private:
    INodePtr _node;
    std::string _key;
    std::string _value;

public:
    // Will call setKeyValue(key, value) on the targetnode when applyChanges() is called
    SetEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value, ActionType mergeActionType) :
        MergeAction(mergeActionType),
        _node(node),
        _key(key),
        _value(value)
    {
        assert(_node);
        assert(Node_isEntity(_node));
        assert(!_key.empty());
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        // No post-clone callback since we don't care about selection groups right now
        auto entity = Node_getEntity(_node);

        if (!entity)
        {
            throw std::runtime_error("Node " + _node->name() + " is not an entity");
        }

        entity->setKeyValue(_key, _value);
    }

    const INodePtr& getEntityNode() const
    {
        return _node;
    }

    const std::string& getKey() const override
    {
        return _key;
    }

    const std::string& getValue() const override
    {
        return _value;
    }

    INodePtr getAffectedNode() override
    {
        return getEntityNode();
    }
};

class AddEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    AddEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::AddKeyValue)
    {}
};

class RemoveEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    RemoveEntityKeyValueAction(const INodePtr& node, const std::string& key) :
        SetEntityKeyValueAction(node, key, std::string(), ActionType::RemoveKeyValue)
    {}
};

class ChangeEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    ChangeEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::ChangeKeyValue)
    {}
};

/**
 * A ConflictResolutionAction encapsulates a source change that has a
 * encountered a conflicting target change.
 * 
 * The source change will only be applied if this action has been
 * told to do so by calling setResolvedByUsingSource(true),
 * otherwise nothing happens and the target change stays in effect.
 */
class ConflictResolutionAction :
    public MergeAction
{
protected:
    INodePtr _conflictingEntity;

    // The action the source diff is trying to apply
    MergeAction::Ptr _sourceAction;
    // The action that happened in the target
    MergeAction::Ptr _targetAction;

    bool _applySourceChange;

protected:
    ConflictResolutionAction(ActionType actionType, const INodePtr& conflictingEntity, const MergeAction::Ptr& sourceAction) :
        ConflictResolutionAction(actionType, conflictingEntity, sourceAction, MergeAction::Ptr())
    {}

    ConflictResolutionAction(ActionType actionType, const INodePtr& conflictingEntity, const MergeAction::Ptr& sourceAction, const MergeAction::Ptr& targetAction) :
        MergeAction(actionType),
        _conflictingEntity(conflictingEntity),
        _sourceAction(sourceAction),
        _targetAction(targetAction),
        _applySourceChange(false)
    {}

public:
    using Ptr = std::shared_ptr<ConflictResolutionAction>;

    // The action the source diff is trying to apply
    const MergeAction::Ptr& getSourceAction() const
    {
        return _sourceAction;
    }

    // The action that happened in the target (can be empty)
    const MergeAction::Ptr& getTargetAction() const
    {
        return _targetAction;
    }

    const INodePtr& getConflictingEntity() const
    {
        return _conflictingEntity;
    }

    INodePtr getAffectedNode() override
    {
        return getConflictingEntity();
    }

    bool isResolvedByUsingSource() const
    {
        return _applySourceChange;
    }

    void setResolvedByUsingSource(bool applySourceChange)
    {
        _applySourceChange = applySourceChange;
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        if (_applySourceChange)
        {
            _sourceAction->applyChanges();
        }
    }
};

// An entity node is a conflicting subject in both maps
class EntityConflictResolutionAction :
    public ConflictResolutionAction
{
public:
    EntityConflictResolutionAction(const INodePtr& conflictingEntity, const MergeAction::Ptr& sourceAction) :
        EntityConflictResolutionAction(conflictingEntity, sourceAction, MergeAction::Ptr())
    {}

    EntityConflictResolutionAction(const INodePtr& conflictingEntity, 
                                   const MergeAction::Ptr& sourceAction, 
                                   const MergeAction::Ptr& targetAction) :
        ConflictResolutionAction(ActionType::EntityNodeConflict, conflictingEntity, sourceAction, targetAction)
    {}
};

// An entity key value is a conflicting subject in both maps
class EntityKeyValueConflictResolutionAction :
    public ConflictResolutionAction
{
public:
    EntityKeyValueConflictResolutionAction(const INodePtr& conflictingEntity, 
                                           const MergeAction::Ptr& sourceAction, 
                                           const MergeAction::Ptr& targetAction) :
        ConflictResolutionAction(ActionType::EntityKeyValueConflict, conflictingEntity, sourceAction, targetAction)
    {}
};

}

}