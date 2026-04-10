// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawner.generated.h"

class UChildActorComponent;

UCLASS()
class WXWORLD_API AWxSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<AActor> ActorClass;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UChildActorComponent> PreviewChildActor;
#endif
};
