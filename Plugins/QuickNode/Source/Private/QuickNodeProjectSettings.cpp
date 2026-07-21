#include "QuickNodeProjectSettings.h"
#include "HelperFunctions.h"

UQuickNodeProjectSettings::UQuickNodeProjectSettings()
{
	UpdateNodeMessages();
}

UQuickNodeProjectSettings::UQuickNodeProjectSettings(const FObjectInitializer& obj)
{
	UpdateNodeMessages();
}

void UQuickNodeProjectSettings::UpdateNodeMessages()
{
	for (FRC_NodeGroup& Group : CustomGroups)
	{
		for (FQN_NodeInfo& Info : Group.Nodes)
		{
			if (Info.NodeType == EQN_NodeType::Function) {// Function checks
				if (Info.FunctionClass == nullptr) {
					Info.State = "A class must be selected";
				}
				else if (Info.FunctionName == "") {
					Info.State = "The name of the function must be given";
				}
				else if (!IsValid(Info.FunctionClass)) {
					Info.State = "The selected class is invalid";
				}
				else if (Info.FunctionClass->FindFunctionByName((FName)Info.FunctionName) == nullptr) {
					Info.State = "The given function can't be found";
				}
				else if (Info.ShownName == "") {
					Info.State = "You may want to give a name to be shown";
				}
				else {
					Info.State = "All Good :D";
				}
			}
			else {// Macro checks
				if (Info.MacroFilePath == "") {
					Info.State = "You need to give a file path to the object the macro is from";
				}
				else if (Info.MacroBlueprint = HelperFunctions::FindBlueprintAsset(Info.MacroFilePath); Info.MacroBlueprint == nullptr) {
					Info.State = "The filepath doesn't find an object";
				}
				else if (Info.MacroName == "") {
					Info.State = "You need to give the name of the macro";
				}
				else if (Info.MacroGraph = HelperFunctions::GetMacroGraphFromBlueprint(Info.MacroBlueprint, Info.MacroName); Info.MacroGraph == nullptr) {
					Info.State = "The macro can't be found in the object";
				}
				else if (Info.ShownName == "") {
					Info.State = "You may want to give a name to be shown";
				}
				else {
					Info.State = "All Good :D";
				}
			}
		}
	}
}
