// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "UObject/ObjectKey.h"
#include "WxAIPerceptionComponent.generated.h"

class APawn;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBlackboardComponent;
class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * AIController 에 부착해 사용하는 Perception 컴포넌트.
 *
 * Sight/Hearing/Damage 감지를 셋업하고 결과를 Blackboard 의 TargetActor 에 동기화한다.
 * 세 센스의 수치는 이 컴포넌트가 소유한다 — 폰별로 달리 쓰는 곳이 없어 주입 경로를 두지 않는다.
 * 세 센스 모두 감지 성공 시 그 액터(소리 발생원 포함)를 TargetActor 로 확정한다.
 *
 * TargetActor 의 유무에 따라 폰의 회전 모드도 함께 발행한다 — 타겟이 있으면 그 액터를 바라본 채 이동(strafe), 없으면 폰의 기본 회전 모드로 원복(평상시).
 *
 * 한 번 확보한 TargetActor 는 시야를 잠시 잃어도(보스 등 뒤로 이동, 벽 뒤 등) 유지되며, 리시 이탈 판정과 복귀는 BT(UWxBTDecorator_BeyondLeash + UWxBTTask_ReturnHome)가 담당한다 — 복귀 Task 가 SetTargetingSuppressed 로 타겟을 비우고 복귀 중 재감지를 억제한다.
 * 다만 타겟이 죽거나 파괴되면 즉시 비운다. 시체는 파괴되지 않고 시야에 남고 파괴는 감지 이벤트를 남기지 않으므로, 자극이 아니라 타겟의 사망 태그·EndPlay 를 구독해 그 시점을 잡는다.
 * 인식(State.InCombat)도 같은 수명을 따른다 — 추적 중이면 on, 복귀(억제)·타겟 소실 시 off 이며, 서버에서 폰 ASC 에 MinimalReplication 태그로 발행되어 네임플레이트 표시에 소비된다.
 *
 * 폰이 받은 피격(Event.Hit)은 이 컴포넌트가 폰 ASC 를 구독해 촉각(Damage 센스)으로 보고한다 — 빙의가 바뀌면 구독을 새 폰으로 옮긴다.
 * 보고 대상은 적대 가해자로 한정한다. Sight·Hearing 은 엔진의 DetectionByAffiliation 이 피아를 가르지만 Damage 센스에는 그 설정이 없어, 이 컴포넌트가 보고 시점에 직접 가른다.
 *
 * 자기 폰의 사망은 다루지 않는다. 이 태그의 소비자(네임플레이트의 표시 정책, 뒤잡 자격 판정)가 저마다 사망을 먼저 걸러내므로 시체 위에 태그가 남아도 관측되지 않는다.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class WXAI_API UWxAIPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:
	UWxAIPerceptionComponent();

	virtual void PostInitProperties() override;

	/** 빙의 변경을 구독하고, 배치된 폰처럼 BeginPlay 전에 이미 빙의된 폰은 그 자리에서 바인드한다. */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 타겟팅 억제(disengage)를 켜고 끈다. 리시 복귀 Task(UWxBTTask_ReturnHome)가 복귀 진입/종료에 호출한다.
	 *  - true: 현재 TargetActor 를 비우고 인식을 끄며 회전 모드를 원복하고, 이후 감지 자극을 무시해 복귀 중 재-어그로를 막는다.
	 *  - false: 억제를 풀고, 그 시점에 보이는(Sight) 대상 중 가장 가까운 쪽을 재획득한다. Sight 는 감지 여부가 바뀔 때만 갱신을 방송하므로, 억제 중 계속 보이던 대상은 자극을 기다려선 영영 다시 잡히지 않는다.
	 *    청각·촉각은 판정에서 뺀다 — 해제 이벤트가 없어 한 번 싸운 상대가 시야 밖에서도 계속 잡히면 복귀가 곧바로 재추격으로 되돌아간다.
	 */
	void SetTargetingSuppressed(bool bSuppressed);

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** 추적 중인 타겟이 죽으면 타겟과 인식을 정리한다. 시체는 파괴되지 않고 시야에 남아 자극만으로는 재판정이 오지 않는다. */
	void HandleTargetDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 추적 중인 타겟이 사라지면 타겟과 인식을 정리한다. 파괴는 ASC 도 함께 없애 사망 태그로는 잡히지 않고, 엔진도 소스가 무효하면 감지 갱신을 방송하지 않는다. */
	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 현재 타겟의 사망 태그와 EndPlay 에 정리 콜백을 바인드/언바인드하고, 적용 기록(AppliedTarget)을 함께 옮긴다. 타겟이 바뀔 때마다 SetTargetActor 가 교체한다. */
	void BindTargetLoss(AActor* NewTarget);
	void UnbindTargetLoss();

	/** 빙의가 바뀌면 피격 구독을 새 폰의 ASC 로 옮긴다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/**
	 * 폰이 받은 적대 대미지를 촉각(Damage 센스)으로 보고해 가해자를 즉시 TargetActor 로 인지하게 한다.
	 * 자극은 피격 액터(폰)로 리스너를 역추적해 이 컴포넌트에 닿는다.
	 */
	void HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	/** 폰 ASC 의 Event.Hit(자식 포함) 구독을 걸고 푼다. */
	void BindPawnHit(APawn* Pawn);
	void UnbindPawnHit();

	/**
	 * 인식/추적 상태를 판정하는 단일 지점.
	 * 인식을 바꿀 수 있는 모든 경로(감지 갱신, 억제 진입, 타겟 소실)가 여기로만 들어온다. 리시 이탈 판정은 BT 로 이관되어 여기서 다루지 않는다.
	 */
	void UpdateRecognition();

	/** UpdateRecognition 의 결정을 적용하는 순수 setter. 전환에서만 동작한다. */
	void SetRecognized(bool bNewRecognized);

	/**
	 * TargetActor 를 설정/해제하는 단일 지점. BB 키 쓰기와 함께 회전 모드(전투 시 strafe)를 발행하고, 타겟 소실 감시를 새 타겟으로 옮긴다.
	 * 값이 바뀔 때만 동작한다.
	 */
	void SetTargetActor(AActor* NewTarget);

	/** 액터가 사망 상태(Ability.Death)인지. ASC 가 없는 액터는 사망 개념이 없으므로 false. */
	static bool IsActorDead(AActor* Actor);

	APawn* GetOwnerPawn() const;

	UBlackboardComponent* GetBlackboard() const;

	// 복귀(리시) Task 가 켜는 억제 플래그. 켜져 있으면 감지 자극을 무시하고 인식을 끈 채로 둔다.
	bool bTargetingSuppressed = false;

	// 마지막으로 적용한 타겟. SetTargetActor 의 중복 적용 가드 기준이자, 소실 콜백을 해제할 대상이다.
	// 블랙보드 Object 키와 약참조는 대상이 파괴되면 비교에서 nullptr 과 같아져 "이미 해제됨" 으로 오판하므로, 유효성과 무관하게 식별자만 비교하는 오브젝트 키로 든다.
	TObjectKey<AActor> AppliedTarget;
	FDelegateHandle TargetDeathTagDelegateHandle;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle PawnHitDelegateHandle;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
