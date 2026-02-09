// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShooterServerTarget : TargetRules
{
	public ShooterServerTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		//IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		
        if (Configuration == UnrealTargetConfiguration.Shipping)
        {
            bUseLoggingInShipping = true;
            bUseChecksInShipping = true;
        }
		ExtraModuleNames.AddRange( new string[] { "Shooter" } );
	}
}
