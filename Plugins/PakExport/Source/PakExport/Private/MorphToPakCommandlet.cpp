// Fill out your copyright notice in the Description page of Project Settings.


#include "MorphToPakCommandlet.h"
#include "PakExportUtility.h"
#include "AssetRegistry/AssetRegistryModule.h"

int32 UMorphToPakCommandlet::Main(const FString& Params)
{
  Super::Main(Params);
  
  auto& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
  auto& AssetRegistry = AssetRegistryModule.Get();
  AssetRegistry.SearchAllAssets(true);
  while (AssetRegistry.IsLoadingAssets())
  {
    AssetRegistry.Tick(1.0f);
  }

  TArray<FString> OutTokens;
  TArray<FString> OutSwitches;
  TMap<FString, FString> OutParams;
  ParseCommandLine(*Params, OutTokens, OutSwitches, OutParams);

  UPakExportUtility::MakeBunchFromMorphs("/Game/T_plank_morph2", "/Game/mat_test/01/MS_01_Inst"
  ,"/Game/mat_test/03/MS_02_Inst1", "/Game/mat_test/04/MS_02_Inst1", "/Game/Meshes/StaticMesh"
  , { FMorphTargetData("Length", FCString::Atof(*OutParams["Length"]))
  , FMorphTargetData("Width", FCString::Atof(*OutParams["Width"]))
  , FMorphTargetData("Edge", FCString::Atof(*OutParams["Edge"])) }
  , FVector2D(FCString::Atof(*OutParams["SizeX"]), FCString::Atof(*OutParams["SizeY"]))
  , FCString::Atof(*OutParams["Gap"]), OutParams["Styling"], false);

  return 0;
}
