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
 * TargetActor 의 유무에 따라 폰의 회전 모드도 함께 발행한다 — 타겟이 있으면 그 액터를 바라본 채 이동(strafe), 없으면 이동 방향으로 회전(평상시).
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

	/** 빙의한 폰이 가진 시야/청각 파라미터를 주입해 센스를 재구성한다. 컨트롤러가 OnPossess 에서 호출한다. */
	void ApplySenseSettings(float InSightRadius, float InSightAngle, float InMaxHearingRange);

protected:
	// 시야/청각 감지 파라미터는 빙의한 폰(AWxEnemyCharacter)이 ApplySenseSettings 로 주입한다. 아래 값은 폰이 주입하지 않았을 때의 기본값(fallback)이다.
	float SightRadius = 1500.f;

	// 정면 기준 편측 시야각(half-angle). 전체 시야각 120° → 좌우 각 60°.
	float SightAngle = 60.f;

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

	/**
	 * TargetActor 를 설정/해제하는 단일 지점. BB 키 쓰기와 함께 회전 모드(전투 시 strafe)를 발행한다.
	 * 타겟이 있으면 AIController 포커스를 그 액터로 두고 bUseControllerDesiredRotation 으로 전환해 타겟을 바라본 채 이동(strafe)하게 하고,
	 * 없으면 포커스를 해제하고 bOrientRotationToMovement(이동 방향으로 회전)로 되돌린다. 값이 바뀔 때만 동작한다.
	 */
	void SetTargetActor(AActor* NewTarget);

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
