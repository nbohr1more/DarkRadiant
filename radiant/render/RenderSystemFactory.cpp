#include "RenderSystemFactory.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

#include "OpenGLRenderSystem.h"

namespace render
{

RenderSystemPtr RenderSystemFactory::createRenderSystem()
{
	return OpenGLRenderSystemPtr(new OpenGLRenderSystem);
}

const std::string& RenderSystemFactory::getName() const
{
	static std::string _name(MODULE_RENDERSYSTEMFACTORY);
	return _name;
}

const StringSet& RenderSystemFactory::getDependencies() const
{
	static StringSet _dependencies; // no dependencies
	return _dependencies;
}

void RenderSystemFactory::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
}

// Define the static RenderSystemFactory module
module::StaticModule<RenderSystemFactory> renderSystemFactory;

} // namespace
