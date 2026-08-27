// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_SendGameplayEvent.generated.h"

/**
 * 몽타주가 양쪽에서 복제 재생돼 노티파이도 양쪽에서 발화하므로, 수신 어빌리티 인스턴스도 양쪽에서 이벤트를 받는다.
 * 권위가 필요한 처리는 수신 측이 각자 권위 게이트로 막는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTag EventTag;
};
