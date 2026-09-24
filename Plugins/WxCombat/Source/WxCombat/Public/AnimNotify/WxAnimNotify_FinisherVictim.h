// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_FinisherVictim.generated.h"

class UAnimMontage;

/**
 * 처형 대상이 짝 몽타주를 시작할 시점에 GameplayEvent를 발행한다.
 * 자신을 페이로드로 전달할 뿐, 재생은 대상을 쥔 UWxAbility_Finisher에 맡긴다.
 * 0초에 두어도 첫 틱에 불리므로 대상은 공격자보다 한 틱 늦게 시작한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherVictim : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetEditorColor() override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<UAnimMontage> VictimMontage;
};
