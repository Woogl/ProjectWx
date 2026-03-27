// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_ApplyEffectToSelf.generated.h"

class UGameplayEffect;

/**
 * GameplayEffect 자기 적용 AnimNotify.
 *
 * 몽타주에 배치하면 해당 프레임에서 소유 캐릭터의 ASC에 지정한 GameplayEffect를 적용한다.
 * 서버에서만 적용한다.
 */
UCLASS(DisplayName = "Wx Apply Effect To Self")
class WXCOMBAT_API UWxAnimNotify_ApplyEffectToSelf : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 적용할 GameplayEffect 클래스 */
	UPROPERTY(EditAnywhere, Category = "Wx|Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	/** Effect 레벨 */
	UPROPERTY(EditAnywhere, Category = "Wx|Effect")
	float EffectLevel = 1.f;
};
