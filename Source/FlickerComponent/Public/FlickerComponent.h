
#pragma once

#include "Modules/ModuleManager.h"

class FFlickerComponentModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};