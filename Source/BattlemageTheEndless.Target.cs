// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BattlemageTheEndlessTarget : TargetRules
{
	public BattlemageTheEndlessTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		ExtraModuleNames.Add("BattlemageTheEndless");
	}
}
