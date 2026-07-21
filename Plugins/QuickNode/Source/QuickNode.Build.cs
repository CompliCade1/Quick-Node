
using UnrealBuildTool;
using System.IO;

public class QuickNode : ModuleRules
{
    public QuickNode(ReadOnlyTargetRules Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "DeveloperSettings",
            }
            );
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine",
                "InputCore",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "ToolMenus",
                "ImGui",
                "Kismet",
                "KismetCompiler",
                "BlueprintGraph",
                "GameplayTags",
            }
            );
    }

}
