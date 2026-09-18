// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WxMinionComponent.generated.h"

class APawn;

/**
 * 소환물로 태어났을 때의 정책을 선언하고, 등록·사망·해제까지 자기 생애를 관리한다.
 * 적 캐릭터가 모두 들고 있으므로 보유 여부가 아니라 Instigator 가 소환물 여부를 가른다 — 소환되지 않은 적은 등록하지 않아 값이 무의미하다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxMinionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxMinionComponent();

	/**
	 * 이 폰을 소환한 주인. 소환된 적 없으면 null이라 이 값이 곧 소환물 여부다.
	 * APawn이 빈 인스티게이터를 자기 자신으로 채우므로 소환 여부 플래그를 따로 두지 않는다.
	 * 컴포넌트 보유와 무관하게 답해야 하므로 static 이다.
	 */
	static APawn* GetMaster(const APawn& Minion);

	/** 소환물은 폰이다. 주인을 Instigator 로 무는 이상 다른 타입은 관계를 절반만 맺는다. */
	APawn* GetMinionPawn() const;

	/** 주인 ASC 가 없으면 빈 태그로 평가하므로 요구 태그가 없을 때만 통과한다. 스폰 전 CDO 에서 불린다. */
	bool CanBeSummonedBy(APawn& Master) const;

	/** 음수는 0으로 보정한다. */
	int32 GetMaxCountPerMaster() const;

	FGameplayTag GetMasterStateTag() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 0이면 개수 제한이 없다.
	 * 양수이면 해당 개수를 넘길 때 주인의 가장 오래된 소환물부터 파괴한다.
	 * 정리 대상은 MasterStateTag 가 같은 소환물뿐이다 — 태그를 비워 두면 주인의 모든 소환물이 한 상한을 나눠 쓴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	int32 MaxCountPerMaster = 1;

	/** 살아 있는 동안 주인 ASC 에 붙는 태그이자 상한을 다투는 단위다. 종류마다 다른 태그를 주면 소환·명령 스킬을 따로 게이팅할 수 있다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	FGameplayTag MasterStateTag;

	/** 주인이 만족해야 이 소환물이 생성되는 조건. 다른 소환물이 발행한 MasterStateTag 를 IgnoreTags 로 걸면 종류끼리 배타가 된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	FGameplayTagRequirements MasterTagRequirements;

private:
	/** 사망은 액터가 남은 채로 로스터에서만 내려가는 사유라, EndPlay 와 별개로 듣는다. */
	void HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	FDelegateHandle DeathTagHandle;
};
