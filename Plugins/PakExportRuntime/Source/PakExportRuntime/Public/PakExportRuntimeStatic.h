// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PakExportRuntimeStatic.generated.h"

/**
 * 
 */
UCLASS()
class PAKEXPORTRUNTIME_API UPakExportRuntimeStatic : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = PakExport) static FString GetPakExportName() { return PakExportName; }
	UFUNCTION(BlueprintPure, Category = PakExport) static FString GetPakExportMountPath() { return PakExportMountPath; }
	
	static constexpr auto PakExportName{"PakE"};
	static constexpr auto PakExportMountPath{"../../../PakExport/Plugins/"};
	
	
	
};
