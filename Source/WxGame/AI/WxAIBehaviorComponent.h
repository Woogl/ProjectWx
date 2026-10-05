// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StateTreeReference.h"
#include "WxAIBehaviorComponent.generated.h"

class AController;
class APawn;
class UWxPatrolComponent;
struct FGameplayEventData;

/**
 * 이 캐릭터가 AI 로 굴러갈 때 필요한 것을 모은다 — 행동 자산, 감각 수치, 정찰 경로, 피격 자극 보고.
 * 퍼셉션은 컨트롤러에 붙어 폰 종류를 가리지 않으므로, 종류별 차이는 이 컴포넌트가 빙의 시점에 밀어 넣어 낸다.
 * 캐릭터가 컨트롤러를 갈아타도(파티원 교체) 이것들은 캐릭터를 따라다닌다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxAIBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxAIBehaviorComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#endif

	const FStateTreeReference& GetStateTree() const;

	const FStateTreeReference& GetPatternStateTree() const;

	/** 지금 향할 정찰 지점을 준다. 경로가 없거나 Once 경로를 완주했으면 false. */
	bool GetPatrolDestination(FVector& OutLocation) const;

	/** 정찰 지점에 도착했을 때 불러 다음 지점으로 넘긴다. */
	void AdvancePatrol();

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** 플레이어가 잡고 있으면 옮길 퍼셉션이 없어 아무 일도 하지 않는다. */
	void ApplySenseSettings(AController* Controller) const;

	/**
	 * 폰이 받은 적대 대미지를 촉각(Damage 센스)으로 보고한다. 시야·청각이 놓치는 가해자도 이 경로로 잡힌다.
	 * 자극은 피격 액터로 리스너를 역추적하므로, 플레이어가 조종 중이면 받을 리스너가 없어 조용히 버려진다.
	 */
	void HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	/** AWxAIController 가 이 폰에 빙의할 때 돌리는 트리. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI", meta = (Schema = "/Script/GameplayStateTreeModule.StateTreeAIComponentSchema"))
	FStateTreeReference StateTree;

	/** StateTree 의 AI.Pattern 태그 상태에 갈아 끼울 이 캐릭터의 패턴 트리. 비우면 StateTree 에 링크된 기본 패턴을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI", meta = (Schema = "/Script/GameplayStateTreeModule.StateTreeAIComponentSchema"))
	FStateTreeReference PatternStateTree;

	/** 스폰 주체(스포너 등)에 붙은 경로. 비어 있으면 정찰하지 않는다. */
	UPROPERTY(Transient)
	TObjectPtr<UWxPatrolComponent> PatrolPath;

	/**
	 * State Tree 태스크의 데이터는 상태에 들어올 때마다 새로 만들어지므로, 전투 뒤에 이어서 정찰하려면 진행 위치를 폰이 들고 있어야 한다.
	 * Once 경로를 완주하면 지점 개수로 넘겨 더 갈 곳이 없음을 나타낸다.
	 */
	int32 PatrolCursor = 0;

	/** PingPong 진행 방향(+1/-1). */
	int32 PatrolDirection = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ForceUnits = "cm"))
	float SightRadius = 1500.f;

	/** 정면 기준 편측 시야각(도). 전체 시야각은 이 값의 2배다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ClampMax = "180", ForceUnits = "deg"))
	float SightAngle = 60.f;

	/** 소음 쪽에서 정한 거리와 이 값 중 짧은 쪽이 실제 청취 거리다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ForceUnits = "cm"))
	float HearingRadius = 1000.f;
};
