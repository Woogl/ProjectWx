// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_UseItem.generated.h"

/**
 * 소비 아이템의 실제 사용 시점에 GameplayEvent를 발행한다.
 * 자신과 Mesh를 페이로드로 전달할 뿐, 아이템 사용은 ItemUseComponent에 맡긴다.
 */
UCLASS()
class WXGAME_API UWxAnimNotify_UseItem : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
