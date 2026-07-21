

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "QuickNodeObject.h"
#include "Framework/Commands/Commands.h"

/**
 *
 */
class QuickNodeCommandsImpl : public TCommands<QuickNodeCommandsImpl>
{
public:
	QuickNodeCommandsImpl()
		: TCommands<QuickNodeCommandsImpl>(
			TEXT("Quick Node Commands"),
			NSLOCTEXT("Contexts", "QuickNodeCommands", "Quick Node Commands"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{
	}

	virtual ~QuickNodeCommandsImpl() override
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> CustomSave;

	TArray<TSharedPtr<FUICommandInfo>> CustomCommands;

	TMap<TSharedPtr<FUICommandInfo>, TSubclassOf<UQuickNodeObject>> QuickNodeClasses;
};

class QuickNodeCommands
{
public:
	static void Register();

	static const QuickNodeCommandsImpl& Get();

	static void Unregister();
};
