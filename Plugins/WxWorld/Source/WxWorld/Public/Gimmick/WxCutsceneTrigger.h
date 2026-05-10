// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxCutsceneTrigger.generated.h"

class ALevelSequenceActor;
class ULevelSequence;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 컷신 트리거.
 * 플레이어가 상호작용하면 지정된 Level Sequence를 재생한다.
 * 재생 중에는 추가 상호작용이 무시되고, 로컬 플레이어 폰의 입력(Move/Look/Ability)이 비활성화된다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxCutsceneTrigger : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxCutsceneTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	/** 재생할 Level Sequence 에셋 */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TObjectPtr<ULevelSequence> LevelSequence;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	UFUNCTION()
	void HandleSequenceFinished();

	void CleanupSequenceActor();

	/** 로컬 PC 의 소유 폰에 대해 입력을 토글한다. 컷신 시작/종료 시 호출. */
	void SetLocalPlayerInputEnabled(bool bEnabled);

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	bool bIsPlaying;
};
