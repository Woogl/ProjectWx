// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ApplyGameplayEffect.generated.h"

class UGameplayEffect;

/**
 * 무적(Effect.Invincible)·퍼펙트가드(Effect.PerfectGuard) 판정 구간이 이걸로 열린다.
 *
 * 구간의 수명은 이 노티파이가 소유한다 — 시작에 걸고 끝에서 걷어낸다.
 * GE에 지속시간을 실어 스스로 만료시키면 애니메이션 시계와 GE 시계가 둘로 갈려, 재생 속도가 도중에 바뀌는 순간 구간이 애니메이션과 어긋난다.
 *
 * 부여·제거는 서버에서만 실행하고 소유 클라를 포함한 클라이언트는 GAS 복제를 따른다.
 * 클라이언트 예측은 별도 동기화 설계 전까지 사용하지 않는다.
 *
 * EffectClass는 지속시간이 없는 GE여야 한다 — Instant나 HasDuration을 지정하면 구간이 성립하지 않는다.
 * 끝에서는 이 구간이 건 핸들의 스택 하나만 걷으므로, 스택형이 아닌 GE도 겹친 다른 구간·소유자의 효과를 건드리지 않는다.
 * 노티파이 객체는 몽타주 에셋에 하나라 여러 캐릭터가 공유하므로, 구간은 몽타주 인스턴스로 가른다 — 몽타주에서만 쓴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ApplyGameplayEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** 엔진 기본 구현은 빈 이벤트 참조로 NotifyBegin/End를 불러 몽타주 인스턴스를 잃으므로 페이로드에서 직접 받는다. */
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<UGameplayEffect> EffectClass;

private:
	void BeginWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID);
	void EndWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID);

	/** 몽타주 인스턴스 ID는 전역에서 고유하다. 서버에서만 채워진다. */
	TMap<int32, FActiveGameplayEffectHandle> AppliedEffects;
};
