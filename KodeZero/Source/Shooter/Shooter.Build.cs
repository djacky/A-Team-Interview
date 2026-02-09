// Copyright Epic Games, Inc. All Rights Reserved.

//using System.IO;
using UnrealBuildTool;

public class Shooter : ModuleRules
{
	public Shooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bUseRTTI = true; // For Python Interpreter
        //bEnableExceptions = true; // For Python Interpreter
		PublicIncludePaths.AddRange(
		new string[] {
					"Shooter"
			}
		);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ApplicationCore", "UMG", "Niagara", "HTTP", "HttpBlueprint", "Json", "JsonUtilities" , "AIModule", "NetCore", "GameplayTags", "ModularGameplayActors", "ModularGameplay", "NavigationSystem", "MediaAssets", "GeometryCollectionEngine", "OpenSSL", "WebSockets", "AnimGraphRuntime", "TSBC_Plugin_Runtime", "NiagaraUIRenderer"});
		// to add AWS to c++, add these in public dependency above: "GameLiftClientLibrary", "AWSCoreLibrary", "CognitoIdentityClientLibrary"

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "RenderCore", "DeveloperSettings", "EnhancedInput", "AudioModulation", "GameSubtitles", "AudioMixer", "CommonInput", "CommonUI" });

		if (Target.Type != TargetRules.TargetType.Server)
        {
			PublicDefinitions.Add("WITH_CLIENT_ONLY=1");
			PrivateDependencyModuleNames.Add("VivoxCore");
		}
		else
		{
			PublicDefinitions.Add("WITH_CLIENT_ONLY=0");
		}
		
		if (Target.Name == "ShooterSteam")
		{
			PublicDefinitions.Add("IS_STEAM=1");
		}
		else
		{
			PublicDefinitions.Add("IS_STEAM=0");
		}
		
		/*
		// When using Python interpreter
		PublicDefinitions.Add("_Py_USE_GCC_BUILTIN_ATOMICS=0");
		PublicDefinitions.Add("__STDC_VERSION__=0");
		
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../Shooter/ThirdParty/pybind11/include"));
		
        string python_path = Path.Combine(ModuleDirectory, "../../Python313/");
        PrivateIncludePaths.Add(Path.Combine(python_path, "include"));
        PublicAdditionalLibraries.Add(Path.Combine(python_path, "libs", "python313.lib"));
		
		//PublicDelayLoadDLLs.Add("python313.dll");
		RuntimeDependencies.Add("$(BinaryOutputDir)/python313.dll", Path.Combine(python_path, "python313.dll"));
		*/
		
		//string pythonSitePackages = Path.Combine(python_path, "Lib", "site-packages");
		//RuntimeDependencies.Add(pythonSitePackages);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
