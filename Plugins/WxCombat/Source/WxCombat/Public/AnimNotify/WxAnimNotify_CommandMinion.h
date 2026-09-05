// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_CommandMinion.generated.h"

/**
 * 주인의 활성 소환물 전원에게 식별 태그의 어빌리티 발동을 월드의 MinionSubsystem에 맡긴다. 권위 판정은 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_CommandMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 소환물 ASC에서 이 태그와 정확히 일치하는 에셋 태그의 어빌리티를 찾는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;
};
