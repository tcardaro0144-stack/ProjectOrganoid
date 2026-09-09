using UnrealBuildTool;

public class OrganoidAIBridge : ModuleRules
{
	public OrganoidAIBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"NavigationSystem",
			"HTTPServer",
			"Sockets",
			"Json",
			"JsonUtilities",
			"AssetRegistry",
			"Kismet",
			"BlueprintGraph",
			"Slate",
			"SlateCore"
		});
	}
}
