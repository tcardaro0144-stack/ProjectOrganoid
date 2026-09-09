#pragma once

#include "Modules/ModuleManager.h"

class FOrganoidAIBridgeServer;

class FOrganoidAIBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FOrganoidAIBridgeServer> Server;
};
