// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeshOpsActor.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS(Abstract)
class PAKEXPORT_API AMeshOpsActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshOpsActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent) void ConstructionScript();

	UFUNCTION(BlueprintCallable) void AssignCutters(const TArray<UStaticMeshComponent*>& InCutters)
	{
		Cutters = InCutters;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComponent = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TArray<UStaticMeshComponent*> Cutters;

};
