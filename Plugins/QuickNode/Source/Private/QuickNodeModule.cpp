#include "QuickNodeModule.h"
#include "QuickNodeCommands.h"
#include "QuickNodeInputProcessor.h"
#include "QuickNodeProjectSettings.h"
#include "HelperFunctions.h"

#include <imgui.h>
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ToolMenus.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetCompilerModule.h"

IMPLEMENT_MODULE(FQuickNodeModule, QuickNode);

DEFINE_LOG_CATEGORY(QuickNode)

FQuickNodeModule* FQuickNodeModule::SelfRef = nullptr;
DECLARE_MULTICAST_DELEGATE(FOnCNStartup);
FOnCNStartup FQuickNodeModule::OnStartup;

#define LOCTEXT_NAMESPACE "QuickNode"

void FQuickNodeModule::StartupModule()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		ModuleSettings = SettingsModule->RegisterSettings("Project", "Plugins", "QuickNodeSettings",
			LOCTEXT("QuickNodeName", "Quick Node"),
			LOCTEXT("QuickNodeDescription", "Quick Node settings"),
			GetMutableDefault<UQuickNodeProjectSettings>());

		ModuleSettings->OnModified().BindLambda([] { RefreshShortcuts(); return true; });

		if (ModuleSettings) {
			if (UQuickNodeProjectSettings* SettingsRef = Cast<UQuickNodeProjectSettings>(ModuleSettings->GetSettingsObject()); SettingsRef != nullptr) {
				SettingsRef->UpdateNodeMessages();
			}
		}
	}

	GEditor->OnBlueprintCompiled().AddLambda([] { RefreshShortcuts(); });

	FString StandardMacrosFilePath = "/Engine/EditorBlueprintResources/StandardMacros.StandardMacros";
	// Add manual functions
	TArray<QuickNodeOption*> BasicGroup;
	BasicGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Add_DoubleDouble")), "Add (+)"));
	BasicGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Subtract_DoubleDouble")), "Subtract (-)"));
	BasicGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Multiply_DoubleDouble")), "Multiply (*)"));
	BasicGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Divide_DoubleDouble")), "Divide (/)"));// 4
	BasicGroup.Add(new QuickNodeFunctionOption(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName("PrintString")), "Print String"));
	BasicGroup.Add(new QuickNodeFunctionOption(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName("PrintText")), "Print Text"));
	BasicGroup.Add(new QuickNodeFunctionOption(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FName("Delay")), "Delay"));
	BasicGroup.Add(new QuickNodeMacroOption(HelperFunctions::GetMacroGraphFromBlueprint(HelperFunctions::FindBlueprintAsset(StandardMacrosFilePath), "IsValid"), "Is Valid"));// 8
	BasicGroup.Add(new QuickNodeSelectOption("Select"));
	ManualGroups.Add("Basics", BasicGroup);

	// Add math functions
	TArray<QuickNodeOption*> MathGroup;
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Sin")), "Sine"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Cos")), "Cosine"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Tan")), "Tan"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Atan")), "Atan"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Atan2")), "Atan2"));// 5
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("SignOfFloat")), "Sign"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Abs")), "Absolute"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("MultiplyMultiply_FloatFloat")), "Power"));
	MathGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Sqrt")), "Square Root"));// 9
	ManualGroups.Add("Math", MathGroup);

	// Add boolean functions
	TArray<QuickNodeOption*> BoolGroup;
	BoolGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Not_PreBool")), "Not (!)"));
	BoolGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("BooleanOR")), "OR (||)"));
	BoolGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("BooleanAND")), "AND (&&)"));
	BoolGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Less_DoubleDouble")), "Less Than (<)"));
	BoolGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Greater_DoubleDouble")), "Greater Than (>)"));
	BoolGroup.Add(new QuickNodePromoteOption(UBlueprintGameplayTagLibrary::StaticClass()->FindFunctionByName(FName("EqualEqual_GameplayTagContainer")), "Equals (==)"));
	BoolGroup.Add(new QuickNodePromoteOption(UBlueprintGameplayTagLibrary::StaticClass()->FindFunctionByName(FName("NotEqual_GameplayTagContainer")), "Not Equals (!=)"));
	BoolGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("LessEqual_DoubleDouble")), "Less Than or\nEqual (<=)"));
	BoolGroup.Add(new QuickNodePromoteOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("GreaterEqual_DoubleDouble")), "Greater Than\nor Equal (>=)"));// 9
	ManualGroups.Add("Boolean", BoolGroup);

	// Add vector functions
	TArray<QuickNodeOption*> VectorGroup;
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("VSize")), "Vector Length"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Normal")), "Normalize"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("VLerp")), "Lerp"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("VEase")), "Ease"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Dot_VectorVector")), "Dot Product"));// 5
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Cross_VectorVector")), "Cross Product"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("GreaterGreater_VectorRotator")), "Rotate Vector"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("LessLess_VectorRotator")), "Unrotate Vector"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Conv_VectorToRotator")), "Get Rotator\nFrom XVector"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Conv_RotatorToVector")), "Get XVector\nFrom Rotator"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("NegateVector")), "Negate"));
	VectorGroup.Add(new QuickNodeFunctionOption(UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("RandomUnitVector")), "Random Unit Vector"));// 12
	ManualGroups.Add("Vector", VectorGroup);

	// Add actor functions
	TArray<QuickNodeOption*> ActorGroup;
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("K2_GetActorLocation")), "GetLocation"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("K2_GetActorRotation")), "GetRotation"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("K2_SetActorLocation")), "SetLocation"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("K2_SetActorRotation")), "SetRotation"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("GetTransform")), "GetTransform"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("K2_SetActorTransform")), "SetTransform"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("GetDistanceTo")), "GetDistanceTo"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("GetActorForwardVector")), "GetForwardVector"));
	ActorGroup.Add(new QuickNodeFunctionOption(AActor::StaticClass()->FindFunctionByName(FName("GetActorRightVector")), "GetRightVector"));// 9
	ManualGroups.Add("Actor", ActorGroup);

	SelfRef = this;
	RegisterShortcuts();

	OnStartup.Broadcast();
#endif
}

void FQuickNodeModule::ShutdownModule()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "QuickNodeSettings");
	}

	if (!ManualGroups.IsEmpty()) {
		for (TPair<FString, TArray<QuickNodeOption*>> Group : ManualGroups) {
			for (int i = 0; i < Group.Value.Num(); i++)
			{
				delete Group.Value[i];
			}
			Group.Value.Empty();
		}
		ManualGroups.Empty();
	}

	CleanShortcuts();

	SelfRef = nullptr;
#endif
}

void FQuickNodeModule::RegisterShortcuts()
{
	QuickNodeCommands::Register();
	FQuickNodeInputProcessor::Create();
}

void FQuickNodeModule::CleanShortcuts()
{
	if (FQuickNodeInputProcessor::Get() != nullptr) {
		FQuickNodeInputProcessor::Get()->Cleanup();
	}
	QuickNodeCommands::Unregister();
}

void FQuickNodeModule::RefreshShortcuts()
{
	UE_LOG(LogTemp, Log, TEXT("Refreshing node groups"));

	// Recreate IsValid macro to fix issue with it not working
	if (SelfRef) {
		if (SelfRef->ManualGroups.Num() > 0 && SelfRef->ManualGroups.Find("Basics")) {
			if (SelfRef->ManualGroups["Basics"][7]) {
				delete SelfRef->ManualGroups["Basics"][7];
				SelfRef->ManualGroups["Basics"][7] = nullptr;
			}
			FString StandardMacrosFilePath = "/Engine/EditorBlueprintResources/StandardMacros.StandardMacros";
			SelfRef->ManualGroups["Basics"][7] = new QuickNodeMacroOption(HelperFunctions::GetMacroGraphFromBlueprint(HelperFunctions::FindBlueprintAsset(StandardMacrosFilePath), "IsValid"), "Is Valid");
		}
	}

	if (SelfRef && SelfRef->ModuleSettings) {
		if (UQuickNodeProjectSettings* SettingsRef = Cast<UQuickNodeProjectSettings>(SelfRef->ModuleSettings->GetSettingsObject()); SettingsRef != nullptr) {
			SettingsRef->UpdateNodeMessages();
		}
	}

	if (FQuickNodeInputProcessor::Get() != nullptr) {
		FQuickNodeInputProcessor::Get()->CreateNodeGroups();
	}
}

TSharedPtr<ISettingsSection> FQuickNodeModule::GetSettingsPointer()
{
	if (SelfRef == nullptr) {
		return nullptr;
	}
	return SelfRef->ModuleSettings;
}

UQuickNodeProjectSettings* FQuickNodeModule::GetSettingsPointerCasted()
{
	if (SelfRef == nullptr) {
		return nullptr;
	}
	return Cast<UQuickNodeProjectSettings>(SelfRef->ModuleSettings->GetSettingsObject());
}

bool FQuickNodeModule::GetHasStarted()
{
	return SelfRef != nullptr;
}

TArray<QuickNodeOption*> FQuickNodeModule::GetManualGroupFromName(FString _Name)
{
	if (SelfRef == nullptr) {
		return TArray<QuickNodeOption*>();
	}
	if (SelfRef->ManualGroups.Contains(_Name)) {
		return SelfRef->ManualGroups[_Name];
	}
	return TArray<QuickNodeOption*>();
}

#undef LOCTEXT_NAMESPACE