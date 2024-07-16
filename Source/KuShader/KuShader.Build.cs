// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KuShader : ModuleRules
{
	public KuShader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
