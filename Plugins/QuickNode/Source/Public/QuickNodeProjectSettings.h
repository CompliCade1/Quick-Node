#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuickNodeProjectSettings.generated.h"

UENUM()
enum class EQN_NodeType : uint8 {
	Function UMETA(DisplayName = "Function"),
	Macro UMETA(DisplayName = "Macro"),
};

USTRUCT(BlueprintType)
struct FQN_NodeInfo {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Node") EQN_NodeType NodeType = EQN_NodeType::Function;
	UPROPERTY(EditAnywhere, Category = "Node") FString ShownName = "";
	UPROPERTY(EditAnywhere, meta = (EditCondition = "NodeType==EQN_NodeType::Function", EditConditionHides), Category = "Node") UClass* FunctionClass = nullptr;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "NodeType==EQN_NodeType::Function", EditConditionHides), Category = "Node") FString FunctionName = "";
	UPROPERTY(EditAnywhere, meta = (EditCondition = "NodeType==EQN_NodeType::Macro", EditConditionHides), Category = "Node") FString MacroFilePath = "";
	UPROPERTY(EditAnywhere, meta = (EditCondition = "NodeType==EQN_NodeType::Macro", EditConditionHides), Category = "Node") FString MacroName = "";
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Node") FString State = "Unknown";

	UBlueprint* MacroBlueprint = nullptr;// Gets stored when checking if macro is valid so it doesn't have to be found again
	UEdGraph* MacroGraph = nullptr;// Gets stored when checking if macro is valid so it doesn't have to be found again

	FQN_NodeInfo() {}
};

USTRUCT(BlueprintType)
struct FRC_NodeGroup {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Group") TArray<FQN_NodeInfo> Nodes;
	UPROPERTY(EditAnywhere, Category = "Group") FString GroupName = "";
	UPROPERTY(EditAnywhere, Category = "Group") bool UseGroup = true;

	FRC_NodeGroup() {}
};

UCLASS(Config = QuickNode, DefaultConfig)
class QUICKNODE_API UQuickNodeProjectSettings : public UObject
{
	GENERATED_BODY()

public:
	UQuickNodeProjectSettings();
	UQuickNodeProjectSettings(const FObjectInitializer& obj);

	void UpdateNodeMessages();
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool UseBasics = true;
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool UseMath = true;
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool UseBoolean = true;
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool UseVector = true;
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool UseActor = true;
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") bool PluginEnabled = true;// Note: This will only cause the node window to not pop up when pressing z. To not lose performance to this plugin, turn off this plugin in Plugins. 
	UPROPERTY(Config, EditAnywhere, Category = "QuickNode") TArray<FRC_NodeGroup> CustomGroups;
};
