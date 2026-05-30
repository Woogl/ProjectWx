// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIPerceptionComponent.generated.h"

class APawn;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBlackboardComponent;

/**
 * AIController 에 부착해 사용하는 Perception 컴포넌트.
 *
 * Sight/Hearing/Damage 감지를 셋업하고 결과를 Blackboard 의 TargetActor / TargetLastKnownLocation 에 동기화한다.
 * 시각/피해는 TargetActor 를 확정하고, 청각은 TargetLastKnownLocation 만 기록한다(조사형).
 *
 * 한 번 확보한 TargetActor 는 시야를 잠시 잃어도(보스 등 뒤로 이동, 벽 뒤 등) 유지되며,
 * 폰이 배치 지점(HomeLocation)에서 LeashRadius 이상 벗어났을 때(리시 이탈)에만 비워진다(이때 BT 는 복귀).
 * 인식(State.Recognized)도 같은 수명을 따른다 — 추적 중이면 on, 리시 이탈로 추적이 끝나면 off 이며,
 * 서버에서 폰 ASC 에 MinimalReplication 태그로 발행되어 네임플레이트/보스 체력바 표시에 소비된다.
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

	// 폰이 배치 지점(HomeLocation)에서 이 거리 이상 벗어나면 추적을 끝내고 복귀하며 인식을 해제한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	float LeashRadius = 3000.f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	/**
	 * 인식/추적 상태를 판정하는 단일 지점.
	 *  - 폰이 리시(HomeLocation 기준 LeashRadius)를 벗어나면 TargetActor/LastKnown 과 인식을 함께 해제한다(복귀).
	 *  - 그 외(폰이 리시 안)에서는 인식 on, 추적 대상이 없으면 off 를 SetRecognized 로 적용한다.
	 * 감지 갱신(이벤트)과 BeginPlay 부터 항상 도는 주기 타이머(폴) 양쪽에서 호출된다.
	 */
	void UpdateRecognition();

	/** UpdateRecognition 의 결정을 적용하는 순수 setter. State.Recognized 태그를 폰의 ASC 에 부여/해제하며, 전환에서만 동작한다. 네임플레이트가 복제된 태그를 소비한다. */
	void SetRecognized(bool bNewRecognized);

	APawn* GetOwnerPawn() const;

	UBlackboardComponent* GetBlackboard() const;

	FTimerHandle LeashTimerHandle;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
