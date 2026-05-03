// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "WxCueNotify_Metamorphose.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * 변신 GameplayCue.
 *
 * GameplayEffect의 유효 기간 동안 캐릭터의 SkeletalMesh를 교체하고,
 * 이펙트가 만료되거나 제거되면 원래 메쉬로 복원한다.
 *
 * BP에서 캐릭터 메시에 추가 부착한 외형 SkeletalMeshComponent가 있으면 그 컴포넌트를 우선 대상으로 한다.
 * (AWxCharacterBase::ApplyEquipmentVisuals 와 동일 규칙)
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxCueNotify_Metamorphose : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AWxCueNotify_Metamorphose();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	/** 변신 중 사용할 SkeletalMesh */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Cue")
	TObjectPtr<USkeletalMesh> MetamorphoseMesh;

private:
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> OriginalMesh;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> AffectedMeshComp;
};
