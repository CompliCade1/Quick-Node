#include "QuickNodeCommands.h"
#include "FileHelpers.h"

#define LOCTEXT_NAMESPACE "QuickNodeCommands"

void QuickNodeCommandsImpl::RegisterCommands()
{
	UQuickNodeObject* NewShortcutObject = NewObject<UQuickNodeObject>();

	CustomCommands.Add(nullptr);

	FUICommandInfo::MakeCommandInfo(
		this->AsShared(),
		CustomCommands.Last(),
		FName(NewShortcutObject->Name),
		FText::FromString(NewShortcutObject->Name),
		FText::FromString(NewShortcutObject->Description),
		FSlateIcon(),
		EUserInterfaceActionType::Button,
		NewShortcutObject->DefaultInputChord
	);

	QuickNodeClasses.Add(CustomCommands.Last(), UQuickNodeObject::StaticClass());
}

void QuickNodeCommands::Register()
{
	QuickNodeCommandsImpl::Register();
	UE_LOG(LogTemp, Log, TEXT("Registered Quick Node Commands"));
}

const QuickNodeCommandsImpl& QuickNodeCommands::Get()
{
	return QuickNodeCommandsImpl::Get();
}

void QuickNodeCommands::Unregister()
{
	QuickNodeCommandsImpl::Unregister();
	UE_LOG(LogTemp, Log, TEXT("Unregistered Quick Node Commands"));
}

#undef LOCTEXT_NAMESPACE
