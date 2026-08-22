// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectOrganoid : ModuleRules
{
	public ProjectOrganoid(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"PhysicsCore",
			"HTTP",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectOrganoid",
			"ProjectOrganoid/UI",
			"ProjectOrganoid/Inventory",
			"ProjectOrganoid/Weapons",
			"ProjectOrganoid/Hosts",
			"ProjectOrganoid/Interaction",
			"ProjectOrganoid/Progression",
			"ProjectOrganoid/Feedback",
			"ProjectOrganoid/LevelStreaming",
			"ProjectOrganoid/Narrative",
			"ProjectOrganoid/Variant_Platforming",
			"ProjectOrganoid/Variant_Platforming/Animation",
			"ProjectOrganoid/Variant_Combat",
			"ProjectOrganoid/Variant_Combat/AI",
			"ProjectOrganoid/Variant_Combat/Animation",
			"ProjectOrganoid/Variant_Combat/Gameplay",
			"ProjectOrganoid/Variant_Combat/Interfaces",
			"ProjectOrganoid/Variant_Combat/UI",
			"ProjectOrganoid/Variant_SideScrolling",
			"ProjectOrganoid/Variant_SideScrolling/AI",
			"ProjectOrganoid/Variant_SideScrolling/Gameplay",
			"ProjectOrganoid/Variant_SideScrolling/Interfaces",
			"ProjectOrganoid/Variant_SideScrolling/UI"
		});
	}
}
