// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BattlemageTheEndless : ModuleRules
{
	public BattlemageTheEndless(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "CommonUI", "GameplayTags", 
			"GameplayAbilities", "GameplayTasks", "Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}
