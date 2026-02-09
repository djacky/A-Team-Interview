// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShooterTarget : TargetRules
{
	public ShooterTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		//IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        if (Configuration == UnrealTargetConfiguration.Shipping)
        {
            bUseLoggingInShipping = true;
        }
		ExtraModuleNames.AddRange( new string[] { "Shooter" } );
	}
}
