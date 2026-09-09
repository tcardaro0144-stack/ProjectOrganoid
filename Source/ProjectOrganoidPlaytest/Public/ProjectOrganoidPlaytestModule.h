#pragma once

#include "Modules/ModuleManager.h"

class FProjectOrganoidPlaytestModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
