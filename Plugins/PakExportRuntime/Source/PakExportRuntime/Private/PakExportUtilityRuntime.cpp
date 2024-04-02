// Fill out your copyright notice in the Description page of Project Settings.


#include "PakExportUtilityRuntime.h"
#include "GameFramework/SpringArmComponent.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "CineCameraComponent.h"
#include "PublicCineCameraComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "CineCameraActor.h"
#include "LevelSequence.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "PakLoaderLibrary.h"
#include "PakExportRuntime.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#if ENGINE_MAJOR_VERSION > 4
#include "Engine/SkinnedAssetCommon.h"
#endif
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetSystemLibrary.h"

constexpr auto SlotDelimiter{"::"};

void UPakExportUtilityRuntime::GenerateJsonsForAssets(const TArray<FString>& InAssets, const FString& DestinationFile)
{
	TArray<FAssetData> Assets;
	const auto& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	for (const auto Asset : InAssets)
	{
		const auto PakName = "/" + Asset.Replace(TEXT("Content"), WCHAR_TO_TCHAR(UPakExportRuntimeStatic::PakExportName), ESearchCase::CaseSensitive)
		.Replace(TEXT(".uasset"), TEXT(""), ESearchCase::CaseSensitive);
		const auto Obj = UPakLoaderLibrary::GetPakFileObject(PakName);
		const auto PackageName = Obj->GetPackage()->GetName();
		AssetRegistryModule.GetAssetsByPackageName(*(PackageName), Assets);
	}

	GenerateJsonsForAssets(Assets, DestinationFile);
}

void UPakExportUtilityRuntime::GenerateJsonsForAssets(const TArray<FAssetData>& InAssets,
                                                      const FString& DestinationFile,
                                                      bool MaterialsPakRequested/* = false*/,
                                                      bool CamerasPakRequested/* = false*/,
                                                      bool LevelSequencePakRequested/* = false*/,
                                                      const FString& ArchivedTempDir/* = {}*/)
{
	TArray<const FAssetData*> SelectedMaterialsAssets;
	TArray<FAssetPakJson> LevelSequencePakDatas;

	const auto DestinationDir = FPaths::GetPath(DestinationFile);
	const auto PakFilePath = FPaths::GetCleanFilename(DestinationFile); //TODO: make relative path instead simple filename
	const auto SelectedPakFileNameBase = FPaths::GetBaseFilename(DestinationFile);
	for (const auto& SelectedAsset : InAssets)
	{
		const auto Asset = SelectedAsset.GetAsset();
		const auto Name = Asset->GetName();
		const auto AssetName = SelectedAsset.PackageName.ToString().RightChop(6);
		//material
		if (Asset->IsA(UMaterialInterface::StaticClass()))
		{
			if (!MaterialsPakRequested)
			{
				if (!ArchivedTempDir.IsEmpty())
				{
					FMaterialExportData MaterialExportData{};

					MaterialExportData.assetPath = AssetName;

					FString JsonString;
					if (FJsonObjectConverter::UStructToJsonObjectString(MaterialExportData, JsonString))
					{
						FFileHelper::SaveStringToFile(JsonString, *(ArchivedTempDir + "/config.json"));
					}
					else
					{
						UE_LOG(LogPakExportRuntime, Error, TEXT("Generate material pak json failed!"));
					}
				}
				else
				{
					FSetMaterialPayloadJson MaterialPakData;
					MaterialPakData.materialName = Name;
					MaterialPakData.materialPak.pakFilePath = PakFilePath;
					MaterialPakData.materialPak.assetName = AssetName;
					//save JSON to file near pak file
					FString JsonString;
					if (FJsonObjectConverter::UStructToJsonObjectString(MaterialPakData, JsonString))
					{
						FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + Name + ".json"));
					}
					else
					{
						UE_LOG(LogPakExportRuntime, Error, TEXT("Generate material pak json failed!"));
					}
				}
			}
			else
			{
				SelectedMaterialsAssets.Add(&SelectedAsset);
			}
		}
		//level
		else if (Asset->IsA(UWorld::StaticClass()))
		{
			if (!CamerasPakRequested)
			{
				if (!ArchivedTempDir.IsEmpty())
				{
					FEnvironmentExportData EnvironmentExportData{};

					EnvironmentExportData.assetPath = AssetName;

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
								, [&EnvironmentExportData, Actor](const auto MeshComponent)
								{
									for (const auto& MaterialName : MeshComponent->GetMaterialSlotNames())
										EnvironmentExportData.slots.Add(
											UKismetSystemLibrary::GetDisplayName(Actor) + SlotDelimiter +
											UKismetSystemLibrary::GetDisplayName(MeshComponent) + SlotDelimiter +
											MaterialName.ToString());
								});
						}
					}

					FString JsonString;
					if (FJsonObjectConverter::UStructToJsonObjectString(EnvironmentExportData, JsonString))
					{
						FFileHelper::SaveStringToFile(JsonString, *(ArchivedTempDir + "/config.json"));
					}
					else
					{
						UE_LOG(LogPakExportRuntime, Error, TEXT("Generate level pak json failed!"));
					}
				}
				else
				{
					FLoadLevelPayloadJson LevelPakData;
					LevelPakData.levelName = Name;
					LevelPakData.levelPak.pakFilePath = PakFilePath;
					LevelPakData.levelPak.assetName = AssetName;

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
								, [&LevelPakData](const auto MeshComponent)
								{
									for (const auto& MaterialName : MeshComponent->GetMaterialSlotNames())
										LevelPakData.slots.Add(MaterialName.ToString());
								});
						}
					}

					//save JSON to file near pak file
					FString JsonString;
					if (FJsonObjectConverter::UStructToJsonObjectString(LevelPakData, JsonString))
					{
						FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + Name + ".json"));
					}
					else
					{
						UE_LOG(LogPakExportRuntime, Error, TEXT("Generate level pak json failed!"));
					}
				}
			}
			else
			{
				const auto World = Cast<UWorld>(Asset);
				TArray<AActor*> Actors;
				for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
					Actors.Append(World->GetLevel(LevelIndex)->Actors);
				for (const auto Actor : Actors)
					if (const auto CameraActor = Cast<ACineCameraActor>(Actor))
					{
						FString JsonString;
						if (FJsonObjectConverter::UStructToJsonObjectString(
							GenerateCameraData(CameraActor), JsonString))
						{
							FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/")
								                              + SelectedPakFileNameBase + "_" + Name + "_" + CameraActor
								                              ->GetName() + ".json"));
						}
						else
						{
							UE_LOG(LogPakExportRuntime, Error, TEXT("Generate cameras pak json failed!"));
						}
					}
			}
		}
		//product
		else if (Asset->IsA(UStaticMesh::StaticClass())
			|| Asset->IsA(UBlueprint::StaticClass())
			|| Asset->IsA(USkeletalMesh::StaticClass()))
		{
			if (!ArchivedTempDir.IsEmpty())
			{
				FProductExportData ProductExportData{};

				ProductExportData.assetPath = AssetName;

				if (const auto Mesh = Cast<UStaticMesh>(Asset))
					for (const auto& Material : Mesh->GetStaticMaterials())
						ProductExportData.slots.Add(Material.MaterialSlotName.ToString());

				if (const auto Mesh = Cast<USkeletalMesh>(Asset))
					for (const auto& Material : Mesh->GetMaterials())
						ProductExportData.slots.Add(Material.MaterialSlotName.ToString());

				if (const auto Blueprint = Cast<UBlueprint>(Asset))
				{
					for (const auto Node : Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)->
					                       SimpleConstructionScript->GetAllNodes())
					{
						if (const auto Mesh = Cast<UMeshComponent>(Node->ComponentTemplate))
							for (const auto& Slot : Mesh->GetMaterialSlotNames())
								ProductExportData.slots.Add(
									UKismetSystemLibrary::GetDisplayName(Mesh) + SlotDelimiter + Slot.ToString());
					}
				}

				FString JsonString;
				if (FJsonObjectConverter::UStructToJsonObjectString(ProductExportData, JsonString))
				{
					FFileHelper::SaveStringToFile(JsonString, *(ArchivedTempDir + "/config.json"));
				}
				else
				{
					UE_LOG(LogPakExportRuntime, Error, TEXT("Generate product pak json failed!"));
				}
			}
			else
			{
				FSelectProductPayloadJson ProductPakData;
				ProductPakData.productName = Name;
				ProductPakData.meshPak.pakFilePath = PakFilePath;
				ProductPakData.meshPak.initialState.meshName = AssetName;

				if (const auto Mesh = Cast<UStaticMesh>(Asset))
					for (const auto& Material : Mesh->GetStaticMaterials())
						ProductPakData.meshPak.initialState.slots.Add(Material.MaterialSlotName.ToString());

				if (const auto Mesh = Cast<USkeletalMesh>(Asset))
				{
					for (const auto& Material : Mesh->GetMaterials())
						ProductPakData.meshPak.initialState.slots.Add(Material.MaterialSlotName.ToString());

					for (const auto& Morph : Mesh->K2_GetAllMorphTargetNames())
						ProductPakData.meshPak.initialState.morphs.AddUnique(Morph);
				}

				//collect animations
				FPakJson AnimationsPakPathData;
				AnimationsPakPathData.pakFilePath = PakFilePath;
				if (const auto Blueprint = Cast<UBlueprint>(Asset))
				{
					TArray<USkeleton*> Skeletons;
					for (const auto Node : Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)->
					                       SimpleConstructionScript->GetAllNodes())
					{
						const auto ComponentTemplate = Node->ComponentTemplate;

						if (const auto Mesh = Cast<UMeshComponent>(ComponentTemplate))
							for (const auto& Slot : Mesh->GetMaterialSlotNames())
								ProductPakData.meshPak.initialState.slots.Add(Slot.ToString());

						if (const auto SkeletalMesh = Cast<USkeletalMeshComponent>(ComponentTemplate))
							Skeletons.Emplace(SkeletalMesh->SkeletalMesh->GetSkeleton());
					}

					const auto& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(
						TEXT("AssetRegistry")).Get();
					TArray<FName> Dependencies;
					AssetRegistryModule.GetDependencies(*Blueprint->GetPackage()->GetName(), Dependencies);
					for (const auto& Dependency : Dependencies)
					{
						TArray<FAssetData> Assets;
						AssetRegistryModule.GetAssetsByPackageName(Dependency, Assets);
						for (const auto& A : Assets)
							if (const auto AnimAsset = Cast<UAnimSequence>(A.GetAsset()))
								if (Skeletons.Contains(AnimAsset->GetSkeleton()))
									ProductPakData.meshPak.initialState.animations.Add(
										A.PackageName.ToString().RightChop(6));
					}
					ProductPakData.animationsPak = AnimationsPakPathData;
				}

				FString JsonString;
				if (FJsonObjectConverter::UStructToJsonObjectString(ProductPakData, JsonString))
				{
					FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + Name + ".json"));
				}
				else
				{
					UE_LOG(LogPakExportRuntime, Error, TEXT("Generate product pak json failed!"));
				}
			}
		}
		//LevelSequence
		else if (Asset->IsA(ULevelSequence::StaticClass()))
		{
			FAssetPakJson LevelSequencePakData;
			LevelSequencePakData.pakFilePath = PakFilePath;
			LevelSequencePakData.assetName = AssetName;
			
			if (!LevelSequencePakRequested)
			{
				FInitSequencePayloadJson InitSequencePayloadJson;
				InitSequencePayloadJson.paks.Add(LevelSequencePakData);
				
				FString JsonString;
				if (FJsonObjectConverter::UStructToJsonObjectString(InitSequencePayloadJson, JsonString))
				{
					FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + Name + ".json"));
				}
				else
				{
					UE_LOG(LogPakExportRuntime, Error, TEXT("Generate Level Sequence pak json failed!"));
				}
			}
			else
			{
				LevelSequencePakDatas.Emplace(MoveTemp(LevelSequencePakData));
			}
		}
		else
		{
			UE_LOG(LogPakExportRuntime, Error, TEXT("Unknow asset type for pak null!"));
		}
	}

	//make level sequence pak JSON
	if (LevelSequencePakRequested)
	{
		FInitSequencePayloadJson InitSequencePayloadJson;
		InitSequencePayloadJson.paks = MoveTemp(LevelSequencePakDatas);

		FString JsonString;
		if (FJsonObjectConverter::UStructToJsonObjectString(InitSequencePayloadJson, JsonString))
		{
			FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + SelectedPakFileNameBase + ".json"));
		}
		else
		{
			UE_LOG(LogPakExportRuntime, Error, TEXT("Generate Level Sequences pak json failed!"));
		}
	}
	
	//make material pak JSON
	if (MaterialsPakRequested)
	{
		FMaterialsData MaterialsData;
		FMaterialsAssetData MaterialsAssetData;
		MaterialsAssetData.title = SelectedPakFileNameBase;
		MaterialsAssetData.sku = SelectedPakFileNameBase;
		MaterialsAssetData.pakFilePath = PakFilePath;
		MaterialsAssetData.id = ""; //TODO: fill
		MaterialsAssetData.previewPath = ""; //TODO: fill
		FMaterialPropertyData MaterialPropertyData;
		for (const auto SelectedMaterialsAssetPtr : SelectedMaterialsAssets)
		{
			const auto lable{ SelectedMaterialsAssetPtr->PackageName.ToString().RightChop(6) };
			MaterialPropertyData.variants.Emplace(lable, lable, FHex());
		}
		MaterialsAssetData.property = MoveTemp(MaterialPropertyData);
		MaterialsData.data = MoveTemp(MaterialsAssetData);

		FString JsonString;
		if (FJsonObjectConverter::UStructToJsonObjectString(MaterialsData, JsonString))
		{
			FFileHelper::SaveStringToFile(JsonString, *(DestinationDir + FString("/") + SelectedPakFileNameBase + ".json"));
		}
		else
		{
			UE_LOG(LogPakExportRuntime, Error, TEXT("Generate material pak json failed!"));
		}
	}
}

FString UPakExportUtilityRuntime::GetSlotDelimiter()
{
	return SlotDelimiter;
}

FApplyCameraPresetPayloadJson UPakExportUtilityRuntime::GenerateCameraData(AActor* CameraActor, float FocusOffset/* = 0.f*/)
{
	FApplyCameraPresetPayloadJson CameraData;
	const auto CopyCameraActor = DuplicateObject<AActor>(CameraActor, CameraActor->GetOuter());
	const auto OrigCameraComp = CameraActor->FindComponentByClass<UCineCameraComponent>();

	CopyCameraActor->SetActorTransform(CameraActor->GetTransform());
	CameraData.location = CopyCameraActor->GetActorLocation();
	if (auto SpringArm = Cast<USpringArmComponent>(OrigCameraComp->GetAttachParent()))
		CameraData.rotation = SpringArm->GetComponentRotation();
	else
		CameraData.rotation = CameraActor->GetActorRotation();
	if (const auto Arm = CopyCameraActor->FindComponentByClass<USpringArmComponent>())
		CameraData.armLength = Arm->TargetArmLength;
	TArray<uint8> Bytes;
	FMemoryWriter MemoryWriter(Bytes, true);
	FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
	const auto CameraComp = CopyCameraActor->FindComponentByClass<UCineCameraComponent>();
	CameraData.aperture = CameraComp->CurrentAperture;
	CameraData.focalLength = CameraComp->CurrentFocalLength;
	CameraData.focusOffset = FocusOffset;
	auto CopyCameraComp = NewObject<UPublicCineCameraComponent>(CameraComp->GetOuter());
	UEngine::FCopyPropertiesForUnrelatedObjectsParams CopyPropertiesForUnrelatedObjectsParams;
	CopyPropertiesForUnrelatedObjectsParams.bDoDelta = false;
	UEngine::CopyPropertiesForUnrelatedObjects(CameraComp, CopyCameraComp, CopyPropertiesForUnrelatedObjectsParams);
	CopyCameraComp->Serialize(Ar);
	CameraData.object = FBase64::Encode(Bytes);
	CameraData.fov = CopyCameraComp->FieldOfView;
	return CameraData;
}
