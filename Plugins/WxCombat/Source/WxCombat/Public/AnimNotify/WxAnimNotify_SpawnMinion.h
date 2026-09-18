// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnMinion.generated.h"

/**
 * 소환 클래스와 스폰 지점을 풀어 월드의 MinionSubsystem에 생성을 맡긴다. 권위 판정과 상한 처리는 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
	UWxAnimNotify_SpawnMinion();
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/**
	 * CDO에 MinionComponent가 네이티브로 붙어 있어야 소환된다 — 동시 유지 수와 소환 조건을 그 컴포넌트가 선언한다.
	 * 팀을 물려받을 수 있는지는 서브시스템이 런타임에 보며, 못 물려받아도 소환은 된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion")
	TSubclassOf<APawn> MinionClass;

	/** 소환자 로컬 기준 스폰 지점. 실제 위치는 스폰 시 충돌 보정으로 밀릴 수 있다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion")
	FTransform LocalSpawnOffset;
};
