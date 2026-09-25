// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_DespawnMinion.generated.h"

/**
 * 소유 폰을 주인으로 보고 월드의 MinionSubsystem에 소환물 거두기를 맡긴다.
 * 소환물은 Event.Death 이벤트로 사망 어빌리티를 태워 죽고, 그 어빌리티가 없을 때만 파괴된다.
 * 권위 판정은 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_DespawnMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 주인의 소환물 중 이 클래스(하위 포함)인 것만 거둔다. 비우면 아무것도 거두지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxCore.WxSpawnable", AllowAbstract = "false"))
	TSubclassOf<APawn> MinionClass;
};
