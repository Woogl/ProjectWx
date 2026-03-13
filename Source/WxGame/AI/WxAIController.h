// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIController.generated.h"

class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;

/**
 * 에너미 AI 컨트롤러.
 *
 * AI Perception으로 타겟을 감지하고 Behavior Tree로 행동을 결정한다.
 * 감지된 타겟은 Blackboard의 "TargetActor" 키에 자동 반영.
 */
UCLASS()
class WXGAME_API AWxAIController : public AAIController
{
	GENERATED_BODY()

public:
	AWxAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float LoseSightRadius = 1600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightAngle = 90.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float MemoryLength = 5.f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetAlerted(bool bNewAlerted);

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	bool bAlerted = false;

	static const FName BBKey_TargetActor;
};
