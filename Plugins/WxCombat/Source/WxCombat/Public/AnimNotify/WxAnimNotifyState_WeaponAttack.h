// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxDamageInfo.h"
#include "WxAnimNotifyState_WeaponAttack.generated.h"

/**
 * 무기 공격 구간 AnimNotifyState.
 *
 * ANS_WeaponCollision과 공격 속성 설정을 하나로 결합.
 * NotifyBegin에서 캐릭터 ASC에 ANS.WeaponCollision 태그를 부여해
 * 무기 히트 콜리전을 활성화하고, 무기에 DamageInfo를 전달한다.
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
	UPROPERTY(EditAnywhere, meta = (ShowOnlyInnerProperties))
	FWxDamageInfo DamageInfo;
};
