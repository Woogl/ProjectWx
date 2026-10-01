// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WxMinionComponent.generated.h"

class APawn;

/**
 * 소환물로 태어났을 때의 정책을 선언하고, 주인 태그·사망·소멸까지 자기 생애를 관리한다.
 * 적 캐릭터가 모두 들고 있으므로 보유 여부가 아니라 Instigator 가 소환물 여부를 가른다 — 소환되지 않은 적은 주인과 묶이지 않아 값이 무의미하다.
 *
 * 주인 → 소환물 참조는 어디에도 들지 않는다. 소환물 액터가 필요할 때(교체·회수·돌진)만 월드에서 그 주인의 살아 있는 소환물을 찾는다.
 * 주인과 묶는 일(주인 태그·취소·주인 소멸)은 전부 파괴나 복제값 쓰기라 권위에서만 한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxMinionComponent : public UActorComponent
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

	/**
	 * 서버에서만 소환한다. SpawnTransform 은 월드 기준이다.
	 * 같은 MasterStateTag 의 소환물이 이미 있으면 그것을 파괴하고 교체한다.
	 * 소환물이 선언한 소환 가능 조건을 만족하지 못하거나 취소 조건을 이미 만족하면, 또는 Master 가 소환물이면 아무것도 하지 않고 null 을 돌려준다.
	 */
	static APawn* SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform);

	/** 주인의 소환물 중 MinionClass(하위 포함)인 것에 Event.Death 를 보내 사망 어빌리티로 거둔다. 파괴가 아니라 사망 연출과 시체 수명을 거친다. */
	static void DespawnMinions(const APawn& Master, TSubclassOf<APawn> MinionClass);

	/** 살아 있는 소환물이 둘 이상이면 먼저 생성된 쪽을 돌려준다. */
	static APawn* FindActiveMinion(const APawn& Master);

	/** 소환물은 폰이다. 주인을 Instigator 로 무는 이상 다른 타입은 관계를 절반만 맺는다. */
	APawn* GetMinionPawn() const;

	/**
	 * 주인 ASC 가 없으면 빈 태그로 평가한다. 스폰 전 CDO 에서 불린다.
	 * 취소 조건을 이미 만족하면 거부한다 — 통과시키면 교체가 기존 소환물을 먼저 지운 뒤 새 소환물마저 곧바로 취소돼 둘 다 잃는다.
	 */
	bool CanBeSummonedBy(APawn& Master) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 살아 있는 동안 주인 ASC 에 하나씩 쌓이는(복제) 태그이자 소환물의 종류다.
	 * 같은 태그의 소환물은 주인당 하나라, 새로 소환되면 기존 것이 파괴된다. 비워 두면 태그를 쌓지 않고, 비운 것끼리 한 자리를 나눠 쓴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	FGameplayTag MasterStateTag;

	/** 소환하는 순간 주인이 만족해야 하는 조건. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	FGameplayTagRequirements SummonMasterTagRequirements;

	/** 소환된 동안 주인이 만족하면 이 소환물이 사라지는 조건. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	FGameplayTagRequirements CancelMasterTagRequirements;

private:
	/** 주인의 살아 있는 소환물. 소환물은 몇 마리뿐이라 목록을 들지 않고 쓸 때마다 월드에서 찾는다. */
	static TArray<UWxMinionComponent*> FindMinions(const APawn& Master);

	/** 사망과 EndPlay 가 둘 다 와도 주인과의 묶음은 한 번만 푼다 — 주인 소멸 구독이 아직 있는지로 가린다. */
	void ReleaseMaster();

	/** 사망은 액터가 남은 채로 소환물에서 내려가는 사유라, EndPlay 와 별개로 듣는다. */
	void HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleMasterTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	FDelegateHandle DeathTagHandle;

	/** 주인 ASC 는 들고 있지 않고 해제할 때 GetMaster 로 다시 구한다. */
	FDelegateHandle MasterTagHandle;
};
