// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "WxAIPerceptionComponent.generated.h"

class APawn;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 시각·청각·피격 감지를 갖춘다. Damage 센스에는 피아 필터가 없어 적대 가해자만 직접 보고한다.
 *
 * 감지·인식까지가 이 컴포넌트의 범위다. 감지한 것 중 누구를 적으로 삼을지는 UWxBTService_UpdateTargetActor 가, 그 타겟을 바라볼지는 UWxBTService_LockOn 이 정한다.
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

private:
	/**
	 * 폰의 UWxAIBehaviorComponent 가 정한 감각 수치를 센스 설정에 옮긴다.
	 * 그 컴포넌트가 없거나 폰이 없으면 클래스 기본값으로 되돌려, 이전 폰의 수치가 남지 않게 한다.
	 */
	void ApplySenseSettings(const APawn* Pawn);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/**
	 * 폰이 받은 적대 대미지를 촉각(Damage 센스)으로 보고한다. 시야·청각이 놓치는 가해자도 이 경로로 잡힌다.
	 * 자극은 피격 액터(폰)로 리스너를 역추적해 이 컴포넌트에 닿는다.
	 */
	void HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload);

	void BindPawnHit(APawn* Pawn);
	void UnbindPawnHit();

	APawn* GetOwnerPawn() const;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle PawnHitDelegateHandle;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;
};
