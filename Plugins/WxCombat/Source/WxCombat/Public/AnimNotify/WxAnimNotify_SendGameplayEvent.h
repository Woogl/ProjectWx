// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_SendGameplayEvent.generated.h"

/**
 * 지정한 GameplayTag 이벤트를 몽타주 재생 중인 아바타 ASC로 송출하는 범용 AnimNotify.
 *
 * 어빌리티의 WaitGameplayEvent 가 이 시점을 받아 타이밍 의존 처리를 수행한다.
 * (예: 피니셔 몽타주 후반에서 Event.Finisher.Damage 를 송출 → 피해·DP 제거 적용)
 *
 * 몽타주는 클라/서버 양쪽에서 복제 재생되어 노티파이도 양쪽에서 발화하지만, 실제 처리는
 * 서버에서 도는 어빌리티 인스턴스에서만 일어나므로 안전하다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 송출할 이벤트 태그. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTag EventTag;
};
