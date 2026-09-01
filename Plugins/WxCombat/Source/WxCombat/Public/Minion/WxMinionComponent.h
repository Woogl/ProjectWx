// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 동시에 유지할 소환물 수. 넘치면 가장 오래된 것을 파괴하고 새로 소환한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion", meta = (ClampMin = "1"))
	int32 MaxMinionCount = 1;

private:
	void HandleSpawnMinionEvent(const FGameplayEventData* Payload);

	/** 사라졌거나 죽은 소환물을 걷어낸다. 시체는 월드에 남으므로 사망 태그로 가른다. */
	void PruneInactiveMinions();

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle SpawnMinionEventHandle;

	/** 소환 순서를 유지한다. 앞쪽이 가장 오래된 소환물이다. */
	TArray<TWeakObjectPtr<AActor>> ActiveMinions;
};
