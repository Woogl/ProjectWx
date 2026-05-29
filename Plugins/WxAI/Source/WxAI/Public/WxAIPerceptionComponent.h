// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIPerceptionComponent.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBlackboardComponent;

/**
 * AIController 에 부착해 사용하는 Perception 컴포넌트.
 *
 * Sight/Hearing/Damage 감지를 셋업하고 결과를 Blackboard 의 TargetActor / TargetLastKnownLocation 에 동기화한다.
 * 시각/피해는 TargetActor 를 확정하고, 청각은 TargetLastKnownLocation 만 기록한다(조사형). BT 가 위치로 이동/회전해 조사하다가 시야에 들어오면 비로소 타겟이 확정된다.
 * 시야 안의 타겟이 처음 감지되면 시야각을 180°로 확장 (Alerted), 잃으면 원복.
 *
 * Alerted 상태는 BT 가 읽지 않는 컴포넌트 내부 상태(시야각 확장 부수효과)이므로 BB 키로 노출하지 않고 멤버 동작으로만 처리한다.
 *
 * 인식(State.Recognized)은 TargetActor 와 분리해 다룬다. TargetActor 는 시야 상실 즉시 비워져(BT 가 조사/귀환으로 전환)
 * 전투 분기를 구동하지만, 인식은 한 번 켜지면 유지되어(벽 뒤 등 일시적 시야 상실에도 네임플레이트 유지) 추적 종료 조건에서만 해제한다.
 * 추적 종료 = 배치 지점(HomeLocation)에서 LeashRadius 이상 이탈하거나, 타겟을 놓친 채 배치 지점으로 귀환 완료.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class WXAI_API UWxAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UWxAIPerceptionComponent();

	virtual void PostInitProperties() override;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightRadius = 1500.f;

	// 정면 기준 편측 시야각(half-angle). 전체 시야각 120° → 좌우 각 60°.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float SightAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float MaxHearingRange = 1000.f;

	// 적이 배치 지점(HomeLocation)에서 이 거리 이상 멀어지면 추적을 종료하고 인식을 해제한다. (cm) 기획서 4.3: 30m.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float LeashRadius = 3000.f;

	// 타겟을 놓친 뒤 배치 지점 이 거리 안으로 돌아오면 귀환 완료로 보고 인식을 해제한다. (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float HomeArrivalRadius = 200.f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	/**
	 * 인식 여부를 판정하는 단일 지점. 현재 상태(TargetActor, HomeLocation 거리)로 인식 on/off 를 결정해 SetRecognized 로 적용한다.
	 *  - 타겟을 직접 확인 중이면 인식(최초 감지 시 즉시 ON, 교전 중 유지)
	 *  - 타겟을 놓친 상태에서 추적 종료 조건(리시 이탈/귀환 완료)을 만족하면 해제, 그 전까지는 유지(벽 뒤 등 일시적 시야 상실)
	 * 감지 갱신(이벤트)과 BeginPlay 부터 항상 도는 주기 타이머(폴) 양쪽에서 호출된다.
	 */
	void UpdateRecognition();

	/** UpdateRecognition 의 결정을 적용하는 순수 setter. State.Recognized 태그를 폰의 ASC 에 부여/해제하며, 전환에서만 동작한다(멱등). 네임플레이트가 복제된 태그를 소비한다. */
	void SetRecognized(bool bNewRecognized);

	UBlackboardComponent* GetBlackboard() const;

	FTimerHandle LeashTimerHandle;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
