// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIPerceptionComponent.generated.h"

class APawn;
class UAbilitySystemComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBlackboardComponent;

/**
 * AIController 에 부착해 사용하는 Perception 컴포넌트.
 *
 * Sight/Hearing/Damage 감지를 셋업하고 결과를 Blackboard 의 TargetActor 에 동기화한다.
 * 세 센스 모두 감지 성공 시 그 액터(소리 발생원 포함)를 TargetActor 로 확정한다.
 *
 * TargetActor 의 유무에 따라 폰의 회전 모드도 함께 발행한다 — 타겟이 있으면 그 액터를 바라본 채 이동(strafe), 없으면 이동 방향으로 회전(평상시).
 *
 * 한 번 확보한 TargetActor 는 시야를 잠시 잃어도(보스 등 뒤로 이동, 벽 뒤 등) 유지되며, 리시 이탈 판정과 복귀는 BT(UWxBTDecorator_BeyondLeash + UWxBTTask_ReturnHome)가 담당한다 — 복귀 Task 가 SetTargetingSuppressed 로 타겟을 비우고 복귀 중 재감지를 억제한다.
 * 인식(State.InCombat)도 같은 수명을 따른다 — 추적 중이면 on, 복귀(억제)·사망 시 off 이며, 서버에서 폰 ASC 에 MinimalReplication 태그로 발행되어 네임플레이트 표시에 소비된다.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class WXAI_API UWxAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UWxAIPerceptionComponent();

	virtual void PostInitProperties() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 빙의한 폰이 가진 시야/청각 파라미터를 주입해 센스를 재구성한다. 컨트롤러가 OnPossess 에서 호출한다. */
	void ApplySenseSettings(float InSightRadius, float InSightAngle, float InMaxHearingRange);

	/**
	 * 타겟팅 억제(disengage)를 켜고 끈다. 리시 복귀 Task(UWxBTTask_ReturnHome)가 복귀 진입/종료에 호출한다.
	 *  - true: 현재 TargetActor 를 비우고 인식을 끄며 회전 모드를 원복하고, 이후 감지 자극을 무시해 복귀 중 재-어그로를 막는다.
	 *  - false: 억제만 풀어 다음 자극에서 정상 재감지하게 한다.
	 */
	void SetTargetingSuppressed(bool bSuppressed);

	/** 빙의한 폰의 ASC 사망 태그(State.Dead)에 인식 해제 콜백을 바인드/언바인드한다. 컨트롤러가 OnPossess/OnUnPossess 에서 호출한다. */
	void BindOwnerDeath();
	void UnbindOwnerDeath();

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** 폰의 ASC 에 State.Dead 가 부여되면(사망) 타겟과 인식을 정리한다. 폴 제거로 사라진 "시체 위 인식 잔존 방지"를 이벤트로 대체한다. */
	void HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	/**
	 * 인식/추적 상태를 판정하는 단일 지점.
	 *  - 억제(복귀) 중이거나 사망 상태면 인식 off.
	 *  - 그 외에서는 추적 대상이 있으면 on, 없으면 off 를 SetRecognized 로 적용한다.
	 * 리시 이탈 판정은 BT 로 이관되어 여기서 다루지 않는다. 감지 갱신(이벤트)에서 호출된다.
	 */
	void UpdateRecognition();

	/** UpdateRecognition 의 결정을 적용하는 순수 setter. State.InCombat 태그를 폰의 ASC 에 부여/해제하며, 전환에서만 동작한다. 네임플레이트가 복제된 태그를 소비한다. */
	void SetRecognized(bool bNewRecognized);

	/**
	 * TargetActor 를 설정/해제하는 단일 지점. BB 키 쓰기와 함께 회전 모드(전투 시 strafe)를 발행한다.
	 * 타겟이 있으면 AIController 포커스를 그 액터로 두고 bUseControllerDesiredRotation 으로 전환해 타겟을 바라본 채 이동(strafe)하게 하고, 없으면 포커스를 해제하고 bOrientRotationToMovement(이동 방향으로 회전)로 되돌린다.
	 * 값이 바뀔 때만 동작한다.
	 */
	void SetTargetActor(AActor* NewTarget);

	APawn* GetOwnerPawn() const;

	UBlackboardComponent* GetBlackboard() const;

	// 복귀(리시) Task 가 켜는 억제 플래그. 켜져 있으면 감지 자극을 무시하고 인식을 끈 채로 둔다.
	bool bTargetingSuppressed = false;

	// 사망 태그 콜백을 바인드한 폰의 ASC 와 그 델리게이트 핸들. 언바인드/재빙의 시 정확히 해제하기 위해 보관한다.
	TWeakObjectPtr<UAbilitySystemComponent> DeathBoundASC;
	FDelegateHandle DeathTagDelegateHandle;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
