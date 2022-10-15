#include "ifavourites.h"
#include "ui/imainframe.h"
#include "ui/iuserinterface.h"
#include "icommandsystem.h"
#include "ui/igroupdialog.h"

#include "module/StaticModule.h"
#include "FavouritesBrowser.h"
#include "FavouritesBrowserControl.h"

namespace ui
{

class FavouritesUserInterfaceModule :
    public RegisterableModule
{
public:
    const std::string& getName() const override
    {
        static std::string _name("FavouritesUserInterface");
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies
        {
            MODULE_FAVOURITES_MANAGER,
            MODULE_COMMANDSYSTEM,
            MODULE_MAINFRAME,
            MODULE_USERINTERFACE,
        };

        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        GlobalCommandSystem().addCommand("ToggleFavouritesBrowser",
            sigc::mem_fun(this, &FavouritesUserInterfaceModule::togglePage));

        // Subscribe to get notified as soon as Radiant is fully initialised
        GlobalMainFrame().signal_MainFrameConstructed().connect(
            sigc::mem_fun(this, &FavouritesUserInterfaceModule::onMainFrameConstructed)
        );

        GlobalUserInterface().registerControl(std::make_shared<FavouritesBrowserControl>());
    }

    void shutdownModule() override
    {
        GlobalUserInterface().unregisterControl(UserControl::FavouritesBrowser);
    }

private:
    void togglePage(const cmd::ArgumentList& args)
    {
        GlobalGroupDialog().togglePage(UserControl::FavouritesBrowser);
    }

    void onMainFrameConstructed()
    {
        GlobalGroupDialog().addControl(UserControl::FavouritesBrowser);
    }
};

module::StaticModuleRegistration<FavouritesUserInterfaceModule> favouritesUserInterfaceModule;

}
