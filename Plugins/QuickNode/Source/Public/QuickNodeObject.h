#pragma once
#include "QuickNodeObject.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UQuickNodeObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Quick Node")
	FInputChord DefaultInputChord = FInputChord(FKey("Z"));

	UPROPERTY(EditAnywhere, Category = "Quick Node")
	FString Name = "Name";

	UPROPERTY(EditAnywhere, Category = "Quick Node")
	FString Description = "Description";

	UFUNCTION(BlueprintNativeEvent)
	void OnShortcutExecuted();

	UFUNCTION(BlueprintNativeEvent)
	bool CanExecuteShortcut();

};
