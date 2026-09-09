#include "OrganoidAIBridgeModule.h"
#include "OrganoidAIBridgeServer.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FOrganoidAIBridgeModule"

void FOrganoidAIBridgeModule::StartupModule()
{
	Server = MakeShared<FOrganoidAIBridgeServer>();
	if (!Server->Start())
	{
		UE_LOG(LogTemp, Error, TEXT("[OrganoidAIBridge] Server failed to start. Check that port 8732 is free."));
		Server.Reset();
	}
}

void FOrganoidAIBridgeModule::ShutdownModule()
{
	if (Server.IsValid())
	{
		Server->Stop();
		Server.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOrganoidAIBridgeModule, OrganoidAIBridge)
