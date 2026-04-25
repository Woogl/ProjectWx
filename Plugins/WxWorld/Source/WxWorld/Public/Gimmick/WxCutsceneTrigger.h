// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/WxInteractableActor.h"
#include "WxCutsceneTrigger.generated.h"

class ALevelSequenceActor;
class ULevelSequence;
class UStaticMeshComponent;

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence를 재생한다.
 * 재생 중에는 추가 상호작용이 무시된다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCutsceneTrigger : public AWxInteractableActor
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 재생할 Level Sequence 에셋 */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
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
