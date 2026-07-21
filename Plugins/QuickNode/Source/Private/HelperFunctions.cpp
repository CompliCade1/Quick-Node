#include "HelperFunctions.h"
#include "K2Node_PromotableOperator.h"
#include "EdGraphSchema_K2.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"

bool HelperFunctions::IsPromotablePropertyType(const FProperty* Property)
{
    if (!Property)
    {
        return false;
    }

    return
        Property->IsA<FByteProperty>() ||
        Property->IsA<FIntProperty>() ||
        Property->IsA<FInt64Property>() ||
        Property->IsA<FFloatProperty>() ||
        Property->IsA<FDoubleProperty>() ||
        Property->IsA<FBoolProperty>();
}

bool HelperFunctions::IsFunctionPromotableOperator(const UFunction* Function)
{
    if (!Function)
    {
        return false;
    }

    // Must be blueprint-callable or pure
    if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
    {
        return false;
    }

    // Cannot be latent, net, or exec-function-only
    if (Function->HasAnyFunctionFlags(FUNC_Net | FUNC_BlueprintAuthorityOnly | FUNC_BlueprintCosmetic | FUNC_Exec))
    {
        return false;
    }

    // Count inputs and outputs
    int32 InputCount = 0;
    int32 OutputCount = 0;
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    for (TFieldIterator<FProperty> It(Function); It; ++It)
    {
        const FProperty* Property = *It;

        // Skip non-param properties
        if (!Property->HasAnyPropertyFlags(CPF_Parm))
        {
            continue;
        }

        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            ++OutputCount;
        }
        else if (Property->HasAnyPropertyFlags(CPF_Parm))
        {
            ++InputCount;
        }

        // Operator node only supports promotable types:
        // numeric (byte, int, int64, float, double), bool

        if (!IsPromotablePropertyType(Property))
        {
            return false;
        }
    }

    // Needs exactly one output (the result)
    if (OutputCount != 1)
    {
        return false;
    }

    // Needs at least one input
    if (InputCount < 1)
    {
        return false;
    }

    return true;
}

UBlueprint* HelperFunctions::FindBlueprintAsset(FString _AssetPath)
{
    if (_AssetPath == "") {
        return nullptr;
    }

    if (!FModuleManager::Get().IsModuleLoaded("AssetRegistry")) {
        return nullptr;
    }
    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FAssetData Asset = AssetRegistry.Get().GetAssetByObjectPath(FSoftObjectPath(_AssetPath));

    if (!Asset.IsValid()) {
        UE_LOG(LogTemp, Log, TEXT("No asset found at file path"));
        return nullptr;
    }

    UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
    if (!Blueprint) {
        UE_LOG(LogTemp, Log, TEXT("Asset isn't a blueprint"));
        return nullptr;
    }

    return Blueprint;
}

UEdGraph* HelperFunctions::GetMacroGraphFromBlueprint(UBlueprint* _Blueprint, FString _GraphName)
{
    if (_Blueprint == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < _Blueprint->MacroGraphs.Num(); i++)
    {
        if (_Blueprint->MacroGraphs[i].GetName() == _GraphName) {
            return _Blueprint->MacroGraphs[i];
        }
    }

    return nullptr;
}
