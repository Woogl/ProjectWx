// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawner.generated.h"

class UBillboardComponent;

UCLASS(meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<AActor> SpawnableActorClass;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif

#if WITH_EDITOR
private:
	void UpdateSpriteFromSpawnableClass();
#endif
};
