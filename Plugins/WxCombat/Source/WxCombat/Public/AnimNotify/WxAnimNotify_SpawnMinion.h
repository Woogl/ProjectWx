// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnMinion.generated.h"

/**
 * 소환 클래스·위치·상한을 풀어 월드의 MinionSubsystem에 생성을 맡긴다. 권위 판정과 상한 처리는 서브시스템이 한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 소환자의 팀을 물려받을 수 있는 Pawn만 지정한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion", meta = (MustImplement = "/Script/AIModule.GenericTeamAgentInterface"))
	TSubclassOf<APawn> MinionClass;

	/** 소환자 로컬 기준 스폰 지점. 실제 위치는 스폰 시 충돌 보정으로 밀릴 수 있다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion")
	FVector SpawnOffset = FVector(200.f, 0.f, 0.f);

	/** 소환자가 동시에 유지할 소환물 수. 넘치면 가장 오래된 소환물부터 파괴하고 새로 소환한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion", meta = (ClampMin = 1))
	int32 MaxMinionCount = 1;
};
