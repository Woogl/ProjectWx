// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_SendGameplayEvent.generated.h"

/**
 * 지정한 GameplayTag 이벤트를 몽타주 재생 중인 아바타 ASC로 송출하는 범용 AnimNotify.
 * 어빌리티의 WaitGameplayEvent가 이 시점을 받아 타이밍 의존 처리를 수행한다.
 *
 * 몽타주가 양쪽에서 복제 재생돼 노티파이도 양쪽에서 발화하지만, 처리는 서버 어빌리티 인스턴스에서만 일어난다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTag EventTag;
};
