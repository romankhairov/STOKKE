// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommandsHelpers.h"
#include "PakExportUtilityRuntime.generated.h"

//JSON data structs

USTRUCT()
struct FHex
{
	GENERATED_BODY()
public:
	FHex() {}
	FHex(int32 R, int32 G, int32 B) : R(R), G(G), B(B) {}
	int32 R{0};
	int32 G{0};
	int32 B{0};
};

USTRUCT()
struct FMaterialVariantData
{
	GENERATED_BODY()
public:
	FMaterialVariantData() {}
	FMaterialVariantData(const FString& Lable, const FString& Name, const FHex& Hex)
	: label(Lable), name(Name) { SetHex(Hex); }
	void SetHex(const FHex& Hex)
	{
		hex = "rgb("
		+ FString::FromInt(Hex.R) + ","
		+ FString::FromInt(Hex.G) + ","
		+ FString::FromInt(Hex.B) + ")";
	}

	UPROPERTY() FString label;
	UPROPERTY() FString name;
	
protected:
	UPROPERTY() FString hex { "rgb(0,0,0)" };
};

USTRUCT()
struct FMaterialPropertyData
{
	GENERATED_BODY()
public:
	UPROPERTY() TArray<FMaterialVariantData> variants;
};

USTRUCT()
struct FMaterialsAssetData
{
	GENERATED_BODY()
public:
	UPROPERTY() FString title;
	UPROPERTY() FString sku;
	UPROPERTY() FString pakFilePath;
	UPROPERTY() FString id;
	UPROPERTY() FString previewPath;
	UPROPERTY() FMaterialPropertyData property;
	UPROPERTY() EAssetType type{EAssetType::material};
};

USTRUCT()
struct FMaterialsData
{
	GENERATED_BODY()
public:
	UPROPERTY() FMaterialsAssetData data;
};

USTRUCT(BlueprintType)
struct FMorphTargetData
{
	GENERATED_BODY()
public:
	FMorphTargetData() {}
	FMorphTargetData(FName Name, float Value) : Name(Name), Value(Value) {}
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName	Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value{};
};

USTRUCT()
struct FExportData
{
	GENERATED_BODY()
public:
	FExportData()
	{
		const auto EngineVersion = FEngineVersion::Current();
		unrealVersion = FString::FromInt(EngineVersion.GetMajor())
		+ "." + FString::FromInt(EngineVersion.GetMinor())
		+ "." + FString::FromInt(EngineVersion.GetPatch());
	}
	
protected:
	UPROPERTY() EAssetType type{};
	UPROPERTY() FString unrealVersion{};

public:
	UPROPERTY() FString assetPath{};
};

USTRUCT()
struct FMaterialableExportData : public FExportData
{
	GENERATED_BODY()
public:
	UPROPERTY() TArray<FString> slots{};
};

USTRUCT()
struct FProductExportData : public FMaterialableExportData
{
	GENERATED_BODY()
public:
	FProductExportData()
	{
		type = EAssetType::product;
	}
};

USTRUCT()
struct FEnvironmentExportData : public FMaterialableExportData
{
	GENERATED_BODY()
public:
	FEnvironmentExportData()
	{
		type = EAssetType::environment;
	}
};

USTRUCT()
struct FMaterialExportData : public FExportData
{
	GENERATED_BODY()
public:
	FMaterialExportData()
	{
		type = EAssetType::material;
	}
};

/**
 * 
 */
UCLASS()
class PAKEXPORTRUNTIME_API UPakExportUtilityRuntime : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "PakExport")
	static void GenerateJsonsForAssets(const TArray<FString>& InAssets, const FString& DestinationFile);

	static FApplyCameraPresetPayloadJson GenerateCameraData(AActor* CameraActor, float FocusOffset = 0.f);
	static void GenerateJsonsForAssets(const TArray<FAssetData>& InAssets, const FString& DestinationFile,
	                                   bool MaterialsPakRequested = false, bool CamerasPakRequested = false,
	                                   bool LevelSequencePakRequested = false, const FString& Archived = {});

public:
	UFUNCTION(BlueprintPure) static FString GetSlotDelimiter();
};
