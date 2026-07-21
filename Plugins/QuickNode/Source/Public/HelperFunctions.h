#pragma once
#include "CoreMinimal.h"

class HelperFunctions {
public:
    static bool IsPromotablePropertyType(const FProperty* Property);

    static bool IsFunctionPromotableOperator(const UFunction* Function);

    static UBlueprint* FindBlueprintAsset(FString _AssetPath);

    static UEdGraph* GetMacroGraphFromBlueprint(UBlueprint* _Blueprint, FString _GraphName);
};
