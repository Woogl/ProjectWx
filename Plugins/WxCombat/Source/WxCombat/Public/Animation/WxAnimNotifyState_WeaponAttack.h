// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/**
 * 무기 공격 구간 AnimNotifyState.
 *
 * ANS_WeaponCollision과 AttackTags 설정을 하나로 결합.
 * NotifyBegin~NotifyEnd 구간 동안:
 *  1. 캐릭터 ASC에 ANS.WeaponCollision 태그를 부여하여 무기 히트 콜리전 활성화
 *  2. 무기의 AttackTags에 설정된 태그를 부여 (GE Spec에 반영)
 *
 * 예) AttackTags에 Damage.Unblockable을 설정하면 해당 구간의 공격은 가드 불가.
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
	/** 이 구간 동안 무기에 부여할 공격 속성 태그 (선택 사항) */
	UPROPERTY(EditAnywhere, Category = "Wx|AttackTags")
	FGameplayTagContainer AttackTags;
};
