// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxMinionComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 소환 AnimNotify가 보낸 GameplayEvent를 받아 서버 권위로 소환물을 생성한다.
 * 소환물 클래스와 스폰 지점은 이벤트 페이로드가 소유하므로 어빌리티와 무관하게 동작한다.
 * 소환수와 설치물을 가리지 않으며, AI 빙의는 소환물 자신의 AutoPossessAI가 맡는다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxMinionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 관리 중인 활성 소환수 모두에게 정확한 식별 태그의 어빌리티 발동을 요청한다. 서버에서 발동을 수락한 소환수 수를 반환한다. */
	int32 TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag);

	/** Payload를 TriggerEventData로 전달하는 명령 경로다. EventTag는 Event.CommandMinionAbility로 설정된다. */
	int32 TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag, const FGameplayEventData& Payload);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 동시에 유지할 소환물 수. 넘치면 가장 오래된 것을 파괴하고 새로 소환한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion", meta = (ClampMin = "1"))
	int32 MaxMinionCount = 1;

	void HandleSpawnMinionEvent(const FGameplayEventData* Payload);

	void RemoveInvalidOrDeadMinions();

	bool TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle SpawnMinionEventHandle;

	TArray<TWeakObjectPtr<AActor>> ActiveMinions;
};
