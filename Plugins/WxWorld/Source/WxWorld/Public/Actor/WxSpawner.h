// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawner.generated.h"

class UArrowComponent;
class UBillboardComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS(meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxWorld.WxSpawnableInterface", AllowAbstract = "false"))
	TSubclassOf<AActor> SpawnableActorClass;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void UpdateEditorPreviewFromSpawnableClass();
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> PreviewSkeletalMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PreviewStaticMeshComponent;
#endif
};
