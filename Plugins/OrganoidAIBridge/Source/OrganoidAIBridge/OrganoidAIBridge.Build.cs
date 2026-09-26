using UnrealBuildTool;

public class OrganoidAIBridge : ModuleRules
{
	public OrganoidAIBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Commands.cpp and Writes.cpp both define the same anonymous-namespace helpers;
		// unity amalgamation of both TUs fails with C2084. Keep this module non-unity.
		bUseUnity = false;

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
			"AssetTools",
			"Kismet",
			"BlueprintGraph",
			"Slate",
			"SlateCore"
		});
	}
}
