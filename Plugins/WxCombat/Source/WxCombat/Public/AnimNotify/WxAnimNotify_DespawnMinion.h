// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_DespawnMinion.generated.h"

/**
 * 소유 폰을 주인으로 보고 월드의 MinionSubsystem에 소환물 거두기를 맡긴다. 소환물은 파괴되지 않고 Ability.Death 로 죽는다. 권위 판정은 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_DespawnMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 주인의 소환물 중 이 클래스(하위 포함)인 것만 거둔다. 비우면 아무것도 거두지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxCore.WxSpawnable", AllowAbstract = "false"))
	TSubclassOf<APawn> MinionClass;
};
