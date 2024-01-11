// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PakExport : ModuleRules
{
	public PakExport(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				//"PakExport/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				//"PakExport/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PhysicsCore"
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"MessageLog",
				"PackagesDialog",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"EditorStyle",
				"PropertyEditor",
				"LevelEditor",
				// ... add private dependencies that you statically link with here ...	
				"AssetTools",
				"Projects",
				"Blutility",
				"DesktopPlatform",
				"Json",
				"JsonUtilities",
				"EditorScriptingUtilities",
				"CinematicCamera",
				"PakExportRuntime",
				"RawMesh",
				"ProceduralMeshComponent",
				"MeshDescription",
				"StaticMeshDescription",
				"DeveloperSettings",
#if UE_5_1_OR_LATER
				"StaticMeshEditor",
#else
				"GeometricObjects",
#endif
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
				
				"MessageLog",
				"PackagesDialog",
			}
			);
	}
}
