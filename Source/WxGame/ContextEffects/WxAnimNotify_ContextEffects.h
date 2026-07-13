// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "WxAnimNotify_ContextEffects.generated.h"

/**
 * 컨텍스트 이펙트 AnimNotify.
 *
 * 애니메이션의 특정 프레임에서 EffectTag 를 발행한다. 실제 표면 판정·사운드/VFX 재생은
 * 소유 액터의 UWxContextEffectsComponent 가 담당한다(노티파이는 의도만 넘긴다).
 * 착지·구르기 등 표면별 코스메틱에 사용한다.
 */
UCLASS(meta = (DisplayName = "Wx Context Effect"))
class WXGAME_API UWxAnimNotify_ContextEffects : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 재생할 컨텍스트 이펙트 종류. 라이브러리가 이 태그로 애셋을 조회한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|ContextEffects")
	FGameplayTag EffectTag;
};
