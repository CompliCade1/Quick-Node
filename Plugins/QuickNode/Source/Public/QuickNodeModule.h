#pragma once

#include "Engine.h"
#include "Modules/ModuleInterface.h"
#include "UnrealEd.h"

DECLARE_LOG_CATEGORY_EXTERN(QuickNode, All, All)
class ISettingsSection;
class UQuickNodeProjectSettings;
class QuickNodeOption;

class FQuickNodeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	DECLARE_MULTICAST_DELEGATE(FOnCNStartup);
	static FOnCNStartup OnStartup;

	static void RegisterShortcuts();
	static void CleanShortcuts();
	static void RefreshShortcuts();

	static TSharedPtr<ISettingsSection> GetSettingsPointer();
	static UQuickNodeProjectSettings* GetSettingsPointerCasted();
	static bool GetHasStarted();
	static TArray<QuickNodeOption*> GetManualGroupFromName(FString _Name);

private:
	TSharedPtr<ISettingsSection> ModuleSettings = nullptr;
	static FQuickNodeModule* SelfRef;
	TMap<FString, TArray<QuickNodeOption*>> ManualGroups;
};