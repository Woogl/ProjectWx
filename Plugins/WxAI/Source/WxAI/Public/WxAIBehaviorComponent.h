// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxAIBehaviorComponent.generated.h"

class UBehaviorTree;

/**
 * AIController 가 실행할 행동 자산과 감각 수치를 캐릭터 상속과 분리해 제공한다.
 * 감각 수치는 컨트롤러의 UWxAIPerceptionComponent 가 빙의 시점에 읽어 간다 — 퍼셉션은 컨트롤러에 붙어 폰 종류를 가리지 않으므로, 종류별 차이는 이 컴포넌트가 낸다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXAI_API UWxAIBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UBehaviorTree* GetBehaviorTree() const;

	float GetSightRadius() const;
	float GetSightAngle() const;
	float GetHearingRadius() const;

private:
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
