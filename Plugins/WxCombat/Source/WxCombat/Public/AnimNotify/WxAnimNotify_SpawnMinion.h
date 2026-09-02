// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SpawnMinion.generated.h"

/**
 * 자신을 이벤트 페이로드로 전달할 뿐, 생성은 MinionComponent에 맡긴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SpawnMinion : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	TSubclassOf<AActor> GetMinionClass() const;
	FVector GetSpawnOffset() const;
	float GetLifetime() const;

protected:
	/** 소환수뿐 아니라 터렛·토템 같은 설치물도 같은 경로로 낸다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion")
	TSubclassOf<APawn> MinionClass;

	/** 소환자 로컬 기준 스폰 지점. 실제 위치는 스폰 시 충돌 보정으로 밀릴 수 있다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion")
	FVector SpawnOffset = FVector(200.f, 0.f, 0.f);

	/** 0이면 무제한이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Minion", meta = (ClampMin = "0"))
	float Lifetime = 0.f;
};
