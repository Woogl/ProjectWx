// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/**
 * 무기 공격 구간 AnimNotifyState.
 *
 * ANS_WeaponCollision과 공격 속성 설정을 하나로 결합.
 * NotifyBegin~NotifyEnd 구간 동안:
 *  1. 캐릭터 ASC에 ANS.WeaponCollision 태그를 부여하여 무기 히트 콜리전 활성화
 *  2. bUnblockable이 true이면 무기에 Damage.Unblockable 태그를 부여
 *  3. HitReactTag가 유효하면 무기에 해당 태그를 부여 (예: Event.HitReact.Knockdown)
 */
UCLASS(DisplayName = "Wx Weapon Attack")
class WXCOMBAT_API UWxAnimNotifyState_WeaponAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** true이면 이 구간의 공격은 가드·퍼펙트 가드를 무시 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bUnblockable = false;

	/**
	 * 적중 시 무기 AttackTags에 부여할 HitReact 태그.
	 * 비어있으면 기본 HitReact 동작(PP 소진 조건)을 유지하고,
	 * Event.HitReact의 자식 태그(Knockback/Knockdown/Knockup 등)를 지정하면
	 * PP 잔량과 무관하게 해당 종류의 HitReact가 강제 발동된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (Categories = "Event.HitReact"))
	FGameplayTag HitReactTag;
};
