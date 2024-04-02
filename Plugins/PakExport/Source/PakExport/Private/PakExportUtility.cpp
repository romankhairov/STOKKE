// Fill out your copyright notice in the Description page of Project Settings.


#include "PakExportUtility.h"
#include "EditorUtilityLibrary.h"
#include "UObject/Object.h"
#include "AssetRegistry/AssetData.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Internationalization.h"
#include "Misc/ScopedSlowTask.h"
#include "AssetTools/Private/SPackageReportDialog.h"
#include "ISourceControlModule.h"
#include "SourceControlOperations.h"
#include "Logging/LogMacros.h"
#include "Logging/MessageLog.h"
#include "PakExport.h"
#include "EditorDirectories.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Misc/MessageDialog.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Templates/SharedPointer.h"
#include "EditorAssetLibrary.h"
#include "EditorStaticMeshLibrary.h"
#include "Engine/Blueprint.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/Base64.h"
#include "PakExportUtilityRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Animation/AnimSequence.h"
#include "Animation/MorphTarget.h"
#include "Components/SkinnedMeshComponent.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "RawMesh/Public/RawMesh.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EditorLevelLibrary.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshOpsActor.h"
#include "KismetProceduralMeshLibrary.h"
#include "Animation/SkeletalMeshActor.h"
#include "HAL/FileManagerGeneric.h"
#include "Kismet/KismetSystemLibrary.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "StaticMeshEditor/Public/StaticMeshEditorSubsystem.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#endif

#define LOCTEXT_NAMESPACE "PakExport"

static auto const Quotes{FString("\"")};

TArray<FAssetData> SelectedAssets;
TMap<FString, TTuple<FString, TTuple<UObject*, UObject*>>> SavedAssetsPaths;
TArray<FString> SimpleCollisionCreatedAssets;
bool Started = false;
bool MaterialsPakRequested = false;
bool CamerasPakRequested = false;
bool LevelSequencePakRequested = false;
bool Archived = false;

TMap<UObject*, TArray<FMeshDescription>> mesh_descs;
TArray<UObject*> AssetsToDelete;

void UPakExportUtility::AddAssets()
{
	if (!SetStarted(true)) return;
	if (!MigratePackages())
	{
		UE_LOG(LogPakExport, Error, TEXT("MigratePackages failed!"));
	}
}

void UPakExportUtility::CleanAssets()
{
	SetStarted(false);
}

void UPakExportUtility::Export2Pak()
{
	Export2Pak_Internal();
}

void UPakExportUtility::Export2PakArchived()
{
	Archived = true;
	Export2Paks();
	Archived = false;
}

void UPakExportUtility::Export2Paks()
{
	// Choose a destination folder
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	FString DestinationFile;
	FString DestinationDir;
	TArray<FString> DestinationFiles;
	if (ensure(DesktopPlatform))
	{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		const FString Title = LOCTEXT("MigrateToFolderTitle", "Choose a destination Content folder").ToString();
		bool bFolderAccepted = false;
		while (!bFolderAccepted)
		{
			const bool bFolderSelected = DesktopPlatform->SaveFileDialog(
				ParentWindowWindowHandle,
				Title,
				FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
				TEXT("Exported.pak"),
				TEXT("Pak Files (*.pak)|*.pak"),
				EFileDialogFlags::None,
				DestinationFiles
			);

			if (!bFolderSelected)
			{
				// User canceled, return
				UE_LOG(LogPakExport, Error, TEXT("User canceled, return!"));
				return;
			}

			DestinationFile = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DestinationFiles[0]);
			DestinationDir = FPaths::GetPath(DestinationFile);

			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationDir);
			FPaths::NormalizeFilename(DestinationFile);

			bFolderAccepted = true;
		}
	}
	else
	{
		// Not on a platform that supports desktop functionality
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoDesktopPlatform", "Error: This platform does not support a file dialog."));
		UE_LOG(LogPakExport, Error, TEXT("This platform does not support a file dialog!"));
		return;
	}

	for (const auto& Asset : UEditorUtilityLibrary::GetSelectedAssetData())
		Export2Pak_Internal({Asset}, DestinationDir);
}

void UPakExportUtility::Export2MaterialsPak()
{
	MaterialsPakRequested = true;
	Export2Pak();
}

void UPakExportUtility::Export2CamerasPak()
{
	CamerasPakRequested = true;
	Export2Pak();
}

void UPakExportUtility::Export2LevelSequencePak()
{
	LevelSequencePakRequested = true;
	Export2Pak_Internal();
}

void UPakExportUtility::MakeBunchFromMorphs(FString SourcePakageName, FString MaterialPakageName,
                                            FString SliceMaterialPakageName, FString SubstrateMaterialPakageName,
                                            FString DestinationPakageName, TArray<FMorphTargetData> MorphTargets,
                                            FVector2D Size, float Gap, FString Styling, bool OnlyCut)
{
	const auto& AssetRegistryModule = (FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))).Get();

#if ENGINE_MAJOR_VERSION >= 5
	const auto World = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>()->GetEditorWorld();
#else
	const auto World = UEditorLevelLibrary::GetEditorWorld();
#endif

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.bAllowDuringConstructionScript = true;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!OnlyCut)
	{
		//Skeletal morph to temp static mesh
		TArray<FAssetData> Assets;
		if (!AssetRegistryModule.GetAssetsByPackageName(FName(SourcePakageName), Assets))
		{
			return;
		}

		auto SkeletalMesh = Cast<USkeletalMesh>(Assets[0].GetAsset());

		TMap<FName, float> MorphTargetCurves;

		for (auto MorphTarget : SkeletalMesh->GetMorphTargets())
			for (auto& InMorphTarget : MorphTargets)
			{
				const auto MorphTargetName = MorphTarget->GetFName();
				if (MorphTargetName == InMorphTarget.Name)
					MorphTargetCurves.Emplace(MorphTargetName, InMorphTarget.Value);
			}

		auto PreviewMeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass()
			, FTransform(FVector(0.f, 0.f, 500.f))
			, ActorSpawnParameters);

		auto PreviewMeshComponent = NewObject<USkeletalMeshComponent>(PreviewMeshActor);
		PreviewMeshComponent->RegisterComponent();
		PreviewMeshComponent->SetCollisionProfileName("NoCollision");
		PreviewMeshComponent->SetSkeletalMesh(SkeletalMesh);

		for (const auto& MorphTargetCurve : MorphTargetCurves)
			PreviewMeshComponent->SetMorphTarget(MorphTargetCurve.Key, MorphTargetCurve.Value);

		constexpr auto TempSavePakageName = "/Game/Temp/Meshes/StaticMesh";

		ConvertMeshesToStaticMesh({ PreviewMeshComponent }, PreviewMeshComponent->GetComponentToWorld(), TempSavePakageName);

		TArray<FAssetData> TempSavedAssets;
		if (!AssetRegistryModule.GetAssetsByPackageName(FName(TempSavePakageName), TempSavedAssets))
		{
			return;
		}

		//EnableSimpleCollisions(TempSavedAssets);
		//UEditorAssetLibrary::SaveLoadedAsset(SaveAssets[0].GetAsset(), false);

		//Bunch of actors to static mesh
		const auto TempStaticMesh = Cast<UStaticMesh>(TempSavedAssets[0].GetAsset());
		SetAllowCPUAccess(World, TempStaticMesh);

		const auto BBX = TempStaticMesh->GetBounds().BoxExtent.X + Gap;
		const auto BBY = TempStaticMesh->GetBounds().BoxExtent.Y + Gap;

		TArray<AActor*> SpawnedActors;

		for (int32 x = 0; x < Size.X; ++x)
			for (int32 y = 0; y < Size.Y; ++y)
			{
				float Offset = 0.f;
				if (Styling == "50/50")
					Offset = (x % 2 == 0) ? BBY : 0;

				auto SpawnedStaticMeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass()
					, FTransform((FVector(1.f, 0.f, 0.f) * (float(x) * BBX * 2.f))
						+ (FVector(0.f, 1.f, 0.f) * (float(y) * BBY * -2.f + Offset)))
					, ActorSpawnParameters);
				auto SpawnedStaticMeshComponent = SpawnedStaticMeshActor->GetStaticMeshComponent();
				SpawnedStaticMeshComponent->SetCollisionProfileName("NoCollision");
				SpawnedStaticMeshComponent->SetStaticMesh(TempStaticMesh);
				SpawnedActors.Add(SpawnedStaticMeshActor);
			}

		ConvertActorsToStaticMesh(SpawnedActors, DestinationPakageName);

		UStaticMesh* SavedStaticMesh = nullptr;

		TArray<FAssetData> FinalSaveAssets;
		if (AssetRegistryModule.GetAssetsByPackageName(FName(DestinationPakageName), FinalSaveAssets))
		{
			//EnableSimpleCollisions(FinalSaveAssets);

			SavedStaticMesh = Cast<UStaticMesh>(FinalSaveAssets[0].GetAsset());

			TArray<UMaterialInterface*> Materials;
			int32 MaterialCount = 0;
			while (true)
			{
				TArray<FAssetData> MaterialAssets;
				AssetRegistryModule.GetAssetsByPackageName(
				FName(MaterialCount == 0 ? MaterialPakageName : (MaterialPakageName + FString::FromInt(MaterialCount))), MaterialAssets);
				if (MaterialAssets.Num() > 0)
				{
					Materials.Add(Cast<UMaterialInterface>(MaterialAssets[0].GetAsset()));
					++MaterialCount;
				}
				else
				{
					break;
				}
			}

			TArray<FStaticMaterial> StaticMaterials;
			for (int i = 0; i < SavedStaticMesh->GetNumSections(0); ++i)
				StaticMaterials.Add(FStaticMaterial(Materials[FMath::RandRange(0, Materials.Num() - 1)]));
			SavedStaticMesh->SetStaticMaterials(StaticMaterials);

			SetAllowCPUAccess(World, SavedStaticMesh);

			UEditorAssetLibrary::SaveLoadedAsset(SavedStaticMesh, false);
		}

		//Cleanup
		for (auto SpawnedActor : SpawnedActors)
			SpawnedActor->Destroy();

		//UEditorAssetLibrary::DeleteLoadedAsset(TempStaticMesh);
	}
	else
	{
		//Cut mesh
		TArray<FAssetData> FinalSaveAssets;
		if (!AssetRegistryModule.GetAssetsByPackageName(FName(DestinationPakageName), FinalSaveAssets))
		{
			return;
		}

		auto SavedStaticMesh = Cast<UStaticMesh>(FinalSaveAssets[0].GetAsset());

		//To Once section
		auto OneSectionActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass()
			, FTransform(FVector(0.f, 0.f, 500.f))
			, ActorSpawnParameters);

		auto OneSectionStaticComponent = NewObject<UStaticMeshComponent>(OneSectionActor);
		OneSectionStaticComponent->RegisterComponent();
		OneSectionStaticComponent->SetCollisionProfileName("NoCollision");
		OneSectionStaticComponent->SetStaticMesh(SavedStaticMesh);

		auto OneSectionProcComponent = NewObject<UProceduralMeshComponent>(OneSectionActor);
		OneSectionProcComponent->RegisterComponent();
		OneSectionProcComponent->SetCollisionProfileName("NoCollision");

		UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(OneSectionStaticComponent, 0, OneSectionProcComponent, false);

		constexpr auto TempProcSavePakageName = "/Game/Temp/Meshes/StaticMesh_Proc";

		ProceduralToStaticMesh(OneSectionProcComponent, TempProcSavePakageName);

		TArray<FAssetData> TempProcSavedAssets;
		AssetRegistryModule.GetAssetsByPackageName(FName(TempProcSavePakageName), TempProcSavedAssets);

		SavedStaticMesh = Cast<UStaticMesh>(TempProcSavedAssets[0].GetAsset());

		UEditorAssetLibrary::SaveLoadedAsset(SavedStaticMesh, false);

		auto MeshOpsActor = World->SpawnActor<AMeshOpsActor>(UPakExportDeveloperSettings::GetMeshOpsActorClass()
			, FTransform(FVector(0.f, 0.f, 500.f)), ActorSpawnParameters);

// 		for (auto Cutter : MeshOpsActor->Cutters)
// 			SetAllowCPUAccess(World, Cutter->GetStaticMesh());

		MeshOpsActor->StaticMeshComponent->SetStaticMesh(SavedStaticMesh);
		MeshOpsActor->ConstructionScript();

		const auto CutDestinationPakageName = DestinationPakageName + "_Cut";

		ProceduralToStaticMesh(MeshOpsActor->ProceduralMeshComponent, CutDestinationPakageName);

		TArray<FAssetData> CutFinalSaveAssets;
		if (AssetRegistryModule.GetAssetsByPackageName(FName(CutDestinationPakageName), CutFinalSaveAssets))
		{
			//EnableSimpleCollisions(CutFinalSaveAssets);

			auto CutFinalStaticMesh = Cast<UStaticMesh>(CutFinalSaveAssets[0].GetAsset());

			TArray<FAssetData> SliceMatAssets;
			AssetRegistryModule.GetAssetsByPackageName(FName(SliceMaterialPakageName), SliceMatAssets);
			const auto SliceMat = Cast<UMaterialInterface>(SliceMatAssets[0].GetAsset());

			auto StaticMaterials = CutFinalStaticMesh->GetStaticMaterials();
			StaticMaterials[StaticMaterials.Num() - 1] = (FStaticMaterial(SliceMat));
			CutFinalStaticMesh->SetStaticMaterials(StaticMaterials);

			UEditorAssetLibrary::SaveLoadedAsset(CutFinalStaticMesh, false);
		}

		//Cleanup
		MeshOpsActor->Destroy();
		OneSectionActor->Destroy();
	}
}

bool UPakExportUtility::SetStarted(bool Val)
{
	if (Val)
	{
		if (!Started)
		{
			if (!Prepare())
			{
				UE_LOG(LogPakExport, Error, TEXT("Prepare failed!"));
				return false;
			}
			Started = true;
		}
	}
	else
	{
		//restore saved assets
		for (const auto& SavedAssetPath : SavedAssetsPaths)
		{
			if (ensure(UEditorAssetLibrary::ConsolidateAssets(SavedAssetPath.Value.Value.Value, {SavedAssetPath.Value.Value.Key})))
			{
				UEditorAssetLibrary::DeleteAsset(SavedAssetPath.Key);
				UEditorAssetLibrary::RenameAsset(SavedAssetPath.Value.Key, SavedAssetPath.Key);
			}
		}

		for (const auto A : AssetsToDelete)
			UEditorAssetLibrary::DeleteLoadedAsset(A);
		
		SelectedAssets.Empty();
		SavedAssetsPaths.Empty();
		SimpleCollisionCreatedAssets.Empty();
#if PLATFORM_WINDOWS
		RunBat("Cleanup.bat");
#elif PLATFORM_MAC
		RunBat("Cleanup.sh");
#endif
		MaterialsPakRequested = false;
		CamerasPakRequested = false;
		LevelSequencePakRequested = false;
		Started = false;
	}
	return true;
}

bool UPakExportUtility::Prepare()
{
#if PLATFORM_WINDOWS
	return RunBat("UnZip.bat");
#elif PLATFORM_MAC
	return RunBat("UnZip.sh");
#endif
}

void UPakExportUtility::SaveAssetsCopy(const TArray<FAssetData>& InAssets)
{
	TArray<FName> PackageNames;
	PackageNames.Reserve(InAssets.Num());
	for (const auto& SelectedAsset : InAssets) PackageNames.Add(SelectedAsset.PackageName);

	TSet<FName> AllPackageNamesToMove;
	{
		FScopedSlowTask SlowTask(PackageNames.Num(), LOCTEXT("MigratePackages_GatheringDependencies", "Gathering Dependencies..."));
		SlowTask.MakeDialog();

		for (const auto& PackageName : PackageNames)
		{
			SlowTask.EnterProgressFrame();

			if (!AllPackageNamesToMove.Contains(PackageName))
			{
				TSet<FName> PackageNamesToMove;
				AllPackageNamesToMove.Add(PackageName);
				FString Path = PackageName.ToString();
				FString OriginalRootString;
				Path.RemoveFromStart(TEXT("/"));
				Path.Split("/", &OriginalRootString, &Path, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				OriginalRootString = TEXT("/") + OriginalRootString;
				RecursiveGetDependencies(PackageName, AllPackageNamesToMove, PackageNamesToMove, OriginalRootString);
			}
		}
	}

	const auto& AssetRegistryModule = (FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry"))).Get();
	for (const auto& PackageNamesToMove : AllPackageNamesToMove)
	{
		TArray<FAssetData> Assets;
		if (AssetRegistryModule.GetAssetsByPackageName(PackageNamesToMove, Assets))
			for (auto const& SelectedAsset : Assets)
				SaveAssetCopy(SelectedAsset);
	}
}

void UPakExportUtility::SaveAssetCopy(const FAssetData& InAsset)
{
	const auto Path{InAsset.PackagePath.ToString() + "/" + InAsset.AssetName.ToString()};
	constexpr auto SavedPrefix{"_Saved"};
	const auto SavedPath{Path + SavedPrefix};
	const auto Path2{Path.Replace(*FString(SavedPrefix), TEXT(""), ESearchCase::CaseSensitive)};
	if (!SavedAssetsPaths.Contains(Path) && !SavedAssetsPaths.Contains(Path2))
		if (const auto Duplicated = UEditorAssetLibrary::DuplicateAsset(Path, SavedPath))
		{
			TTuple<UObject*, UObject*> Objects(InAsset.GetAsset(), Duplicated);
			TTuple<FString, TTuple<UObject*, UObject*>> Payload(SavedPath, Objects);
			SavedAssetsPaths.Emplace(Path, Payload);
		}
}

void UPakExportUtility::SaveAssetCopy(const UObject* InAsset)
{
	const auto& AssetRegistryModule = (FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))).Get();
	
	const auto PackageName{InAsset->GetPathName().LeftChop(InAsset->GetName().Len() + 1)};
	
	TArray<FAssetData> Assets;
	if (AssetRegistryModule.GetAssetsByPackageName(FName(PackageName), Assets))
		for (const auto& Asset : Assets)
			SaveAssetCopy(Asset);
}

void UPakExportUtility::MaterialSlotsUnification(const TArray<FAssetData>& InAssets)
{
	for (const auto InAsset : InAssets)
	{
		const auto Asset = InAsset.GetAsset();

		if (const auto Mesh = Cast<UStaticMesh>(Asset))
		{
			SaveAssetCopy(InAsset);
			for (auto& Material : Mesh->GetStaticMaterials())
				Material.MaterialSlotName = FName(Material.MaterialSlotName.ToString() + "_" + Mesh->GetName());
		}
			

		if (auto const Mesh = Cast<USkeletalMesh>(Asset))
		{
			SaveAssetCopy(InAsset);
			for (auto& Material : Mesh->GetMaterials())
				Material.MaterialSlotName = FName(Material.MaterialSlotName.ToString() + "_" + Mesh->GetName());
		}
		
		if (const auto Blueprint = Cast<UBlueprint>(Asset))
		{
			for (const auto Node : Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)->
			                       SimpleConstructionScript->GetAllNodes())
			{
				if (const auto StaticMeshComponent = Cast<UStaticMeshComponent>(Node->ComponentTemplate))
					if (const auto StaticMesh = StaticMeshComponent->GetStaticMesh())
					{
						SaveAssetCopy(StaticMesh);
						for (auto& Material : StaticMesh->GetStaticMaterials())
							Material.MaterialSlotName = FName(Material.MaterialSlotName.ToString() + "_" + StaticMeshComponent->GetName());
					}
						

				if (const auto SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Node->ComponentTemplate))
					if (const auto SkeletalMesh = SkeletalMeshComponent->SkeletalMesh)
					{
						SaveAssetCopy(SkeletalMesh);
						for (auto& Material : SkeletalMesh->GetMaterials())
							Material.MaterialSlotName = FName(
								Material.MaterialSlotName.ToString() + "_" + SkeletalMeshComponent->GetName());
					}
			}
		}

		if (Asset->IsA(UWorld::StaticClass()))
		{
			const auto World = Cast<UWorld>(Asset);
			TArray<AActor*> Actors;
			for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
				Actors.Append(World->GetLevel(LevelIndex)->Actors);
			for (const auto Actor : Actors)
			{
				if (Actor->IsA<AStaticMeshActor>()
					|| Actor->IsA<ASkeletalMeshActor>()
					|| (((UObject*)Actor->GetClass())->IsA<UBlueprintGeneratedClass>()))
				{
					Actor->ForEachComponent<UMeshComponent, TFunction<void(UMeshComponent*)>>(false
						, [Actor](auto MeshComponent)
						{
							if (auto StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
								if (auto StaticMesh = StaticMeshComponent->GetStaticMesh())
								{
									SaveAssetCopy(StaticMesh);
									for (auto& Material : StaticMesh->GetStaticMaterials())
										Material.MaterialSlotName = FName(
											Material.MaterialSlotName.ToString() + "_" + Actor->GetName() + "." +
											StaticMeshComponent->GetName());
								}
							
							if (auto SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
								if (auto SkeletalMesh = SkeletalMeshComponent->SkeletalMesh)
								{
									SaveAssetCopy(SkeletalMesh);
									for (auto& Material : SkeletalMesh->GetMaterials())
										Material.MaterialSlotName = FName(
											Material.MaterialSlotName.ToString() + "_" + Actor->GetName() + "." +
											SkeletalMeshComponent->GetName());
								}
						});
				}
			}
		}
	}
}

bool UPakExportUtility::MigratePackages(const TArray<FAssetData>& InAssets/* = {}*/)
{
	// Get a list of package names for input into MigratePackages
	auto l_PreSelectedAssets = InAssets.Num() == 0 ? UEditorUtilityLibrary::GetSelectedAssetData() : InAssets;
	TArray<FAssetData> l_SelectedAssets;
#if ENGINE_MAJOR_VERSION >= 5
	l_SelectedAssets = l_PreSelectedAssets;
#else
	for (const auto& AssetData : l_PreSelectedAssets)
	{
		const auto Asset = AssetData.GetAsset();
		if (Asset->GetOutermost()->bIsCookedForEditor)
		{
			if (const auto StaticMesh = Cast<UStaticMesh>(Asset))
			{
				const auto srcPackagePath = FPackageName::ObjectPathToPackageName(StaticMesh->GetPathName());
				auto package = CreatePackage(*(FPackageName::GetLongPackagePath(srcPackagePath)
				+ "/_decooked/" + FPackageName::GetShortName(srcPackagePath)));
				auto dstAsset = DuplicateObject<UStaticMesh>(StaticMesh, package, *StaticMesh->GetName());
				AssetsToDelete.Add(dstAsset);
				
				const auto& LODResources = StaticMesh->GetRenderData()->LODResources;
				for (const auto& Lod : LODResources)
				{
					const auto Ind = &Lod - &LODResources[0];

					FMeshDescription mesh_desc;

					for (int32 i = 0; i < StaticMesh->GetNumUVChannels(Ind); ++i)
					{
						dstAsset->InsertUVChannel(Ind, i);
					}
	
					FStaticMeshAttributes attributes(mesh_desc);
					attributes.Register();
					
					auto positions = mesh_desc.VertexAttributes()
					.GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
					const auto NumVerts = Lod.VertexBuffers.PositionVertexBuffer.GetNumVertices();
					mesh_desc.ReserveNewVertices(NumVerts);
					mesh_desc.ReserveNewVertexInstances(NumVerts);
					positions = attributes.GetVertexPositions();
					auto normals = attributes.GetVertexInstanceNormals();
					auto uvs = attributes.GetVertexInstanceUVs();
					auto tangs = attributes.GetVertexInstanceTangents();
					TArray<FVertexInstanceID> vis;
					for (uint32 i = 0; i < NumVerts; ++i)
					{
						const auto v = mesh_desc.CreateVertex();
						const auto Id = FVertexInstanceID(i);
						vis.Add(mesh_desc.CreateVertexInstance(v));
						positions[FVertexID(i)] = Lod.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
						normals[Id] = Lod.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
						uvs[Id] = Lod.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
						tangs[Id] = Lod.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(i);
					}

					FPolygonGroupID polygon_group = mesh_desc.CreatePolygonGroup();
					const auto& IndexBuffer = Lod.IndexBuffer;

					TArray<int32> v3;
					v3.Reserve(3);
					for (int32 i = 0; i < IndexBuffer.GetNumIndices(); ++i)
					{
						v3.Add(IndexBuffer.GetIndex(i));
						if (v3.Num() == 3)
						{
							mesh_desc.CreateTriangle(polygon_group, {vis[v3[0]], vis[v3[1]], vis[v3[2]]});
							v3.Reset();
						}
					}

					if (auto Found = mesh_descs.Find(StaticMesh))
					{
						(*Found).Add(mesh_desc);
					}
					else
					{
						mesh_descs.Add(TTuple<UObject*, TArray<FMeshDescription>>(StaticMesh, TArray<FMeshDescription>{mesh_desc}));
					}
				}
	
				// At least one material must be added
				dstAsset->GetStaticMaterials() = StaticMesh->GetStaticMaterials();

				UStaticMesh::FBuildMeshDescriptionsParams mdParams;
				mdParams.bBuildSimpleCollision = true;

				// Build static mesh
				TArray<const FMeshDescription*> mesh_descs_p;

				if (const auto Found = mesh_descs.Find(StaticMesh))
					for (const auto& d : *Found)
						mesh_descs_p.Add(&d);
				
				dstAsset->BuildFromMeshDescriptions(mesh_descs_p, mdParams);
				
				UEditorAssetLibrary::SaveLoadedAsset(dstAsset, false);

				l_SelectedAssets.Add(dstAsset);
			}
			else
			{
				UE_LOG(LogPakExport, Log, TEXT("Unknown cooked asset!"));
			}
		}
		else
		{
			l_SelectedAssets.Add(AssetData);
		}
	}
#endif
	//return false;
	
	EnableSimpleCollisions(l_SelectedAssets);
	SelectedAssets.Append(l_SelectedAssets);
	TArray<FName> PackageNames;
	PackageNames.Reserve(l_SelectedAssets.Num());
	for (const auto& SelectedAsset : l_SelectedAssets) PackageNames.Add(SelectedAsset.PackageName);

	// Packages must be saved for the migration to work
	if (!FEditorFileUtils::SaveDirtyPackages(false, true, true, true))
	{
		UE_LOG(LogPakExport, Error, TEXT("Save pakages failed!"));
		return false;
	}

	// Form a full list of packages to move by including the dependencies of the supplied packages
	TSet<FName> AllPackageNamesToMove;
	{
		FScopedSlowTask SlowTask(PackageNames.Num(), LOCTEXT("MigratePackages_GatheringDependencies", "Gathering Dependencies..."));
		SlowTask.MakeDialog();

		for (const auto& PackageName : PackageNames)
		{
			TSet<FName> PackageNamesToMove;

			SlowTask.EnterProgressFrame();

			if (!AllPackageNamesToMove.Contains(PackageName))
			{
				AllPackageNamesToMove.Add(PackageName);
				FString Path = PackageName.ToString();
				FString OriginalRootString;
				Path.RemoveFromStart(TEXT("/"));
				Path.Split("/", &OriginalRootString, &Path, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				OriginalRootString = TEXT("/") + OriginalRootString;
				RecursiveGetDependencies(PackageName, AllPackageNamesToMove, PackageNamesToMove, OriginalRootString);
			}

			if (l_SelectedAssets[&PackageName - &PackageNames[0]].GetAsset()->IsA(UBlueprint::StaticClass()))
			{
				const auto& AssetRegistryModule = (FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))).Get();
				for (const auto& PackageNameToMove : PackageNamesToMove)
				{
					TArray<FAssetData> Assets;
					if (AssetRegistryModule.GetAssetsByPackageName(PackageNameToMove, Assets))
						EnableSimpleCollisions(Assets);
				}
			}
		}
	}

	// Confirm that there is at least one package to move
	if (AllPackageNamesToMove.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigratePackages_NoFilesToMove", "No files were found to move"));
		return false;
	}

	const FText ReportMessage = LOCTEXT("MigratePackagesReportTitle", "The following assets will be migrated to another content folder.");
	TSharedPtr<TArray<ReportPackageData>> ReportPackages = MakeShareable(new TArray<ReportPackageData>);
	for (auto PackageIt = AllPackageNamesToMove.CreateConstIterator(); PackageIt; ++PackageIt)
		ReportPackages.Get()->Add({ PackageIt->ToString(), true });
	MigratePackages_ReportConfirmed(ReportPackages);
	return true;
}

void UPakExportUtility::MigratePackages_ReportConfirmed(TSharedPtr<TArray<ReportPackageData>> PackageDataToMigrate)
{
	// Choose a destination folder
	const auto GameDir = FPaths::GameSourceDir();
	FString DestinationFolder{ GameDir + "../Plugins/PakExport/Utils/PakExport/Plugins/" + UPakExportRuntimeStatic::PakExportName + "/Content/"};

	bool bUserCanceled = false;

	// Copy all specified assets and their dependencies to the destination folder
	FScopedSlowTask SlowTask(2, LOCTEXT("MigratePackages_CopyingFiles", "Copying Files..."));
	SlowTask.MakeDialog();

	EAppReturnType::Type LastResponse = EAppReturnType::Yes;
	TArray<FString> SuccessfullyCopiedFiles;
	TArray<FString> SuccessfullyCopiedPackages;
	FString CopyErrors;

	SlowTask.EnterProgressFrame();
	{
		FScopedSlowTask LoopProgress(PackageDataToMigrate.Get()->Num());
		for (auto PackageDataIt = PackageDataToMigrate.Get()->CreateConstIterator(); PackageDataIt; ++PackageDataIt)
		{
			LoopProgress.EnterProgressFrame();
			if (!PackageDataIt->bShouldMigratePackage)
			{
				continue;
			}

			const FString& PackageName = PackageDataIt->Name;
			FString SrcFilename;
#if ENGINE_MAJOR_VERSION >= 5
			if (!FPackageName::DoesPackageExist(PackageName, &SrcFilename))
#else
			if (!FPackageName::DoesPackageExist(PackageName, nullptr, &SrcFilename))
#endif
			{
				const FText ErrorMessage = FText::Format(LOCTEXT("MigratePackages_PackageMissing", "{0} does not exist on disk."), FText::FromString(PackageName));
				UE_LOG(LogPakExport, Warning, TEXT("%s"), *ErrorMessage.ToString());
				CopyErrors += ErrorMessage.ToString() + LINE_TERMINATOR;
			}
			else if (SrcFilename.Contains(FPaths::EngineContentDir()))
			{
				const FString LeafName = SrcFilename.Replace(*FPaths::EngineContentDir(), TEXT("Engine/"));
				CopyErrors += FText::Format(LOCTEXT("MigratePackages_EngineContent", "Unable to migrate Engine asset {0}. Engine assets cannot be migrated."), FText::FromString(LeafName)).ToString() + LINE_TERMINATOR;
			}
			else
			{
				bool bFileOKToCopy = true;

				FString DestFilename = DestinationFolder;

				FString SubFolder;
				if (SrcFilename.Split(TEXT("/Content/"), nullptr, &SubFolder))
				{
					DestFilename += *SubFolder;
				}
				else
				{
					// Couldn't find Content folder in source path
					bFileOKToCopy = false;
				}

				if (bFileOKToCopy)
				{
					if (IFileManager::Get().Copy(*DestFilename, *SrcFilename) == COPY_OK)
					{
						SuccessfullyCopiedPackages.Add(PackageName);
						SuccessfullyCopiedFiles.Add(DestFilename);
					}
					else
					{
						UE_LOG(LogPakExport, Warning, TEXT("Failed to copy %s to %s while migrating assets"), *SrcFilename, *DestFilename);
						CopyErrors += SrcFilename + LINE_TERMINATOR;
					}
				}
			}
		}
	}

	FString SourceControlErrors;
	SlowTask.EnterProgressFrame();

	if (!bUserCanceled && SuccessfullyCopiedFiles.Num() > 0)
	{
		// attempt to add files to source control (this can quite easily fail, but if it works it is very useful)
		if (GetDefault<UEditorLoadingSavingSettings>()->bSCCAutoAddNewFiles)
		{
			if (ISourceControlModule::Get().IsEnabled())
			{
				ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
				if (SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SuccessfullyCopiedFiles) == ECommandResult::Failed)
				{
					FScopedSlowTask LoopProgress(SuccessfullyCopiedFiles.Num());

					for (auto FileIt(SuccessfullyCopiedFiles.CreateConstIterator()); FileIt; FileIt++)
					{
						LoopProgress.EnterProgressFrame();
						if (!SourceControlProvider.GetState(*FileIt, EStateCacheUsage::Use)->IsAdded())
						{
							SourceControlErrors += FText::Format(LOCTEXT("MigratePackages_SourceControlError", "{0} could not be added to source control"), FText::FromString(*FileIt)).ToString();
							SourceControlErrors += LINE_TERMINATOR;
						}
					}
				}
			}
		}
	}

	FMessageLog MigrateLog("PakExport");
	FText LogMessage = FText::FromString(TEXT("Content migration completed successfully!"));
	EMessageSeverity::Type Severity = EMessageSeverity::Info;
	if (CopyErrors.Len() > 0 || SourceControlErrors.Len() > 0)
	{
		FString ErrorMessage;
		Severity = EMessageSeverity::Error;
		if (CopyErrors.Len() > 0)
		{
			MigrateLog.NewPage(LOCTEXT("MigratePackages_CopyErrorsPage", "Copy Errors"));
			MigrateLog.Error(FText::FromString(*CopyErrors));
			ErrorMessage += FText::Format(LOCTEXT("MigratePackages_CopyErrors", "Copied {0} files. Some content could not be copied."), FText::AsNumber(SuccessfullyCopiedPackages.Num())).ToString();
		}
		if (SourceControlErrors.Len() > 0)
		{
			MigrateLog.NewPage(LOCTEXT("MigratePackages_SourceControlErrorsListPage", "Source Control Errors"));
			MigrateLog.Error(FText::FromString(*SourceControlErrors));
			ErrorMessage += LINE_TERMINATOR;
			ErrorMessage += LOCTEXT("MigratePackages_SourceControlErrorsList", "Some files reported source control errors.").ToString();
		}
		if (SuccessfullyCopiedPackages.Num() > 0)
		{
			MigrateLog.NewPage(LOCTEXT("MigratePackages_CopyErrorsSuccesslistPage", "Copied Successfully"));
			MigrateLog.Info(FText::FromString(*SourceControlErrors));
			ErrorMessage += LINE_TERMINATOR;
			ErrorMessage += LOCTEXT("MigratePackages_CopyErrorsSuccesslist", "Some files were copied successfully.").ToString();
			for (auto FileIt = SuccessfullyCopiedPackages.CreateConstIterator(); FileIt; ++FileIt)
			{
				if (FileIt->Len() > 0)
				{
					MigrateLog.Info(FText::FromString(*FileIt));
				}
			}
		}
		LogMessage = FText::FromString(ErrorMessage);
	}
	else if (!bUserCanceled)
	{
		MigrateLog.NewPage(LOCTEXT("MigratePackages_CompletePage", "Content migration completed successfully!"));
		for (auto FileIt = SuccessfullyCopiedPackages.CreateConstIterator(); FileIt; ++FileIt)
		{
			if (FileIt->Len() > 0)
			{
				MigrateLog.Info(FText::FromString(*FileIt));
			}
		}
	}
	MigrateLog.Notify(LogMessage, Severity, true);
}

bool UPakExportUtility::CookPak(const FString& InDestinationFile, const FString& CustomFileName)
{
	const auto EngineDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
	*(FPaths::EngineSourceDir() + "/../"));

	// Choose a destination folder
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	FString DestinationFile;
	FString DestinationDir;
	TArray<FString> DestinationFiles;

	if (!InDestinationFile.IsEmpty())
	{
		DestinationFile = InDestinationFile;
		DestinationDir = FPaths::GetPath(DestinationFile);
	}
	else
	{
		if (ensure(DesktopPlatform))
		{
			const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

			const FString Title = LOCTEXT("MigrateToFolderTitle", "Choose a destination Content folder").ToString();
			bool bFolderAccepted = false;
			while (!bFolderAccepted)
			{
				const bool bFolderSelected = DesktopPlatform->SaveFileDialog(
					ParentWindowWindowHandle,
					Title,
					FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
					TEXT("Exported.pak"),
					TEXT("Pak Files (*.pak)|*.pak"),
					EFileDialogFlags::None,
					DestinationFiles
				);

				if (!bFolderSelected)
				{
					// User canceled, return
					return false;
				}

				DestinationFile = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*DestinationFiles[0]);
				DestinationDir = FPaths::GetPath(DestinationFile);

				FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationDir);
				FPaths::NormalizeFilename(DestinationFile);

				if (!CustomFileName.IsEmpty())
				{
					DestinationFile = DestinationDir + FString("/") + CustomFileName + FString(".pak");
				}
				
				bFolderAccepted = true;
			}
		}
		else
		{
			// Not on a platform that supports desktop functionality
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoDesktopPlatform", "Error: This platform does not support a file dialog."));
			return false;
		}
	}

	const auto ArchivedTempDir = Archived ? UKismetSystemLibrary::GetProjectSavedDirectory() + "ArchivedTemp" : FString();

	if (!ArchivedTempDir.IsEmpty())
	{
		auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (PlatformFile.DirectoryExists(*ArchivedTempDir))
		{
			FFileManagerGeneric::Get().DeleteDirectory(*ArchivedTempDir, true, true);
		}

		if (!PlatformFile.DirectoryExists(*ArchivedTempDir))
		{
			FFileManagerGeneric::Get().MakeDirectory(*ArchivedTempDir, true);
		}
	}
	
	UPakExportUtilityRuntime::GenerateJsonsForAssets(SelectedAssets, DestinationFile, MaterialsPakRequested, CamerasPakRequested, LevelSequencePakRequested, ArchivedTempDir);

	if (CamerasPakRequested) return true;
	
	const auto ContentFolder{ IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
	*(FPaths::GameSourceDir() + "../Plugins/PakExport/Utils/PakExport/Plugins/" + UPakExportRuntimeStatic::PakExportName + "/Content/"))};
	
	//cook
	const auto FinalDestinationFile{ArchivedTempDir.IsEmpty() ? DestinationFile : ArchivedTempDir + "/asset.pak"};
	const auto FinalSourcePath{ArchivedTempDir.IsEmpty() ? FinalDestinationFile :  ArchivedTempDir + "/source"};
	
#if PLATFORM_WINDOWS
	const auto Res = RunBat("CookAndPak.bat\" " + Quotes + EngineDir + Quotes
	+ " " + Quotes + FinalDestinationFile + Quotes
	+ " " + "1>" + Quotes + DestinationDir + FString("/") + "CookAndPak.log" + Quotes + "2>&1");

	if (Res)
	{
		RunBat("PakE2Game.bat\" " + Quotes + ContentFolder + Quotes);
		
		RunBat("Zip.bat\" " + Quotes + FinalSourcePath + Quotes + " " + Quotes + ContentFolder + Quotes);
		
		if (!ArchivedTempDir.IsEmpty()) RunBat("Zip.bat\" " + Quotes + DestinationFile + Quotes + " " + Quotes + ArchivedTempDir + Quotes);
	}
	
	return Res;
#elif PLATFORM_MAC
	const auto Res = RunBat("CookAndPak.sh " + EngineDir  + " " + FinalDestinationFile);

	if (Res)
	{
		RunBat("PakE2Game.sh " + ContentFolder);
		
		RunBat("Zip.sh " + FinalSourcePath + " " + ContentFolder);

		if (!ArchivedTempDir.IsEmpty()) RunBat("Zip.sh " + DestinationFile + " " + ArchivedTempDir);
	}
	
	return Res;
#endif
}

bool UPakExportUtility::RunBat(const FString& BatFileAndParams)
{
#if PLATFORM_WINDOWS
	const auto BatFile = Quotes + Quotes + IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
		*(FPaths::GameSourceDir() + "../Plugins/PakExport/Scripts/" + BatFileAndParams)) + Quotes;
	if (system(TCHAR_TO_ANSI(*BatFile)) == 0)
	{
		UE_LOG(LogPakExport, Log, TEXT("%s success!"), *BatFileAndParams);
		return true;
	}
	else
	{
		UE_LOG(LogPakExport, Error, TEXT("%s failed!"), *BatFileAndParams);
		return false;
	}
#elif PLATFORM_MAC
	const auto BatFile = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
		*(FPaths::GameSourceDir() + "../Plugins/PakExport/Scripts/" + BatFileAndParams));
	if (system(TCHAR_TO_ANSI(*BatFile)) == 0)
	{
		UE_LOG(LogPakExport, Log, TEXT("%s success!"), *BatFileAndParams);
		return true;
	}
	else
	{
		UE_LOG(LogPakExport, Error, TEXT("%s failed!"), *BatFileAndParams);
		return false;
	}
#endif
}

void UPakExportUtility::EnableSimpleCollisions(const TArray<FAssetData>& Assets)
{
	for (auto const& SelectedAsset : Assets)
	{		
		const auto AssetName = SelectedAsset.GetFullName();
		if (!SimpleCollisionCreatedAssets.Contains(AssetName))
			if (auto StaticMesh = Cast<UStaticMesh>(SelectedAsset.GetAsset()))
			{
				SaveAssetCopy(SelectedAsset);
				
				SimpleCollisionCreatedAssets.Add(AssetName);
				if (!StaticMesh->GetBodySetup()) StaticMesh->CreateBodySetup();
				StaticMesh->GetBodySetup()->CollisionTraceFlag = CTF_UseSimpleAndComplex;
				StaticMesh->Build();
#if ENGINE_MAJOR_VERSION >= 5
				GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>()->SetConvexDecompositionCollisions(StaticMesh, 16, 16, 100000);
#else
				UEditorStaticMeshLibrary::SetConvexDecompositionCollisions(StaticMesh, 16, 16, 100000);
#endif
				UEditorAssetLibrary::SaveLoadedAsset(StaticMesh, false);
			}
	}
}

void UPakExportUtility::RecursiveGetDependencies(const FName& PackageName, TSet<FName>& AllDependencies, TSet<FName>& Dependencies, const FString& OriginalRoot)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FName> l_Dependencies;
	AssetRegistryModule.Get().GetDependencies(PackageName, l_Dependencies);

	for (auto DependsIt = l_Dependencies.CreateConstIterator(); DependsIt; ++DependsIt)
	{
		if (!AllDependencies.Contains(*DependsIt))
		{
			const bool bIsEnginePackage = (*DependsIt).ToString().StartsWith(TEXT("/Engine"));
			const bool bIsScriptPackage = (*DependsIt).ToString().StartsWith(TEXT("/Script"));
			// Skip all packages whose root is different than the source package list root
			const bool bIsInSamePackage = (*DependsIt).ToString().StartsWith(OriginalRoot);
			if (!bIsEnginePackage && !bIsScriptPackage && bIsInSamePackage)
			{
				AllDependencies.Add(*DependsIt);
				Dependencies.Add(*DependsIt);
				RecursiveGetDependencies(*DependsIt, AllDependencies, Dependencies, OriginalRoot);
			}
		}
	}
}

static void AddOrDuplicateMaterial(UMaterialInterface* InMaterialInterface, const FString& InPackageName, TArray<UMaterialInterface*>& OutMaterials)
{
	if (InMaterialInterface && !InMaterialInterface->GetOuter()->IsA<UPackage>())
	{
		// Convert runtime material instances to new concrete material instances
		// Create new package
		FString OriginalMaterialName = InMaterialInterface->GetName();
		FString MaterialPath = FPackageName::GetLongPackagePath(InPackageName) / OriginalMaterialName;
		FString MaterialName;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(MaterialPath, TEXT(""), MaterialPath, MaterialName);
		UPackage* MaterialPackage = CreatePackage(*MaterialPath);

		// Duplicate the object into the new package
		UMaterialInterface* NewMaterialInterface = DuplicateObject<UMaterialInterface>(InMaterialInterface, MaterialPackage, *MaterialName);
		NewMaterialInterface->SetFlags(RF_Public | RF_Standalone);

		if (UMaterialInstanceDynamic* MaterialInstanceDynamic = Cast<UMaterialInstanceDynamic>(NewMaterialInterface))
		{
			UMaterialInstanceDynamic* OldMaterialInstanceDynamic = CastChecked<UMaterialInstanceDynamic>(InMaterialInterface);
			MaterialInstanceDynamic->K2_CopyMaterialInstanceParameters(OldMaterialInstanceDynamic);
		}

		NewMaterialInterface->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(NewMaterialInterface);

		InMaterialInterface = NewMaterialInterface;
	}

	OutMaterials.Add(InMaterialInterface);
}
template <typename ComponentType>
static void ProcessMaterials(ComponentType* InComponent, const FString& InPackageName, TArray<UMaterialInterface*>& OutMaterials)
{
	const int32 NumMaterials = InComponent->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; MaterialIndex++)
	{
		UMaterialInterface* MaterialInterface = InComponent->GetMaterial(MaterialIndex);
		AddOrDuplicateMaterial(MaterialInterface, InPackageName, OutMaterials);
	}
}
struct FRawMeshTracker
{
	FRawMeshTracker()
		: bValidColors(false)
	{
		FMemory::Memset(bValidTexCoords, 0);
	}

	bool bValidTexCoords[MAX_MESH_TEXTURE_COORDS];
	bool bValidColors;
};
static bool IsValidSkinnedMeshComponent(USkinnedMeshComponent* InComponent)
{
	return InComponent && InComponent->MeshObject && InComponent->IsVisible();
}
static void SkinnedMeshToRawMeshes(USkinnedMeshComponent* InSkinnedMeshComponent, int32 InOverallMaxLODs, const FMatrix& InComponentToWorld, const FString& InPackageName, TArray<FRawMeshTracker>& OutRawMeshTrackers, TArray<FRawMesh>& OutRawMeshes, TArray<UMaterialInterface*>& OutMaterials)
{
	const int32 BaseMaterialIndex = OutMaterials.Num();

	// Export all LODs to raw meshes
	const int32 NumLODs = InSkinnedMeshComponent->GetNumLODs();

	for (int32 OverallLODIndex = 0; OverallLODIndex < InOverallMaxLODs; OverallLODIndex++)
	{
		int32 LODIndexRead = FMath::Min(OverallLODIndex, NumLODs - 1);

		FRawMesh& RawMesh = OutRawMeshes[OverallLODIndex];
		FRawMeshTracker& RawMeshTracker = OutRawMeshTrackers[OverallLODIndex];
		const int32 BaseVertexIndex = RawMesh.VertexPositions.Num();

		FSkeletalMeshLODInfo& SrcLODInfo = *(InSkinnedMeshComponent->SkeletalMesh->GetLODInfo(LODIndexRead));

		// Get the CPU skinned verts for this LOD
		TArray<FFinalSkinVertex> FinalVertices;
		InSkinnedMeshComponent->GetCPUSkinnedVertices(FinalVertices, LODIndexRead);
		
		FSkeletalMeshRenderData& SkeletalMeshRenderData = InSkinnedMeshComponent->MeshObject->GetSkeletalMeshRenderData();
		FSkeletalMeshLODRenderData& LODData = SkeletalMeshRenderData.LODRenderData[LODIndexRead];
		
		// Copy skinned vertex positions
		for (int32 VertIndex = 0; VertIndex < FinalVertices.Num(); ++VertIndex)
		{
#if ENGINE_MAJOR_VERSION >= 5
			RawMesh.VertexPositions.Add((FVector4f)InComponentToWorld.TransformPosition((FVector)FinalVertices[VertIndex].Position));
#else
			RawMesh.VertexPositions.Add(InComponentToWorld.TransformPosition(FinalVertices[VertIndex].Position));
#endif
		}

		const uint32 NumTexCoords = FMath::Min(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(), (uint32)MAX_MESH_TEXTURE_COORDS);
		const int32 NumSections = LODData.RenderSections.Num();
		FRawStaticIndexBuffer16or32Interface& IndexBuffer = *LODData.MultiSizeIndexContainer.GetIndexBuffer();

		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			const FSkelMeshRenderSection& SkelMeshSection = LODData.RenderSections[SectionIndex];
			if (InSkinnedMeshComponent->IsMaterialSectionShown(SkelMeshSection.MaterialIndex, LODIndexRead))
			{
				// Build 'wedge' info
				const int32 NumWedges = SkelMeshSection.NumTriangles * 3;
				for (int32 WedgeIndex = 0; WedgeIndex < NumWedges; WedgeIndex++)
				{
					const int32 VertexIndexForWedge = IndexBuffer.Get(SkelMeshSection.BaseIndex + WedgeIndex);

					RawMesh.WedgeIndices.Add(BaseVertexIndex + VertexIndexForWedge);

#if ENGINE_MAJOR_VERSION >= 5
					const FFinalSkinVertex& SkinnedVertex = FinalVertices[VertexIndexForWedge];
					const FVector3f TangentX = (FVector4f)InComponentToWorld.TransformVector(SkinnedVertex.TangentX.ToFVector());
					const FVector3f TangentZ = (FVector4f)InComponentToWorld.TransformVector(SkinnedVertex.TangentZ.ToFVector());
					const FVector4 UnpackedTangentZ = SkinnedVertex.TangentZ.ToFVector4();
					const FVector3f TangentY = (TangentZ ^ TangentX).GetSafeNormal() * UnpackedTangentZ.W;
#else
					const FFinalSkinVertex& SkinnedVertex = FinalVertices[VertexIndexForWedge];
					const FVector TangentX = InComponentToWorld.TransformVector(SkinnedVertex.TangentX.ToFVector());
					const FVector TangentZ = InComponentToWorld.TransformVector(SkinnedVertex.TangentZ.ToFVector());
					const FVector4 UnpackedTangentZ = SkinnedVertex.TangentZ.ToFVector4();
					const FVector TangentY = (TangentZ ^ TangentX).GetSafeNormal() * UnpackedTangentZ.W;
#endif

					RawMesh.WedgeTangentX.Add(TangentX);
					RawMesh.WedgeTangentY.Add(TangentY);
					RawMesh.WedgeTangentZ.Add(TangentZ);

					for (uint32 TexCoordIndex = 0; TexCoordIndex < MAX_MESH_TEXTURE_COORDS; TexCoordIndex++)
					{
						if (TexCoordIndex >= NumTexCoords)
						{
							RawMesh.WedgeTexCoords[TexCoordIndex].AddDefaulted();
						}
						else
						{
							RawMesh.WedgeTexCoords[TexCoordIndex].Add(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndexForWedge, TexCoordIndex));
							RawMeshTracker.bValidTexCoords[TexCoordIndex] = true;
						}
					}

					if (LODData.StaticVertexBuffers.ColorVertexBuffer.IsInitialized())
					{
						RawMesh.WedgeColors.Add(LODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(VertexIndexForWedge));
						RawMeshTracker.bValidColors = true;
					}
					else
					{
						RawMesh.WedgeColors.Add(FColor::White);
					}
				}

				int32 MaterialIndex = SkelMeshSection.MaterialIndex;
				// use the remapping of material indices if there is a valid value
				if (SrcLODInfo.LODMaterialMap.IsValidIndex(SectionIndex) && SrcLODInfo.LODMaterialMap[SectionIndex] != INDEX_NONE)
				{
					MaterialIndex = FMath::Clamp<int32>(SrcLODInfo.LODMaterialMap[SectionIndex], 0, InSkinnedMeshComponent->SkeletalMesh->GetMaterials().Num());
				}

				// copy face info
				for (uint32 TriIndex = 0; TriIndex < SkelMeshSection.NumTriangles; TriIndex++)
				{
					RawMesh.FaceMaterialIndices.Add(BaseMaterialIndex + MaterialIndex);
					RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
				}
			}
		}
	}

	ProcessMaterials<USkinnedMeshComponent>(InSkinnedMeshComponent, InPackageName, OutMaterials);
}
static bool IsValidStaticMeshComponent(UStaticMeshComponent* InComponent)
{
	return InComponent && InComponent->GetStaticMesh() && InComponent->GetStaticMesh()->GetRenderData() && InComponent->IsVisible();
}
static void StaticMeshToRawMeshes(UStaticMeshComponent* InStaticMeshComponent, int32 InOverallMaxLODs, const FMatrix& InComponentToWorld, const FString& InPackageName, TArray<FRawMeshTracker>& OutRawMeshTrackers, TArray<FRawMesh>& OutRawMeshes, TArray<UMaterialInterface*>& OutMaterials)
{
	const int32 BaseMaterialIndex = OutMaterials.Num();

	const int32 NumLODs = InStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num();

	for (int32 OverallLODIndex = 0; OverallLODIndex < InOverallMaxLODs; OverallLODIndex++)
	{
		int32 LODIndexRead = FMath::Min(OverallLODIndex, NumLODs - 1);

		FRawMesh& RawMesh = OutRawMeshes[OverallLODIndex];
		FRawMeshTracker& RawMeshTracker = OutRawMeshTrackers[OverallLODIndex];
		const FStaticMeshLODResources& LODResource = InStaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[LODIndexRead];
		const int32 BaseVertexIndex = RawMesh.VertexPositions.Num();

		for (int32 VertIndex = 0; VertIndex < LODResource.GetNumVertices(); ++VertIndex)
		{
#if ENGINE_MAJOR_VERSION >= 5
			RawMesh.VertexPositions.Add(FVector4f(InComponentToWorld.TransformPosition((FVector)LODResource.VertexBuffers.PositionVertexBuffer.VertexPosition((uint32)VertIndex))));
#else
			RawMesh.VertexPositions.Add(InComponentToWorld.TransformPosition(LODResource.VertexBuffers.PositionVertexBuffer.VertexPosition((uint32)VertIndex)));
#endif
		}

		const FIndexArrayView IndexArrayView = LODResource.IndexBuffer.GetArrayView();
		const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
		const int32 NumTexCoords = FMath::Min(StaticMeshVertexBuffer.GetNumTexCoords(), (uint32)MAX_MESH_TEXTURE_COORDS);
		const int32 NumSections = LODResource.Sections.Num();

		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			const FStaticMeshSection& StaticMeshSection = LODResource.Sections[SectionIndex];

			const int32 NumIndices = StaticMeshSection.NumTriangles * 3;
			for (int32 IndexIndex = 0; IndexIndex < NumIndices; IndexIndex++)
			{
				int32 Index = IndexArrayView[StaticMeshSection.FirstIndex + IndexIndex];
				RawMesh.WedgeIndices.Add(BaseVertexIndex + Index);
#if ENGINE_MAJOR_VERSION >= 5
				RawMesh.WedgeTangentX.Add(FVector4f(InComponentToWorld.TransformVector(FVector(StaticMeshVertexBuffer.VertexTangentX(Index)))));
				RawMesh.WedgeTangentY.Add(FVector4f(InComponentToWorld.TransformVector(FVector(StaticMeshVertexBuffer.VertexTangentY(Index)))));
				RawMesh.WedgeTangentZ.Add(FVector4f(InComponentToWorld.TransformVector(FVector(StaticMeshVertexBuffer.VertexTangentZ(Index)))));
#else
				RawMesh.WedgeTangentX.Add(InComponentToWorld.TransformVector(StaticMeshVertexBuffer.VertexTangentX(Index)));
				RawMesh.WedgeTangentY.Add(InComponentToWorld.TransformVector(StaticMeshVertexBuffer.VertexTangentY(Index)));
				RawMesh.WedgeTangentZ.Add(InComponentToWorld.TransformVector(StaticMeshVertexBuffer.VertexTangentZ(Index)));
#endif

				for (int32 TexCoordIndex = 0; TexCoordIndex < MAX_MESH_TEXTURE_COORDS; TexCoordIndex++)
				{
					if (TexCoordIndex >= NumTexCoords)
					{
						RawMesh.WedgeTexCoords[TexCoordIndex].AddDefaulted();
					}
					else
					{
						RawMesh.WedgeTexCoords[TexCoordIndex].Add(StaticMeshVertexBuffer.GetVertexUV(Index, TexCoordIndex));
						RawMeshTracker.bValidTexCoords[TexCoordIndex] = true;
					}
				}

				if (LODResource.VertexBuffers.ColorVertexBuffer.IsInitialized())
				{
					RawMesh.WedgeColors.Add(LODResource.VertexBuffers.ColorVertexBuffer.VertexColor(Index));
					RawMeshTracker.bValidColors = true;
				}
				else
				{
					RawMesh.WedgeColors.Add(FColor::White);
				}
			}

			// copy face info
			for (uint32 TriIndex = 0; TriIndex < StaticMeshSection.NumTriangles; TriIndex++)
			{
				RawMesh.FaceMaterialIndices.Add(BaseMaterialIndex + StaticMeshSection.MaterialIndex);
				RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
			}
		}
	}

	ProcessMaterials<UStaticMeshComponent>(InStaticMeshComponent, InPackageName, OutMaterials);
}

UStaticMesh* UPakExportUtility::ConvertMeshesToStaticMesh(const TArray<UMeshComponent*>& InMeshComponents, const FTransform& InRootTransform, const FString& InPackageName)
{
	UStaticMesh* StaticMesh = nullptr;

	// Build a package name to use
	FString MeshName;
	FString PackageName;
	if (InPackageName.IsEmpty())
	{
		FString NewNameSuggestion = FString(TEXT("StaticMesh"));
		FString PackageNameSuggestion = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageNameSuggestion, TEXT(""), PackageNameSuggestion, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
			.DefaultAssetPath(FText::FromString(PackageNameSuggestion));

		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			// Get the full name of where we want to create the mesh asset.
			PackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			MeshName = FPackageName::GetLongPackageAssetName(PackageName);

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MeshName.IsEmpty())
			{
				// Use the defaults that were already generated.
				PackageName = PackageNameSuggestion;
				MeshName = *Name;
			}
		}
	}
	else
	{
		PackageName = InPackageName;
		MeshName = *FPackageName::GetLongPackageAssetName(PackageName);
	}

	if (!PackageName.IsEmpty() && !MeshName.IsEmpty())
	{
		TArray<FRawMesh> RawMeshes;
		TArray<UMaterialInterface*> Materials;

		TArray<FRawMeshTracker> RawMeshTrackers;

		FMatrix WorldToRoot = InRootTransform.ToMatrixWithScale().Inverse();

		// first do a pass to determine the max LOD level we will be combining meshes into
		int32 OverallMaxLODs = 0;
		for (UMeshComponent* MeshComponent : InMeshComponents)
		{
			USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(MeshComponent);
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);

			if (IsValidSkinnedMeshComponent(SkinnedMeshComponent))
			{
				OverallMaxLODs = FMath::Max(SkinnedMeshComponent->MeshObject->GetSkeletalMeshRenderData().LODRenderData.Num(), OverallMaxLODs);
			}
			else if (IsValidStaticMeshComponent(StaticMeshComponent))
			{
				OverallMaxLODs = FMath::Max(StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num(), OverallMaxLODs);
			}
		}

		// Resize raw meshes to accommodate the number of LODs we will need
		RawMeshes.SetNum(OverallMaxLODs);
		RawMeshTrackers.SetNum(OverallMaxLODs);

		// Export all visible components
		for (UMeshComponent* MeshComponent : InMeshComponents)
		{
			FMatrix ComponentToWorld = MeshComponent->GetComponentTransform().ToMatrixWithScale() * WorldToRoot;

			USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(MeshComponent);
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);

			if (IsValidSkinnedMeshComponent(SkinnedMeshComponent))
			{
				SkinnedMeshToRawMeshes(SkinnedMeshComponent, OverallMaxLODs, ComponentToWorld, PackageName, RawMeshTrackers, RawMeshes, Materials);
			}
			else if (IsValidStaticMeshComponent(StaticMeshComponent))
			{
				StaticMeshToRawMeshes(StaticMeshComponent, OverallMaxLODs, ComponentToWorld, PackageName, RawMeshTrackers, RawMeshes, Materials);
			}
		}

		uint32 MaxInUseTextureCoordinate = 0;

		// scrub invalid vert color & tex coord data
		check(RawMeshes.Num() == RawMeshTrackers.Num());
		for (int32 RawMeshIndex = 0; RawMeshIndex < RawMeshes.Num(); RawMeshIndex++)
		{
			if (!RawMeshTrackers[RawMeshIndex].bValidColors)
			{
				RawMeshes[RawMeshIndex].WedgeColors.Empty();
			}

			for (uint32 TexCoordIndex = 0; TexCoordIndex < MAX_MESH_TEXTURE_COORDS; TexCoordIndex++)
			{
				if (!RawMeshTrackers[RawMeshIndex].bValidTexCoords[TexCoordIndex])
				{
					RawMeshes[RawMeshIndex].WedgeTexCoords[TexCoordIndex].Empty();
				}
				else
				{
					// Store first texture coordinate index not in use
					MaxInUseTextureCoordinate = FMath::Max(MaxInUseTextureCoordinate, TexCoordIndex);
				}
			}
		}

		// Check if we got some valid data.
		bool bValidData = false;
		for (FRawMesh& RawMesh : RawMeshes)
		{
			if (RawMesh.IsValidOrFixable())
			{
				bValidData = true;
				break;
			}
		}

		if (bValidData)
		{
			// Then find/create it.
			UPackage* Package = CreatePackage(*PackageName);
			check(Package);

			// Create StaticMesh object
			StaticMesh = NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone);
			StaticMesh->InitResources();

			StaticMesh->SetLightingGuid();

			// Determine which texture coordinate map should be used for storing/generating the lightmap UVs
			const uint32 LightMapIndex = FMath::Min(MaxInUseTextureCoordinate + 1, (uint32)MAX_MESH_TEXTURE_COORDS - 1);

			// Add source to new StaticMesh
			for (FRawMesh& RawMesh : RawMeshes)
			{
				if (RawMesh.IsValidOrFixable())
				{
					FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
					SrcModel.BuildSettings.bRecomputeNormals = false;
					SrcModel.BuildSettings.bRecomputeTangents = false;
					SrcModel.BuildSettings.bRemoveDegenerates = true;
					SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
					SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
					SrcModel.BuildSettings.bGenerateLightmapUVs = true;
					SrcModel.BuildSettings.SrcLightmapIndex = 0;
					SrcModel.BuildSettings.DstLightmapIndex = LightMapIndex;
					SrcModel.SaveRawMesh(RawMesh);
				}
			}

			// Copy materials to new mesh 
			for (UMaterialInterface* Material : Materials)
			{
				StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material));
			}

			//Set the Imported version before calling the build
			StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

			// Set light map coordinate index to match DstLightmapIndex
			StaticMesh->SetLightMapCoordinateIndex(LightMapIndex);

			// setup section info map
			for (int32 RawMeshLODIndex = 0; RawMeshLODIndex < RawMeshes.Num(); RawMeshLODIndex++)
			{
				const FRawMesh& RawMesh = RawMeshes[RawMeshLODIndex];
				TArray<int32> UniqueMaterialIndices;
				for (int32 MaterialIndex : RawMesh.FaceMaterialIndices)
				{
					UniqueMaterialIndices.AddUnique(MaterialIndex);
				}

				int32 SectionIndex = 0;
				for (int32 UniqueMaterialIndex : UniqueMaterialIndices)
				{
					StaticMesh->GetSectionInfoMap().Set(RawMeshLODIndex, SectionIndex, FMeshSectionInfo(UniqueMaterialIndex));
					SectionIndex++;
				}
			}
			StaticMesh->GetOriginalSectionInfoMap().CopyFrom(StaticMesh->GetSectionInfoMap());

			// Build mesh from source
			StaticMesh->Build(false);
			StaticMesh->PostEditChange();

			StaticMesh->MarkPackageDirty();

			// Notify asset registry of new asset
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}
	}

	return StaticMesh;
}

void GetSkinnedAndStaticMeshComponentsFromActors(const TArray<AActor*> InActors, TArray<UMeshComponent*>& OutMeshComponents)
{
	for (AActor* Actor : InActors)
	{
		// add all components from this actor
		TInlineComponentArray<UMeshComponent*> ActorComponents(Actor);
		for (UMeshComponent* ActorComponent : ActorComponents)
		{
			if (ActorComponent->IsA(USkinnedMeshComponent::StaticClass()) || ActorComponent->IsA(UStaticMeshComponent::StaticClass()))
			{
				OutMeshComponents.AddUnique(ActorComponent);
			}
		}

		// add all attached actors
		TArray<AActor*> AttachedActors;
		Actor->GetAttachedActors(AttachedActors);
		for (AActor* AttachedActor : AttachedActors)
		{
			TInlineComponentArray<UMeshComponent*> AttachedActorComponents(AttachedActor);
			for (UMeshComponent* AttachedActorComponent : AttachedActorComponents)
			{
				if (AttachedActorComponent->IsA(USkinnedMeshComponent::StaticClass()) || AttachedActorComponent->IsA(UStaticMeshComponent::StaticClass()))
				{
					OutMeshComponents.AddUnique(AttachedActorComponent);
				}
			}
		}
	}
}

void UPakExportUtility::ConvertActorsToStaticMesh(const TArray<AActor*> InActors, FString DestinationPakageName)
{
	TArray<UMeshComponent*> MeshComponents;

	GetSkinnedAndStaticMeshComponentsFromActors(InActors, MeshComponents);

	auto GetActorRootTransform = [](AActor* InActor)
	{
		FTransform RootTransform(FTransform::Identity);
		if (ACharacter* Character = Cast<ACharacter>(InActor))
		{
			RootTransform = Character->GetTransform();
			RootTransform.SetLocation(RootTransform.GetLocation() - FVector(0.0f, 0.0f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		}
		else
		{
			// otherwise just use the actor's origin
			RootTransform = InActor->GetTransform();
		}

		return RootTransform;
	};

	// now pick a root transform
	FTransform RootTransform(FTransform::Identity);
	if (InActors.Num() == 1)
	{
		RootTransform = GetActorRootTransform(InActors[0]);
	}
	else
	{
		// multiple actors use the average of their origins, with Z being the min of all origins. Rotation is identity for simplicity
		FVector Location(FVector::ZeroVector);
		float MinZ = FLT_MAX;
		for (AActor* Actor : InActors)
		{
			FTransform ActorTransform(GetActorRootTransform(Actor));
			Location += ActorTransform.GetLocation();
			MinZ = FMath::Min(ActorTransform.GetLocation().Z, MinZ);
		}
		Location /= (float)InActors.Num();
		Location.Z = MinZ;

		RootTransform.SetLocation(Location);
	}

	UStaticMesh* StaticMesh = ConvertMeshesToStaticMesh(MeshComponents, RootTransform, DestinationPakageName);
}

TMap<UMaterialInterface*, FPolygonGroupID> BuildMaterialMap(UProceduralMeshComponent* ProcMeshComp, FMeshDescription& MeshDescription)
{
	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials;
	const int32 NumSections = ProcMeshComp->GetNumSections();
	UniqueMaterials.Reserve(NumSections);

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface* Material = ProcMeshComp->GetMaterial(SectionIdx);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		if (!UniqueMaterials.Contains(Material))
		{
			FPolygonGroupID NewPolygonGroup = MeshDescription.CreatePolygonGroup();
			UniqueMaterials.Add(Material, NewPolygonGroup);
			PolygonGroupNames[NewPolygonGroup] = Material->GetFName();
		}
	}
	return UniqueMaterials;
}
FMeshDescription BuildMeshDescription(UProceduralMeshComponent* ProcMeshComp)
{
	FMeshDescription MeshDescription;

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	AttributeGetter.Register();
#if ENGINE_MAJOR_VERSION >= 5
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	TVertexAttributesRef<FVector3f> VertexPositions = AttributeGetter.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Tangents = AttributeGetter.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector3f> Normals = AttributeGetter.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4f> Colors = AttributeGetter.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2f> UVs = AttributeGetter.GetVertexInstanceUVs();
#else
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	TVertexAttributesRef<FVector> VertexPositions = AttributeGetter.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector> Tangents = AttributeGetter.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector> Normals = AttributeGetter.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4> Colors = AttributeGetter.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2D> UVs = AttributeGetter.GetVertexInstanceUVs();
#endif

	// Materials to apply to new mesh
	const int32 NumSections = ProcMeshComp->GetNumSections();
	int32 VertexCount = 0;
	int32 VertexInstanceCount = 0;
	int32 PolygonCount = 0;

	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials = BuildMaterialMap(ProcMeshComp, MeshDescription);
	TArray<FPolygonGroupID> PolygonGroupForSection;
	PolygonGroupForSection.Reserve(NumSections);

	// Calculate the totals for each ProcMesh element type
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		VertexCount += ProcSection->ProcVertexBuffer.Num();
		VertexInstanceCount += ProcSection->ProcIndexBuffer.Num();
		PolygonCount += ProcSection->ProcIndexBuffer.Num() / 3;
	}
	MeshDescription.ReserveNewVertices(VertexCount);
	MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
	MeshDescription.ReserveNewPolygons(PolygonCount);
	MeshDescription.ReserveNewEdges(PolygonCount * 2);
#if ENGINE_MAJOR_VERSION >= 5
	UVs.SetNumChannels(4);
#else
	UVs.SetNumIndices(4);
#endif

	// Create the Polygon Groups
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface* Material = ProcMeshComp->GetMaterial(SectionIdx);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		FPolygonGroupID* PolygonGroupID = UniqueMaterials.Find(Material);
		check(PolygonGroupID != nullptr);
		PolygonGroupForSection.Add(*PolygonGroupID);
	}


	// Add Vertex and VertexInstance and polygon for each section
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		FPolygonGroupID PolygonGroupID = PolygonGroupForSection[SectionIdx];
		// Create the vertex
		int32 NumVertex = ProcSection->ProcVertexBuffer.Num();
		TMap<int32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(NumVertex);
		for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
		{
			FProcMeshVertex& Vert = ProcSection->ProcVertexBuffer[VertexIndex];
			const FVertexID VertexID = MeshDescription.CreateVertex();
#if ENGINE_MAJOR_VERSION >= 5
			VertexPositions[VertexID] = (FVector3f)Vert.Position;
#else
			VertexPositions[VertexID] = Vert.Position;
#endif
			VertexIndexToVertexID.Add(VertexIndex, VertexID);
		}
		// Create the VertexInstance
		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		int32 NumTri = NumIndices / 3;
		TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
		IndiceIndexToVertexInstanceID.Reserve(NumVertex);
		for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
		{
			const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
			const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
			const FVertexInstanceID VertexInstanceID =
				MeshDescription.CreateVertexInstance(VertexID);
			IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);

			FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[VertexIndex];
#if ENGINE_MAJOR_VERSION >= 5
			Tangents[VertexInstanceID] = (FVector3f)ProcVertex.Tangent.TangentX;
			Normals[VertexInstanceID] = (FVector3f)ProcVertex.Normal;
#else
			Tangents[VertexInstanceID] = ProcVertex.Tangent.TangentX;
			Normals[VertexInstanceID] = ProcVertex.Normal;
#endif
			BinormalSigns[VertexInstanceID] =
				ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f;

			Colors[VertexInstanceID] = FLinearColor(ProcVertex.Color);

#if ENGINE_MAJOR_VERSION >= 5
			UVs.Set(VertexInstanceID, 0, FVector2f(ProcVertex.UV0));
			UVs.Set(VertexInstanceID, 1, FVector2f(ProcVertex.UV1));
			UVs.Set(VertexInstanceID, 2, FVector2f(ProcVertex.UV2));
			UVs.Set(VertexInstanceID, 3, FVector2f(ProcVertex.UV3));
#else
			UVs.Set(VertexInstanceID, 0, ProcVertex.UV0);
			UVs.Set(VertexInstanceID, 1, ProcVertex.UV1);
			UVs.Set(VertexInstanceID, 2, ProcVertex.UV2);
			UVs.Set(VertexInstanceID, 3, ProcVertex.UV3);
#endif
		}

		// Create the polygons for this section
		for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
		{
			FVertexID VertexIndexes[3];
			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);

			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;
				const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
				VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
				VertexInstanceIDs[CornerIndex] =
					IndiceIndexToVertexInstanceID[IndiceIndex];
			}

			// Insert a polygon into the mesh
			MeshDescription.CreatePolygon(PolygonGroupID, VertexInstanceIDs);
		}
	}
	return MeshDescription;
}

void UPakExportUtility::ProceduralToStaticMesh(UProceduralMeshComponent* ProceduralMeshComponentconst, const FString& InPackageName)
{
	FString NewNameSuggestion = FString(TEXT("ProcMesh"));
	FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
	FString Name;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

	// Get the full name of where we want to create the physics asset.
	FString UserPackageName = InPackageName;
	FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

	// Check if the user inputed a valid asset name, if they did not, give it the generated default name
	if (MeshName == NAME_None)
	{
		// Use the defaults that were already generated.
		UserPackageName = PackageName;
		MeshName = *Name;
	}

	FMeshDescription MeshDescription = BuildMeshDescription(ProceduralMeshComponentconst);

	// If we got some valid data.
	if (MeshDescription.Polygons().Num() == 0)
	{
		return;
	}

	// Then find/create it.
	UPackage* Package = CreatePackage(*UserPackageName);
	check(Package);

	// Create StaticMesh object
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
	StaticMesh->InitResources();

	StaticMesh->SetLightingGuid();

	// Add source to new StaticMesh
	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bRemoveDegenerates = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.SrcLightmapIndex = 0;
	SrcModel.BuildSettings.DstLightmapIndex = 1;
	StaticMesh->CreateMeshDescription(0, MoveTemp(MeshDescription));
	StaticMesh->CommitMeshDescription(0);

	//// SIMPLE COLLISION
	if (!ProceduralMeshComponentconst->bUseComplexAsSimpleCollision)
	{
		StaticMesh->CreateBodySetup();
		UBodySetup* NewBodySetup = StaticMesh->GetBodySetup();
		NewBodySetup->BodySetupGuid = FGuid::NewGuid();
		NewBodySetup->AggGeom.ConvexElems = ProceduralMeshComponentconst->ProcMeshBodySetup->AggGeom.ConvexElems;
		NewBodySetup->bGenerateMirroredCollision = false;
		NewBodySetup->bDoubleSidedGeometry = true;
		NewBodySetup->CollisionTraceFlag = CTF_UseDefault;
		NewBodySetup->CreatePhysicsMeshes();
	}

	//// MATERIALS
	TSet<UMaterialInterface*> UniqueMaterials;
	const int32 NumSections = ProceduralMeshComponentconst->GetNumSections();
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProceduralMeshComponentconst->GetProcMeshSection(SectionIdx);
		UMaterialInterface* Material = ProceduralMeshComponentconst->GetMaterial(SectionIdx);
		UniqueMaterials.Add(Material);
	}
	// Copy materials to new mesh
	for (auto* Material : UniqueMaterials)
	{
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material));
	}

	//Set the Imported version before calling the build
	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

	// Build mesh from source
	StaticMesh->Build(false);
	StaticMesh->PostEditChange();

	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(StaticMesh);
}

void UPakExportUtility::SetAllowCPUAccess(UObject* WorldContextObject, UStaticMesh* StaticMesh)
{
	constexpr auto Allow = true;
	if (StaticMesh == nullptr) { return; }
	if (StaticMesh->bAllowCPUAccess != Allow) {
		StaticMesh->Modify();
		StaticMesh->bAllowCPUAccess = Allow;
		StaticMesh->PostEditChange();
		TArray<UPackage*> Packages;
		Packages.Add(StaticMesh->GetOutermost());
		UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
	}
}

void UPakExportUtility::Export2Pak_Internal(const TArray<FAssetData>& Assets /*= {}*/, const FString& DestinationDir/* = {}*/)
{
	if (!SetStarted(true)) return;

	const auto PreSelectedAssets = Assets.Num() == 0 ? UEditorUtilityLibrary::GetSelectedAssetData() : Assets;
	
	//MaterialSlotsUnification(PreSelectedAssets);
	
	if (MigratePackages(Assets))
	{
		if (!CookPak(DestinationDir.IsEmpty() ? FString() : DestinationDir + "/" + Assets[0].AssetName.ToString() + ".pak"
			, PreSelectedAssets.Num() > 1 ? FString{} : PreSelectedAssets[0].AssetName.ToString()))
		{
			UE_LOG(LogPakExport, Error, TEXT("Cook failed!"));
		}
	}
	else
	{
		UE_LOG(LogPakExport, Error, TEXT("MigratePackages failed!"));
	}
	SetStarted(false);
}

UClass* UPakExportDeveloperSettings::GetMeshOpsActorClass()
{
	return Cast<UClass>(
		StaticLoadObject(UClass::StaticClass(), nullptr
			, *GetDefault<UPakExportDeveloperSettings>()->MeshOpsActorClass.ToSoftObjectPath().ToString()));
}
