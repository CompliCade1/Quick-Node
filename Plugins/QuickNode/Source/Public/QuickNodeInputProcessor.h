#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "QuickNodeInputProcessor.generated.h"

class UQuickNodeObject;
class FUICommandList;
class FBlueprintEditor;
class UK2Node;
class FBlueprintEditor;

class QuickNodeOption {
public:
	TSubclassOf<UK2Node> Type;
	FString NodeName;

	virtual bool SetUpNode(UK2Node* _Node);
	virtual ~QuickNodeOption() = default;
protected:
	QuickNodeOption(TSubclassOf<UK2Node> _Type, FString _NodeName = "");
};

class QuickNodeFunctionOption : public QuickNodeOption {
public:
	QuickNodeFunctionOption(UFunction* _Function, FString _NodeName = "");
	~QuickNodeFunctionOption() = default;

	bool SetUpNode(UK2Node* _Node) override;
protected:
	UFunction* NodeFunction = nullptr;
};

class QuickNodePromoteOption : public QuickNodeOption {
public:
	QuickNodePromoteOption(UFunction* _Function, FString _NodeName = "");
	~QuickNodePromoteOption() = default;

	bool SetUpNode(UK2Node* _Node) override;
protected:
	UFunction* NodeFunction = nullptr;
};

class QuickNodeMacroOption : public QuickNodeOption {
public:
	QuickNodeMacroOption(UEdGraph* _Macro, FString _NodeName = "");
	~QuickNodeMacroOption() = default;

	bool SetUpNode(UK2Node* _Node) override;
protected:
	UEdGraph* NodeMacro = nullptr;
};

class QuickNodeVarOption : public QuickNodeOption {
public:
	QuickNodeVarOption(FString _VarName = "");
	~QuickNodeVarOption() = default;

	bool SetUpNode(UK2Node* _Node) override;
};

class QuickNodeSelectOption : public QuickNodeOption {
public:
	QuickNodeSelectOption(FString _NodeName);
	~QuickNodeSelectOption() = default;

	bool SetUpNode(UK2Node* _Node) override;
};

USTRUCT(BlueprintType, Blueprintable)
struct FRC_OptionGroup {
	GENERATED_BODY()

	TArray<QuickNodeOption*> Options;
	FString GroupName = "";

	FRC_OptionGroup() {}
	FRC_OptionGroup(TArray<QuickNodeOption*> _Options, FString _Name) {
		Options = _Options;
		GroupName = _Name;
	}
};

class FQuickNodeInputProcessor
	: public TSharedFromThis<FQuickNodeInputProcessor>
	, public IInputProcessor
{
public:
	virtual ~FQuickNodeInputProcessor() override;

	static void Create();

	static TSharedPtr<FQuickNodeInputProcessor> Get();

	void Cleanup();

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	FQuickNodeInputProcessor();

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;

	void CreateNodeGroups();
protected:
	TSharedPtr<FUICommandList> CustomCommands;

	bool ToggledOn = true;
	bool ShouldDrawMenu = false;
	FVector2D MousePos;
	int CurrentSelected = -1;
	int UserOptionsIndex = -1;
	TArray<FRC_OptionGroup> OptionGroups;
	int ClassGroupIndex = -1;
	FBlueprintEditor* CurrentBlueprint = nullptr;
	FString SelectWindowName = "SelectWindow";
	const char* PreviewWindowName = "Preview";

	int SelectedGroup = 0;
	const float StartMoveDelay = 0.2f;
	const float MoveDelay = 0.2f;

	UQuickNodeObject* GetQuickNodeObjectByCommandID(int32 CommandID) const;

	void DeleteUserNodes();
	void BeginZHeld();
	void SetSelectedGroup(int _Group);
	void UpdateWindowSize();

	void DrawSelectWindow();
	void CheckShouldCreateNode();
	void CreateBlueprintNode(QuickNodeOption* _NodeDetails);

	float GetCurrentZoom();
};
