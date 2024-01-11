// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraComponent.h"
#include "PublicCineCameraComponent.generated.h"

UCLASS(HideCategories = (CameraSettings), HideFunctions = (SetFieldOfView, SetAspectRatio), Blueprintable, ClassGroup = Camera, meta = (BlueprintSpawnableComponent), Config = Engine)
class PAKEXPORTRUNTIME_API UPublicCineCameraComponent : public UCineCameraComponent
{
	GENERATED_BODY()

public:
	virtual void Serialize(FArchive& Ar) override { Super::Serialize(Ar); }
};
