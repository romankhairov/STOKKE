// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MorphToPakCommandlet.generated.h"

/**
 * 
 */
UCLASS()
class PAKEXPORT_API UMorphToPakCommandlet : public UCommandlet
{
	GENERATED_BODY()
	
	virtual int32 Main(const FString& Params) override;
};
