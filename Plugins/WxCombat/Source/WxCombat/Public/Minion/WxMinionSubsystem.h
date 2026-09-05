// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxMinionSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 소환물을 서버 권위로 생성해 주인별로 관리한다. AI 빙의는 소환물의 AutoPossessAI에 맡긴다.
 * 로스터에 소환물로 올라 있는 액터는 소환자가 될 수 없다 — 주인의 몽타주를 따라하는 소환물이 같은 노티파이로 증식하지 않는다.
 * 주인에게 살아 있는 소환물이 하나라도 있는 동안 주인 ASC에 State.Minion.Active를 복제 loose 태그로 발행한다 — 소환↔명령 어빌리티가 한 슬롯을 나누는 기준.
 */
UCLASS()
class WXCOMBAT_API UWxMinionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 상한을 넘치면 주인의 가장 오래된 소환물부터 파괴한다. 생성하지 않으면 null. */
	APawn* SpawnMinion(AActor& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform, int32 MaxMinionCount);

	/**
	 * 주인이 관리 중인 활성 소환물 모두에게 정확한 식별 태그의 어빌리티 발동을 요청하고, 발동을 수락한 소환물 수를 반환한다.
	 * Payload는 TriggerEventData로 전달되고 EventTag는 Event.CommandMinionAbility로 설정된다.
	 */
	int32 TryActivateAbilityOnMinions(AActor& Master, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload);

protected:
	/** 에디터 월드에서는 시퀀서 프리뷰의 노티파이가 권위를 통과해 레벨에 스폰해 버리므로 게임 월드에만 만든다. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	UFUNCTION()
	void HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void HandleMinionEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 태그 이벤트는 어느 ASC에서 왔는지 주지 않으므로 구독할 때 소환물을 페이로드로 실어 둔다. */
	void HandleMinionDeathTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<APawn> Minion);

	bool IsMinion(const AActor& Actor) const;

	/** 소환물을 로스터에서 내리고 구독을 푼 뒤 주인을 돌려준다. 로스터에 없으면 null. 태그는 호출자가 마감한다 — 교체 소환은 내림과 올림을 한 번에 반영해야 해서. */
	AActor* ReleaseMinion(APawn& Minion);

	/** 주인의 로스터가 비었는지로 State.Minion.Active를 올리거나 내린다. */
	void PublishMinionActiveTag(AActor& Master) const;

	bool TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const;

	/** 주인 → 소환 순서의 살아 있는 소환물. 사망·파괴는 구독으로 즉시 내린다. 주인이 처음 오를 때 OnEndPlay를 구독하고 EndPlay에서 통째로 내린다. */
	TMap<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<APawn>>> Rosters;
};
