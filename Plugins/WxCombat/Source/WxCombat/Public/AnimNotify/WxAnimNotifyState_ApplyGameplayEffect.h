// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ApplyGameplayEffect.generated.h"

class UGameplayEffect;

/**
 * 구간 길이만큼 지정한 GE를 소유자 ASC에 건다. 무적(Effect.Invincible)·퍼펙트가드(Effect.PerfectGuard) 판정 구간이 이걸로 열린다.
 *
 * 구간 시작에 길이만큼의 GE를 걸어 두고 끝에서 걷어내지 않는다.
 * 노티파이 상태 객체는 몽타주 에셋에 하나뿐이라 액터별 핸들을 못 들고, 정의로 조회해 지우면 같은 GE를 건 다른 출처까지 걷어가기 때문이다.
 * 덕분에 몽타주가 중간에 끊겨도 태그가 남지 않는다.
 *
 * EffectClass는 지속시간을 스펙에서 덮어쓸 수 있는 GE여야 한다 — Instant GE를 지정하면 구간이 성립하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ApplyGameplayEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	/** 트랙에 클래스 이름이 뜨지 않으면 한 몽타주에 놓인 구간들을 눈으로 구분할 수 없다. */
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<UGameplayEffect> EffectClass;
};
