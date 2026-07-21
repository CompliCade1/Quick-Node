#pragma once
#include "QuickNodeInputProcessor.h"
#include "QuickNodeModule.h"
#include "QuickNodeProjectSettings.h"
#include "QuickNodeCommands.h"
#include "HelperFunctions.h"

#include "EditorBuildUtils.h"
#include "Engine.h"

#include "BlueprintEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ToolMenus.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Editor/UnrealEd/Public/Subsystems/AssetEditorSubsystem.h"
#include "K2Node_CallFunction.h"
#include "K2Node_PromotableOperator.h"
#include "K2Node_VariableGet.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Select.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Toolkits/ToolkitManager.h"
#include "BlueprintEditorModule.h"

#include <imgui.h>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "D3D11.h"


// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}
const ImVec2 ButtonSize = ImVec2(150.f, 100.f);
ImVec2 SelectWindowSize = ImVec2(340, 150);

static TSharedPtr<FQuickNodeInputProcessor> QuickNodeInputProcessorInstance = nullptr;

FQuickNodeInputProcessor::~FQuickNodeInputProcessor()
{
	DeleteUserNodes();
}

void FQuickNodeInputProcessor::Create()
{
	QuickNodeInputProcessorInstance = MakeShareable(new FQuickNodeInputProcessor());
	if (!FEditorBuildUtils::IsBuildCurrentlyRunning() && FSlateApplication::IsInitialized())
	{
		if (FSlateApplication* SlateApplication = &FSlateApplication::Get())
		{
			SlateApplication->RegisterInputPreProcessor(QuickNodeInputProcessorInstance);
		}
	}
}

TSharedPtr<FQuickNodeInputProcessor> FQuickNodeInputProcessor::Get()
{
	return QuickNodeInputProcessorInstance;
}

void FQuickNodeInputProcessor::Cleanup()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(QuickNodeInputProcessorInstance);
	}

	QuickNodeInputProcessorInstance.Reset();
}

bool FQuickNodeInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (SlateApp.IsInitialized())
	{
		if (CustomCommands->ProcessCommandBindings(
			InKeyEvent.GetKey(),
			SlateApp.GetModifierKeys(),
			InKeyEvent.IsRepeat()))
		{
			BeginZHeld();
			return true;
		}
	}

	return IInputProcessor::HandleKeyDownEvent(SlateApp, InKeyEvent);
}

bool FQuickNodeInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (SlateApp.IsInitialized())
	{
		if (CustomCommands->ProcessCommandBindings(
			InKeyEvent.GetKey(),
			SlateApp.GetModifierKeys(),
			InKeyEvent.IsRepeat()))
		{
			return true;
		}
	}

	return IInputProcessor::HandleKeyUpEvent(SlateApp, InKeyEvent);
}

FQuickNodeInputProcessor::FQuickNodeInputProcessor()
{
	if (FQuickNodeModule::GetHasStarted()) {
		CreateNodeGroups();
	}
	else {
		FQuickNodeModule::OnStartup.AddLambda([this]() {this->CreateNodeGroups(); });
	}

	CustomCommands = MakeShareable(new FUICommandList());

	if (!QuickNodeCommandsImpl::IsRegistered())
	{
		return;
	}

	if (QuickNodeCommands::Get().CustomCommands.IsEmpty())
	{
		return;
	}

	for (int32 i = 0; i < QuickNodeCommands::Get().CustomCommands.Num(); i++)
	{
		TSharedPtr<FUICommandInfo> Command = QuickNodeCommands::Get().CustomCommands[i];

		CustomCommands->MapAction(
			Command,
			FExecuteAction::CreateLambda([this, i]()
				{
					if (UQuickNodeObject* CustomShortcutObject = GetQuickNodeObjectByCommandID(i); IsValid(CustomShortcutObject))
					{
						// Prevents the instance of custom shortcut object from being garbage collected if OnShortcutExecuted has delay
						CustomShortcutObject->AddToRoot();

						CustomShortcutObject->OnShortcutExecuted();

						// Release the instance
						CustomShortcutObject->RemoveFromRoot();
					}
				}),
			FCanExecuteAction::CreateLambda([this, i]
				{
					if (UQuickNodeObject* CustomShortcutObject = GetQuickNodeObjectByCommandID(i); IsValid(CustomShortcutObject))
					{
						// Prevents the instance of custom shortcut object from being garbage collected if CanExecuteShortcut has delay
						CustomShortcutObject->AddToRoot();

						const bool bCanExecute = CustomShortcutObject->CanExecuteShortcut();

						// Release the instance
						CustomShortcutObject->RemoveFromRoot();

						return bCanExecute;
					}

					return false;
				})
		);
	}
}

void FQuickNodeInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	static bool LWasHeld = false;
	// If L is held
	if (GetKeyState('l') & 0x8000 || GetKeyState('L') & 0x8000) {
		if (!LWasHeld) {// If just pressed
			if (GetKeyState(VK_SHIFT) & 0x8000 || GetKeyState(VK_CONTROL) & 0x8000) {
				ToggledOn = !ToggledOn;
			}
		}
		LWasHeld = true;
	}
	else {
		LWasHeld = false;
	}

	const ImGui::FScopedContext ScopedContext;
	static bool ZWasHeld = false;
	if (ScopedContext && ShouldDrawMenu)
	{
		ImGui::SetWindowSize(PreviewWindowName, ImVec2(128 / GetCurrentZoom(), 64 / GetCurrentZoom()));
		DrawSelectWindow();
		// If Z is held
		if (GetKeyState('z') & 0x8000 || GetKeyState('Z') & 0x8000) {
			if (!ZWasHeld) {
				ImGui::SetWindowPos(TCHAR_TO_UTF8(*SelectWindowName), MousePos - FVector2D(SelectWindowSize) / 2.f);
				ImGui::SetWindowSize(TCHAR_TO_UTF8(*SelectWindowName), SelectWindowSize);
				ImGui::SetWindowPos(PreviewWindowName, MousePos);
			}
			ZWasHeld = true;
		}
		else if (ZWasHeld) {
			CheckShouldCreateNode();
			ShouldDrawMenu = false;
			ZWasHeld = false;
		}

		// Check moving left
		static bool XHeld = false;
		static float XHoldTime = 0.f;
		if (GetKeyState('x') & 0x8000 || GetKeyState('X') & 0x8000) {// If X is held
			if (XHeld == false) {
				SetSelectedGroup(FMath::Max(SelectedGroup - 1, 0));
				XHoldTime = 0.f;
			}
			else {
				XHoldTime += DeltaTime;
				if (XHoldTime > StartMoveDelay + MoveDelay) {
					XHoldTime -= MoveDelay;
					SetSelectedGroup(FMath::Max(SelectedGroup - 1, 0));
				}
			}
			XHeld = true;
		}
		else {
			XHeld = false;
		}

		// Check moving right
		static bool CHeld = false;
		static float CHoldTime = 0.f;
		if (GetKeyState('c') & 0x8000 || GetKeyState('C') & 0x8000) {// If C is held
			if (CHeld == false) {
				SetSelectedGroup(FMath::Min(SelectedGroup + 1, OptionGroups.Num() - 1));
				CHoldTime = 0.f;
			}
			else {
				CHoldTime += DeltaTime;
				if (CHoldTime > StartMoveDelay + MoveDelay) {
					CHoldTime -= MoveDelay;
					SetSelectedGroup(FMath::Max(SelectedGroup + 1, 0));
				}
			}
			CHeld = true;
		}
		else {
			CHeld = false;
		}
	}
}

UQuickNodeObject* FQuickNodeInputProcessor::GetQuickNodeObjectByCommandID(int32 CommandID) const
{
	const TSharedPtr<FUICommandInfo> Command = QuickNodeCommands::Get().CustomCommands[CommandID];

	if (!Command)
		return nullptr;

	const TSubclassOf<UQuickNodeObject> CustomShortcutClass = QuickNodeCommands::Get().
		QuickNodeClasses[Command];

	if (!IsValid((CustomShortcutClass)))
		return nullptr;

	UQuickNodeObject* QuickNodeObject = Cast<UQuickNodeObject>(
		NewObject<UObject>(GetTransientPackage(), CustomShortcutClass));

	return QuickNodeObject;
}

void FQuickNodeInputProcessor::CreateNodeGroups()
{
	//UKismetMathLibrary is math functions
	//UKismetSystemLibrary is utility functions

	// Potential Additions: Combine Rotators
	DeleteUserNodes();
	OptionGroups.Empty();

	UQuickNodeProjectSettings* SettingsRef = FQuickNodeModule::GetSettingsPointerCasted();

	// Add build in groups
	if (!SettingsRef || SettingsRef->UseBasics) {
		OptionGroups.Add(FRC_OptionGroup(FQuickNodeModule::GetManualGroupFromName("Basics"), "Basics"));
	}

	if (!SettingsRef || SettingsRef->UseMath) {
		OptionGroups.Add(FRC_OptionGroup(FQuickNodeModule::GetManualGroupFromName("Math"), "Math"));
	}

	if (!SettingsRef || SettingsRef->UseBoolean) {
		OptionGroups.Add(FRC_OptionGroup(FQuickNodeModule::GetManualGroupFromName("Boolean"), "Boolean"));
	}

	if (!SettingsRef || SettingsRef->UseVector) {
		OptionGroups.Add(FRC_OptionGroup(FQuickNodeModule::GetManualGroupFromName("Vector"), "Vector"));
	}

	if (!SettingsRef || SettingsRef->UseActor) {
		OptionGroups.Add(FRC_OptionGroup(FQuickNodeModule::GetManualGroupFromName("Actor"), "Actor"));
	}

	// Add get class variables group
	//OptionGroups.Add(FRC_OptionGroup(TArray<FRC_Option>(), "Variables"));
	//ClassGroupIndex = OptionGroups.Num() - 1;

	// Add user defined groups
	UserOptionsIndex = OptionGroups.Num();
	TArray<FRC_NodeGroup> UserLists = GetDefault<UQuickNodeProjectSettings>()->CustomGroups;
	for (FRC_NodeGroup NodeList : UserLists) {
		if (!NodeList.UseGroup) {
			continue;
		}

		TArray<QuickNodeOption*> NewList = TArray<QuickNodeOption*>();
		TArray<FQN_NodeInfo> List = NodeList.Nodes;
		for (int i = 0; i < List.Num(); i++)
		{
			if (List[i].NodeType == EQN_NodeType::Function) {
				if (List[i].FunctionClass == nullptr) {
					continue;
				}
				UFunction* FunctionRef = List[i].FunctionClass->FindFunctionByName((FName)List[i].FunctionName);
				if (FunctionRef == nullptr) {
					continue;
				}

				NewList.Add(new QuickNodeFunctionOption(FunctionRef, List[i].ShownName));
			}
			else {
				if (List[i].MacroBlueprint == nullptr || List[i].MacroGraph == nullptr) {
					continue;
				}

				NewList.Add(new QuickNodeMacroOption(List[i].MacroGraph, List[i].ShownName));
			}
		}

		if (NewList.Num() == 0) {
			continue;
		}
		OptionGroups.Add(FRC_OptionGroup(NewList, NodeList.GroupName));
	}

	SelectedGroup = FMath::Min(SelectedGroup, OptionGroups.Num() - 1);

	UpdateWindowSize();
}

void FQuickNodeInputProcessor::DeleteUserNodes()
{
	if (UserOptionsIndex == -1) {
		return;
	}
	for (int i = OptionGroups.Num() - 1; i >= UserOptionsIndex; i--)
	{
		for (int j = 0; j < OptionGroups[i].Options.Num(); j++)
		{
			delete OptionGroups[i].Options[j];
		}
		OptionGroups.RemoveAt(i);
	}
	UserOptionsIndex = -1;
}

void FQuickNodeInputProcessor::BeginZHeld()
{
	if (!ToggledOn) {
		return;
	}
	MousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };

	CurrentBlueprint = nullptr;
	if (ClassGroupIndex != -1) {
		OptionGroups[ClassGroupIndex].Options.Empty();
	}
	TArray<UObject*> Editors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetAllEditedAssets();
	for (UObject* obj : Editors)
	{
		IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(obj, false);
		FAssetEditorToolkit* editor = static_cast<FAssetEditorToolkit*>(AssetEditor);

		TSharedPtr<SDockTab> tab = editor->GetTabManager()->GetOwnerTab();
		if (tab->IsForeground()) {
			if (editor->GetEditingAssetTypeName() != "Blueprint") {
				continue;
			}
			CurrentBlueprint = static_cast<FBlueprintEditor*>(editor);
			ShouldDrawMenu = true;

			// Get class variables
			if (ClassGroupIndex == -1) {
				return;
			}
			TArray<FBPVariableDescription> ClassVariables = CurrentBlueprint->GetBlueprintObj()->NewVariables;
			for (int i = 0; i < ClassVariables.Num(); i++)
			{
				OptionGroups[ClassGroupIndex].Options.Add(new QuickNodeVarOption(ClassVariables[i].VarName.ToString()));
			}
			UpdateWindowSize();
		}
	}
}

void FQuickNodeInputProcessor::SetSelectedGroup(int _Group)
{
	if (!OptionGroups.IsValidIndex(_Group)) {
		return;
	}
	SelectedGroup = _Group;
	UpdateWindowSize();

	ImGui::SetWindowPos(TCHAR_TO_UTF8(*SelectWindowName), MousePos - FVector2D(SelectWindowSize) / 2.f);
	ImGui::SetWindowSize(TCHAR_TO_UTF8(*SelectWindowName), SelectWindowSize);
}

void FQuickNodeInputProcessor::UpdateWindowSize()
{
	int OptNum = 0;
	if (OptionGroups.IsValidIndex(SelectedGroup)) {
		OptNum = OptionGroups[SelectedGroup].Options.Num();
	}
	int XNum = FMath::Clamp(OptNum, 1, 3);
	int YNum = FMath::Max((OptNum + 2) / 3, 1);// Add row for every 3 options
	ImVec2 ButtonPadding = { 8.f, 6.f };
	float TextHeight = 22.f;
	SelectWindowSize = ImVec2(ButtonSize.x * XNum + ButtonPadding.x * (XNum + 1), ButtonSize.y * YNum + ButtonPadding.y * (YNum + 1) + TextHeight);
}

void FQuickNodeInputProcessor::DrawSelectWindow()
{
	ImGuiWindowFlags PreviewFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	if (ImGui::Begin(PreviewWindowName, nullptr, PreviewFlags)) {

	}
	ImGui::End();

	ImGuiStyle& Style = ImGui::GetStyle();
	Style.WindowRounding = 10.f;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.3f, 0.1f, 0.2f));

	ImGuiWindowFlags SelectFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	if (ImGui::Begin(TCHAR_TO_UTF8(*SelectWindowName), nullptr, SelectFlags)) {
		if (OptionGroups.IsValidIndex(SelectedGroup)) {
			FString Text = OptionGroups[SelectedGroup].GroupName + " " + FString::FromInt(SelectedGroup + 1) + "/" + FString::FromInt(OptionGroups.Num());
			ImGui::SetCursorPosX(SelectWindowSize.x / 2.f - ImGui::CalcTextSize(TCHAR_TO_UTF8(*Text)).x / 2.f);
			ImGui::Text(TCHAR_TO_UTF8(*Text));

			CurrentSelected = -1;
			TArray<QuickNodeOption*> CurrentOptions = OptionGroups[SelectedGroup].Options;
			for (int i = 0; i < CurrentOptions.Num(); i++)
			{
				if (i % 3 != 0) {
					ImGui::SameLine();
				}
				ImGui::PushID(TCHAR_TO_UTF8(*(CurrentOptions[i]->NodeName + FString::FromInt(i))));
				ImGui::Button(TCHAR_TO_UTF8(*CurrentOptions[i]->NodeName), ButtonSize);
				if (ImGui::IsItemHovered()) {
					CurrentSelected = i;
				}
				ImGui::PopID();
			}
		}
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

void FQuickNodeInputProcessor::CheckShouldCreateNode()
{
	if (CurrentBlueprint == nullptr || !OptionGroups.IsValidIndex(SelectedGroup)) {
		return;
	}

	// Get all currently opened asset editors
	TArray<UObject*> Editors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetAllEditedAssets();

	for (UObject* obj : Editors)
	{
		IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(obj, false);
		FAssetEditorToolkit* editor = static_cast<FAssetEditorToolkit*>(AssetEditor);

		TSharedPtr<SDockTab> tab = editor->GetTabManager()->GetOwnerTab();
		if (tab->IsForeground()) {
			FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(editor);
			if (BlueprintEditor != CurrentBlueprint) {
				return;
			}

			if (OptionGroups[SelectedGroup].Options.IsValidIndex(CurrentSelected)) {
				CreateBlueprintNode(OptionGroups[SelectedGroup].Options[CurrentSelected]);
			}

			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No Blueprint Graph Editor is currently open."));
}

void FQuickNodeInputProcessor::CreateBlueprintNode(QuickNodeOption* _NodeOption)
{
	if (!CurrentBlueprint || !_NodeOption) {
		if (!CurrentBlueprint) {
			UE_LOG(LogTemp, Warning, TEXT("CurrentBlueprint is invalid"));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Node Details struct is invalid"));
		}
		return;
	}

	// Get blueprint asset
	UBlueprint* BlueprintRef = CurrentBlueprint->GetBlueprintObj();
	if (BlueprintRef == nullptr) {
		return;
	}

	// Get the current blueprint graph
	UEdGraph* BlueprintGraph = CurrentBlueprint->GetFocusedGraph();
	if (!BlueprintGraph) {
		return;
	}

	// Begin transaction for undo/redo support
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "AddNode", "Add Blueprint Node"));
	BlueprintGraph->Modify();
	BlueprintRef->Modify();

	// Create the node
	UK2Node* PrintNode = NewObject<UK2Node>(BlueprintGraph, _NodeOption->Type);
	PrintNode->CreateNewGuid();
	PrintNode->PostPlacedNewNode();
	PrintNode->SetFlags(RF_Transactional);

	if (!_NodeOption->SetUpNode(PrintNode)) {
		PrintNode->DestroyNode();
		return;
	}

	PrintNode->AllocateDefaultPins();

	// Get offset and zoom of the current graph
	FVector2D Offset;
	float Zoom;
	CurrentBlueprint->GetViewLocation(Offset, Zoom);

	// Get position of the window
	FVector2D RelativePos = (FVector2D)CurrentBlueprint->OpenGraphAndBringToFront(BlueprintGraph)->GetCachedGeometry().GetAbsolutePosition();

	// Set the position of node in the graph
	FVector2D SpawnPos = (MousePos - RelativePos) / Zoom + Offset;
	PrintNode->NodePosX = SpawnPos.X;
	PrintNode->NodePosY = SpawnPos.Y;

	// Add node to the graph
	BlueprintGraph->AddNode(PrintNode, true, false);

	// Notify Blueprint system of changes
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BlueprintRef);
}

float FQuickNodeInputProcessor::GetCurrentZoom()
{
	if (CurrentBlueprint == nullptr) {
		return 1.f;
	}

	// Get offset and zoom of the current graph
	FVector2D Offset;
	float Zoom;
	CurrentBlueprint->GetViewLocation(Offset, Zoom);

	return Zoom;
}

bool QuickNodeOption::SetUpNode(UK2Node* _Node)
{
	return false;
}

QuickNodeOption::QuickNodeOption(TSubclassOf<UK2Node> _Type, FString _NodeName)
{
	Type = _Type;
	NodeName = _NodeName;
}

QuickNodeFunctionOption::QuickNodeFunctionOption(UFunction* _Function, FString _NodeName) : QuickNodeOption(UK2Node_CallFunction::StaticClass(), _NodeName)
{
	NodeFunction = _Function;
}

bool QuickNodeFunctionOption::SetUpNode(UK2Node* _Node)
{
	if (_Node == nullptr) {
		return false;
	}
	if (!IsValid(NodeFunction)) {
		return false;
	}
	UK2Node_CallFunction* NodeRef = Cast<UK2Node_CallFunction>(_Node);
	if (NodeRef == nullptr) {
		return false;
	}

	NodeRef->SetFromFunction(NodeFunction);

	return true;
}

QuickNodePromoteOption::QuickNodePromoteOption(UFunction* _Function, FString _NodeName) : QuickNodeOption(UK2Node_PromotableOperator::StaticClass(), _NodeName)
{
	NodeFunction = _Function;
}

bool QuickNodePromoteOption::SetUpNode(UK2Node* _Node)
{
	if (!IsValid(NodeFunction)) {
		return false;
	}
	UK2Node_PromotableOperator* NodeRef = Cast<UK2Node_PromotableOperator>(_Node);
	if (NodeRef == nullptr) {
		return false;
	}

	NodeRef->SetFromFunction(NodeFunction);

	return true;
}

QuickNodeMacroOption::QuickNodeMacroOption(UEdGraph* _Macro, FString _NodeName) : QuickNodeOption(UK2Node_MacroInstance::StaticClass(), _NodeName)
{
	NodeMacro = _Macro;
}

bool QuickNodeMacroOption::SetUpNode(UK2Node* _Node)
{
	if (NodeMacro == nullptr) {
		return false;
	}
	UK2Node_MacroInstance* NodeRef = Cast<UK2Node_MacroInstance>(_Node);
	if (NodeRef == nullptr) {
		return false;
	}

	NodeRef->SetMacroGraph(NodeMacro);

	return true;
}

QuickNodeVarOption::QuickNodeVarOption(FString _VarName) : QuickNodeOption(UK2Node_VariableGet::StaticClass(), _VarName)
{
}

bool QuickNodeVarOption::SetUpNode(UK2Node* _Node)
{
	if (NodeName == "") {
		return false;
	}

	UK2Node_VariableGet* NodeRef = Cast<UK2Node_VariableGet>(_Node);
	if (NodeRef == nullptr) {
		return false;
	}

	NodeRef->VariableReference.SetSelfMember((FName)NodeName);

	return true;
}

QuickNodeSelectOption::QuickNodeSelectOption(FString _NodeName) : QuickNodeOption(UK2Node_Select::StaticClass(), _NodeName)
{
}

bool QuickNodeSelectOption::SetUpNode(UK2Node* _Node)
{
	return true;
}
