// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ApplyGameplayEffect.generated.h"

class UGameplayEffect;

/**
 * 구간이 열려 있는 동안 지정한 GE를 소유자 ASC에 건다. 무적(Effect.Invincible)·퍼펙트가드(Effect.PerfectGuard) 판정 구간이 이걸로 열린다.
 *
 * 구간의 수명은 이 노티파이가 소유한다 — 시작에 걸고 끝에서 걷어낸다.
 * GE에 지속시간을 실어 스스로 만료시키면 애니메이션 시계와 GE 시계가 둘로 갈려, 재생 속도가 도중에 바뀌는 순간 구간이 애니메이션과 어긋난다.
 *
 * EffectClass는 지속시간이 없는 GE여야 한다 — Instant나 HasDuration을 지정하면 구간이 성립하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ApplyGameplayEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** 트랙에 클래스 이름이 뜨지 않으면 한 몽타주에 놓인 구간들을 눈으로 구분할 수 없다. */
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<UGameplayEffect> EffectClass;
};
