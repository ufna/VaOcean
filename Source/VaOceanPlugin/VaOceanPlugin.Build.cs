// Copyright 2014 Vladimir Alyamkin. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class VaOceanPlugin : ModuleRules
	{
        public VaOceanPlugin(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] { 
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"VaOceanPlugin/Private",
                    // ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "RenderCore",
                    "ShaderCore",
                    "RHI"
                    // ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}