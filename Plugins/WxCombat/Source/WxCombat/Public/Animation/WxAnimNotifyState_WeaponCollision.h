// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_WeaponCollision.generated.h"

/**
 * 무기 콜리전 활성/비활성 AnimNotifyState.
 *
 * 공격 몽타주에 배치하면 NotifyBegin~NotifyEnd 구간 동안
 * 캐릭터의 EquippedWeapon 히트 콜리전을 활성화.
 */
UCLASS(DisplayName = "Wx Weapon Collision")
class WXCOMBAT_API UWxAnimNotifyState_WeaponCollision : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
