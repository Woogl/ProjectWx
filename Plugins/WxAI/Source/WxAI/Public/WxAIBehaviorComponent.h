// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxAIBehaviorComponent.generated.h"

class AController;
class APawn;
class UBehaviorTree;
struct FGameplayEventData;

/**
 * 이 캐릭터가 AI 로 굴러갈 때 필요한 것을 모은다 — 행동 자산, 감각 수치, 피격 자극 보고.
 * 퍼셉션은 컨트롤러에 붙어 폰 종류를 가리지 않으므로, 종류별 차이는 이 컴포넌트가 빙의 시점에 밀어 넣어 낸다.
 * 캐릭터가 컨트롤러를 갈아타도(파티원 교체) 이 셋은 캐릭터를 따라다닌다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXAI_API UWxAIBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxAIBehaviorComponent();

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#endif

	UBehaviorTree* GetBehaviorTree() const;

	float GetSightRadius() const;
	float GetSightAngle() const;
	float GetHearingRadius() const;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** 이 폰의 감각 수치를 컨트롤러의 퍼셉션 설정에 옮긴다. 플레이어가 잡고 있으면 옮길 곳이 없어 아무 일도 하지 않는다. */
	void ApplySenseSettings(AController* Controller) const;

	/**
	 * 폰이 받은 적대 대미지를 촉각(Damage 센스)으로 보고한다. 시야·청각이 놓치는 가해자도 이 경로로 잡힌다.
	 * 자극은 피격 액터로 리스너를 역추적하므로, 플레이어가 조종 중이면 받을 리스너가 없어 조용히 버려진다.
	 */
	void HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ForceUnits = "cm"))
	float SightRadius = 1500.f;

	/** 정면 기준 편측 시야각(도). 전체 시야각은 이 값의 2배다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ClampMax = "180", ForceUnits = "deg"))
	float SightAngle = 60.f;

	/** 소음 쪽에서 정한 거리와 이 값 중 짧은 쪽이 실제 청취 거리다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI|Perception", meta = (ClampMin = "0", ForceUnits = "cm"))
	float HearingRadius = 1000.f;
};
