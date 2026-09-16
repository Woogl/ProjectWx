// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WxBTTask_MirrorAbility.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UAnimMontage;
struct FAbilityEndedData;

/** 원본의 실제 몽타주로 콤보 단계를 구분한다. 몽타주 없는 지속 행동은 SourceMontage를 비운다. */
USTRUCT(BlueprintType)
struct WXAI_API FWxMirrorAbilityMapping
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wx|AI")
	TSubclassOf<UGameplayAbility> SourceAbility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wx|AI")
	TObjectPtr<UAnimMontage> SourceMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wx|AI")
	TSubclassOf<UGameplayAbility> MirrorAbility;
	/** 대상 등 이벤트 문맥이 필요한 행동. Master가 성공한 발동에만 전달한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wx|AI")
	FGameplayTag SourceEventTag;
	/** 커밋 없이 동기 종료하는 패시브에 사용한다. 취소된 발동은 복제하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wx|AI")
	bool bReplayOnSuccessfulEnd = false;
};

USTRUCT()
struct FWxMirroredAbilityState
{
	GENERATED_BODY()
	TWeakObjectPtr<UGameplayAbility> Source;
	FGameplayAbilitySpecHandle MirrorHandle;
	TWeakObjectPtr<UAnimMontage> LastMontage;
	bool bStarted = false;
	UPROPERTY()
	FGameplayEventData EventData;
	bool bHasEventData = false;
};

/** BT 실행 중 Master의 커밋/종료를 구독한다. 발동 데이터는 분신 전용 에셋에서 지정한다. */
UCLASS()
class WXAI_API UWxBTTask_MirrorAbility : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UWxBTTask_MirrorAbility();
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type Result) override;
protected:
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	FBlackboardKeySelector MirrorTarget;
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	TArray<FWxMirrorAbilityMapping> AbilityMappings;
private:
	void BindMaster(UAbilitySystemComponent* ASC);
	void HandleCommitted(UGameplayAbility* Ability);
	void HandleEnded(const FAbilityEndedData& Data);
	void HandleGameplayEvent(const FGameplayEventData* Data, FGameplayTag EventTag);
	void CleanUp();
	void StopMirror(FGameplayAbilitySpecHandle Handle);
	void Replay(FGameplayAbilitySpecHandle SourceHandle);
	TWeakObjectPtr<UAbilitySystemComponent> MasterASC;
	TWeakObjectPtr<UAbilitySystemComponent> MirrorASC;
	UPROPERTY(Transient)
	TMap<FGameplayAbilitySpecHandle, FWxMirroredAbilityState> Active;
	TArray<FGameplayAbilitySpecHandle> GrantedHandles;
	FGameplayTagContainer ObservedEventTags;
};
