// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_UseItem.generated.h"

/**
 * 소비 아이템 마시기 몽타주의 사용(꿀꺽) 시점에 Event.UseItem 을 아바타 ASC로 송출하는 AnimNotify.
 *
 * UseItem 어빌리티의 WaitGameplayEvent 가 이 시점을 받아 UseItemByDef(충전 1 감소 + 회복 GE)를 수행한다.
 * 몽타주가 클라/서버 양쪽에서 재생되어 노티파이도 양쪽에서 발화하지만, 어빌리티가 권위를 검사해 서버에서만 실제 사용을 처리한다.
 */
UCLASS()
class WXGAME_API UWxAnimNotify_UseItem : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
