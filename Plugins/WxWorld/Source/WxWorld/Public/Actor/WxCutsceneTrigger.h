// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/WxSpawnableInterface.h"
#include "GameFramework/Actor.h"
#include "WxCutsceneTrigger.generated.h"

class ALevelSequenceActor;
class ULevelSequence;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxPromptWidgetComponent;

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence를 재생한다.
 * 재생 중에는 추가 상호작용이 무시된다.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxCutsceneTrigger : public AActor, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

	// IWxSpawnableInterface
#if WITH_EDITOR
	virtual UStreamableRenderAsset* GetEditorPreviewMesh() const override;
#endif

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxPromptWidgetComponent> PromptWidget;

	/** 재생할 Level Sequence 에셋 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<ULevelSequence> LevelSequence;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void HandleSequenceFinished();

	void CleanupSequenceActor();

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	bool bIsPlaying;
};
