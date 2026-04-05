// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/**
 * 무기 공격 구간 AnimNotifyState.
 *
 * ANS_WeaponCollision과 공격 속성 설정을 하나로 결합.
 * NotifyBegin~NotifyEnd 구간 동안:
 *  1. 캐릭터 ASC에 ANS.WeaponCollision 태그를 부여하여 무기 히트 콜리전 활성화
 *  2. bUnblockable이 true이면 무기에 Damage.Unblockable 태그를 부여
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
	UPROPERTY(EditAnywhere, Category = "Wx|Weapon")
	bool bUnblockable = false;
};
