// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxEnemyController.generated.h"

class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;

/**
 * 에너미 AI 컨트롤러.
 *
 * AI Perception으로 타겟을 감지하고 Behavior Tree로 행동을 결정한다.
 * Blackboard 자동 갱신 키:
 *  - SelfActor: OnPossess 시 폰. OnUnPossess 시 클리어.
 *  - HomeLocation: OnPossess 시 폰의 위치. 복귀/Leash 분기에 사용.
 *  - TargetActor: 시야 안의 타겟. 시야에서 벗어나면 클리어.
 *  - TargetLastKnownLocation: 마지막 감지 위치. 타겟 분실 후 Search 분기에 사용.
 *
 * Alerted 상태는 BT 가 읽지 않는 컨트롤러 내부 상태(시야각 확장 부수효과)이므로 BB 키로 노출하지 않고 멤버 변수로만 보관한다.
 */
UCLASS()
class WXGAME_API AWxEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AWxEnemyController();

protected:
	virtual void PostInitProperties() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

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

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
