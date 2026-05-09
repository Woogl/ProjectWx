// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIPerceptionComponent.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UBlackboardComponent;

/**
 * AIController 에 부착해 사용하는 Perception 컴포넌트.
 *
 * Sight/Damage 감지를 셋업하고 결과를 Blackboard 의 TargetActor / TargetLastKnownLocation 에 동기화한다.
 * 시야 안의 타겟이 처음 감지되면 시야각을 180°로 확장 (Alerted), 잃으면 원복.
 *
 * Alerted 상태는 BT 가 읽지 않는 컴포넌트 내부 상태(시야각 확장 부수효과)이므로 BB 키로 노출하지 않고 멤버 동작으로만 처리한다.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class WXAI_API UWxAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UWxAIPerceptionComponent();

	virtual void PostInitProperties() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float LoseSightRadius = 1600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightAngle = 90.f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetAlerted(bool bNewAlerted);

	UBlackboardComponent* GetBlackboard() const;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
