#pragma once

#include "ideclmanager.h"

namespace fx
{

class IFxAction
{
public:
    using Ptr = std::shared_ptr<IFxAction>;

    virtual ~IFxAction() {}

    // Returns the action delay in seconds
    virtual float getDelay() = 0;

    // True: Don't shake the entity this effect is attached to
    virtual bool getIgnoreMaster() = 0;
};

class IFxDeclaration :
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<IFxDeclaration>;

    // Returns the number of actions in this FX declaration
    virtual std::size_t getNumActions() = 0;

    // Returns the n-th action (based on the given 0-based index)
    virtual IFxAction::Ptr getAction(std::size_t index) = 0;

    // Returns the name of the joint this FX should bind to
    // Evaluated to an empty string if "bindTo" is not set
    virtual std::string getBindTo() = 0;
};

}