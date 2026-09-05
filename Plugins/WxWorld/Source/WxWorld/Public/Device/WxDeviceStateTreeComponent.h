// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"
#include "StateTreeExecutionTypes.h"
#if WITH_EDITOR
#include "UObject/PropertyText.h"
#endif
#include "WxDeviceStateTreeComponent.generated.h"

class ACharacter;
class UWxDeviceStateTreeComponent;

/** 최신 상태의 스냅샷이다. 복제 사이의 여러 진입은 최신 진입으로 합쳐지며 과거 연출을 큐로 재생하지 않는다. */
USTRUCT()
struct FWxDeviceStateSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	FName StateTagName;

	/** 0은 권위 상태 미수신이다. 최초 수신은 과거 진입을 재생하지 않는다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	uint32 EntrySerial = 0;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	EStateTreeRunStatus RunStatus = EStateTreeRunStatus::Unset;

	UPROPERTY()
	TObjectPtr<ACharacter> Interactor;

	/** 미해소 NetGUID와 당사자 없는 상태를 구분한다. */
	UPROPERTY()
	bool bHasInteractor = false;
};

/** 기존 활성 프레임이 지워지기 직전에 관측하며 순정 컴포넌트의 틱 깨우기를 유지한다. */
USTRUCT()
struct FWxDeviceExecutionExtension : public FStateTreeExecutionExtension
{
	GENERATED_BODY()

	virtual void ScheduleNextTick(const FContextParameters& Context, const FNextTickArguments& Args) override;
	virtual void OnBeginApplyTransition(const FContextParameters& Context, const FStateTreeTransitionResult& Transition) override;

	UPROPERTY()
	TObjectPtr<UWxDeviceStateTreeComponent> Component;
};

/**
 * 실제 적용 전이와 활성 상태 인스턴스로 진입을 관측한다. 발행 성공은 상태 변경의 근거가 아니다.
 * 태그는 루트 에셋에서 유일한 상태 식별자다. 하위 미태그 시퀀스는 상위 태그 진입 안에서 실행한다.
 * 클라이언트는 스냅샷의 새 진입을 적용하고, 로컬 선행 완료는 같은 스냅샷당 제한된 횟수만 복구한다.
 */
UCLASS()
class UWxDeviceStateTreeComponent : public UStateTreeComponent
{
	GENERATED_BODY()

public:
	UWxDeviceStateTreeComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void StartLogic() override;
	virtual void RestartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FGameplayTag GetStateTag() const;
	FString DescribeSynchronization() const;
	bool IsRestoringState() const;

#if WITH_GAMEPLAY_DEBUGGER
	virtual FString GetDebugInfoString() const override;
#endif

protected:
	UFUNCTION()
	void OnRep_StateSnapshot(const FWxDeviceStateSnapshot& Previous);

	UPROPERTY(VisibleInstanceOnly, Transient, ReplicatedUsing = OnRep_StateSnapshot, Category = "Wx")
	FWxDeviceStateSnapshot StateSnapshot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (GetOptions = "GetInitialStateOptions"))
	FName InitialState;

private:
	friend struct FWxDeviceExecutionExtension;
	friend struct FWxDeviceTestAccess;

	void InstallExecutionObserver();
	void HandleBeginApplyTransition(const FStateTreeExecutionExtension::FContextParameters& Context, const FStateTreeTransitionResult& Transition);
	void ObserveActiveState();
	void Synchronize();
	void PublishAuthorityState();
	void FollowAuthorityState();
	void RequestState(FGameplayTag TargetTag);
	void ApplyInteractor();
	void FailSynchronization(const FString& Reason);
	bool HasState(FGameplayTag Tag) const;

#if WITH_EDITOR
	UFUNCTION()
	TArray<FPropertyTextFName> GetInitialStateOptions() const;
#endif

	FGameplayTag InitialTarget;
	FGameplayTag LastEnteredTag;
	UE::StateTree::FActiveFrameID ObservedFrameID;
	UE::StateTree::FActiveStateID ObservedStateID;
	uint32 LocalEntrySerial = 0;
	uint32 PublishedLocalEntrySerial = 0;
	uint32 AppliedEntrySerial = 0;
	uint32 RequestedAtLocalEntry = 0;
	uint8 SyncAttempts = 0;
	bool bPendingReselect = false;
	bool bRequestPending = false;
	bool bHasAppliedSnapshot = false;
	bool bEndingPlay = false;
	bool bRestoringState = false;
	bool bRequestIsLive = false;
	FString SyncFailure;
};
