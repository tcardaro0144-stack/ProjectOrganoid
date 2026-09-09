using UnrealBuildTool;

public class ProjectOrganoidPlaytest : ModuleRules
{
	public ProjectOrganoidPlaytest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorSubsystem",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"UMG",
			"InputCore",
			"EnhancedInput",
			"OrganoidAIBridge",
			"ProjectOrganoid",
			"NavigationSystem",
			"AIModule"
		});
	}
}
