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

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnPerceptionTargetChanged, AActor* /*NewTarget*/);

/**
 * 시각·청각·피격 감지를 Blackboard TargetActor에 동기화한다.
 * 타겟은 사망·파괴나 리시 복귀에서만 해제하며, Damage 센스에는 피아 필터가 없어 적대 가해자만 직접 보고한다.
 *
 * 감지·인식까지가 이 컴포넌트의 범위다. 그 타겟을 바라볼지(컨트롤러 포커스·폰 회전 모드)는 UWxBTService_LockOn 이 단독으로 정한다.
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

	/**
	 * 현재 타겟의 Perception 기록과 Blackboard 를 정리한다.
	 * 이후 자극은 평소 감지 경로를 통해 다시 타겟을 획득할 수 있다.
	 */
	void ForgetTargetActor();

	FWxOnPerceptionTargetChanged OnTargetChanged;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** 추적 중인 타겟이 죽으면 타겟을 정리한다. 시체는 파괴되지 않고 시야에 남아 자극만으로는 재판정이 오지 않는다. */
	void HandleTargetDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 추적 중인 타겟이 사라지면 타겟을 정리한다. 파괴는 ASC 도 함께 없애 사망 태그로는 잡히지 않고, 엔진도 소스가 무효하면 감지 갱신을 방송하지 않는다. */
	UFUNCTION()
	void HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 현재 타겟의 사망·EndPlay 구독과 AppliedTarget 기록을 함께 갱신한다. */
	void BindTargetLoss(AActor* NewTarget);
	void UnbindTargetLoss();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/**
	 * 폰이 받은 적대 대미지를 촉각(Damage 센스)으로 보고한다. 시야·청각이 놓치는 가해자도 타겟이 비어 있으면 이 경로로 잡힌다.
	 * 자극은 피격 액터(폰)로 리스너를 역추적해 이 컴포넌트에 닿는다.
	 */
	void HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	void BindPawnHit(APawn* Pawn);
	void UnbindPawnHit();

	/** TargetActor 와 타겟 소실 감시를 함께 갱신하며, 자기 폰은 타겟으로 받지 않는다. */
	void SetTargetActor(AActor* NewTarget);

	bool IsActorDead(AActor* Actor);

	APawn* GetOwnerPawn() const;

	UBlackboardComponent* GetBlackboard() const;

	// 마지막으로 적용한 타겟으로, 중복 적용 방지와 소실 구독 해제에 쓴다.
	// 블랙보드 약참조는 파괴 시 nullptr로 바뀌므로, 유효성과 무관한 TObjectKey로 보관한다.
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
