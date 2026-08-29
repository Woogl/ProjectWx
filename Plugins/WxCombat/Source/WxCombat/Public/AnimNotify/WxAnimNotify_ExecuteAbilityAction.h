// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_ExecuteAbilityAction.generated.h"

/**
 * 어빌리티가 몽타주 시점별 효과를 직접 처리할 수 있도록 GameplayEvent를 발행하는 공용 AnimNotify.
 * Event.AbilityAction 하위 태그만 허용한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_ExecuteAbilityAction : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Ability Action", meta = (Categories = "Event.AbilityAction"))
	FGameplayTag ActionEventTag;
};
