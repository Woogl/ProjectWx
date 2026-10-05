// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_MirrorAbility.h"

#include "AbilitySystem/Abilities/WxAbility_Combo.h"
#include "Minion/WxMinionComponent.h"
#include "WxGame.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

/** 통지 구독은 공유 포인터로 걸어, 통지가 도는 동안에는 태스크가 놓아도 이 상태가 살아 있다. */
struct FWxMirrorAbilityState : public TSharedFromThis<FWxMirrorAbilityState>
{
	/** 주인이 바뀌면 이전 주인을 따라 부여한 어빌리티를 걷고 새 주인의 통지를 구독한다. nullptr 이면 구독만 끊는다. */
	void BindMaster(UAbilitySystemComponent* ASC);

	void TickRetry(float DeltaSeconds, float RetryDuration);

	void CleanUp();

	FGameplayTagContainer ExcludedAbilities;

	TWeakObjectPtr<UAbilitySystemComponent> MirrorASC;

private:
	void HandleCommitted(UGameplayAbility* Ability);

	void HandleMasterAbilityEnded(const FAbilityEndedData& Data);

	void ReplayAutomatic(UGameplayAbility* Ability);

	bool TryReplayAbility(FGameplayAbilitySpecHandle Handle, int32 ComboIndex);

	void ClearAutomaticAbilities();

	TWeakObjectPtr<UAbilitySystemComponent> MasterASC;

	/** 주인의 스펙 → 따라 쓰려고 분신에 부여한 스펙. */
	TMap<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle> AutomaticHandles;

	FGameplayAbilitySpecHandle RetryHandle;

	int32 RetryComboIndex = INDEX_NONE;

	float RetryElapsed = 0.f;

	/** 주인이 바뀔 때마다 오른다. 따라 쓰기 도중 주인이 갈렸는지 가린다. */
	uint32 MasterGeneration = 0;

	bool bReplayingAutomatic = false;

	FDelegateHandle CommittedHandle;

	FDelegateHandle AbilityEndedHandle;
};

void FWxMirrorAbilityState::BindMaster(UAbilitySystemComponent* ASC)
{
	if (ASC == MirrorASC.Get())
	{
		ASC = nullptr;
	}
	if (MasterASC.Get() == ASC && (ASC || MasterASC.IsExplicitlyNull()))
	{
		return;
	}

	++MasterGeneration;
	if (UAbilitySystemComponent* PreviousMasterASC = MasterASC.Get())
	{
		PreviousMasterASC->AbilityCommittedCallbacks.Remove(CommittedHandle);
		PreviousMasterASC->OnAbilityEnded.Remove(AbilityEndedHandle);
	}
	CommittedHandle.Reset();
	AbilityEndedHandle.Reset();
	MasterASC.Reset();
	ClearAutomaticAbilities();

	MasterASC = ASC;
	if (!ASC)
	{
		return;
	}
	CommittedHandle = ASC->AbilityCommittedCallbacks.AddSP(this, &FWxMirrorAbilityState::HandleCommitted);
	AbilityEndedHandle = ASC->OnAbilityEnded.AddSP(this, &FWxMirrorAbilityState::HandleMasterAbilityEnded);
}

void FWxMirrorAbilityState::TickRetry(float DeltaSeconds, float RetryDuration)
{
	if (!RetryHandle.IsValid() || !MirrorASC.IsValid())
	{
		return;
	}

	RetryElapsed += DeltaSeconds;
	const bool bActivated = RetryElapsed <= RetryDuration && TryReplayAbility(RetryHandle, RetryComboIndex);
	if (bActivated || RetryElapsed > RetryDuration)
	{
		UE_LOG(LogWxAI, Verbose, TEXT("Mirror retry: %s -> %s after %.3f s, activation %s"), *RetryHandle.ToString(), *GetNameSafe(MirrorASC.IsValid() ? MirrorASC->GetAvatarActor() : nullptr), RetryElapsed, bActivated ? TEXT("accepted") : TEXT("expired"));
		RetryHandle = FGameplayAbilitySpecHandle();
	}
}

void FWxMirrorAbilityState::CleanUp()
{
	BindMaster(nullptr);
	ClearAutomaticAbilities();
	MirrorASC.Reset();
}

void FWxMirrorAbilityState::HandleCommitted(UGameplayAbility* Ability)
{
	if (!Ability || Ability->GetAssetTags().HasAnyExact(ExcludedAbilities))
	{
		return;
	}
	ReplayAutomatic(Ability);
}

void FWxMirrorAbilityState::HandleMasterAbilityEnded(const FAbilityEndedData& Data)
{
	UAbilitySystemComponent* ASC = MirrorASC.Get();
	const FGameplayAbilitySpecHandle MirrorHandle = AutomaticHandles.FindRef(Data.AbilitySpecHandle);
	if (!ASC || !MirrorHandle.IsValid())
	{
		return;
	}

	if (RetryHandle == MirrorHandle)
	{
		RetryHandle = FGameplayAbilitySpecHandle();
		RetryElapsed = 0.f;
	}
	ASC->CancelAbilityHandle(MirrorHandle);
}

void FWxMirrorAbilityState::ReplayAutomatic(UGameplayAbility* Ability)
{
	if (bReplayingAutomatic || !MasterASC.IsValid() || !MirrorASC.IsValid())
	{
		return;
	}
	TGuardValue<bool> ReplayGuard(bReplayingAutomatic, true);

	UAbilitySystemComponent* ASC = MirrorASC.Get();
	const uint32 Generation = MasterGeneration;
	const FGameplayAbilitySpecHandle SourceHandle = Ability->GetCurrentAbilitySpecHandle();
	const FGameplayAbilitySpec* SourceSpec = MasterASC->FindAbilitySpecFromHandle(SourceHandle);
	if (!SourceSpec || !SourceSpec->Ability)
	{
		return;
	}

	const int32 Level = SourceSpec->Level;
	const TSubclassOf<UGameplayAbility> AbilityClass = SourceSpec->Ability->GetClass();
	FGameplayAbilitySpecHandle MirrorHandle = AutomaticHandles.FindRef(SourceHandle);
	FGameplayAbilitySpec* MirrorSpec = ASC->FindAbilitySpecFromHandle(MirrorHandle);
	if (!MirrorSpec)
	{
		// 원본 스펙의 SourceObject나 실행 상태는 주인 소유일 수 있으므로 복사하지 않는다.
		MirrorHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, Level));
		if (Generation != MasterGeneration || MirrorASC.Get() != ASC)
		{
			if (MirrorHandle.IsValid())
			{
				ASC->ClearAbility(MirrorHandle);
			}
			return;
		}
		if (!MirrorHandle.IsValid())
		{
			return;
		}
		AutomaticHandles.Add(SourceHandle, MirrorHandle);
		UE_LOG(LogWxAI, Verbose, TEXT("Mirror grant: %s -> %s, level %d"), *GetNameSafe(AbilityClass.Get()), *GetNameSafe(ASC->GetAvatarActor()), Level);
	}
	else if (MirrorSpec->Level != Level)
	{
		MirrorSpec->Level = Level;
		ASC->MarkAbilitySpecDirty(*MirrorSpec);
	}

	const UWxAbility_Combo* Combo = Cast<UWxAbility_Combo>(Ability);
	const int32 ComboIndex = Combo ? Combo->GetComboIndex() : INDEX_NONE;
	const bool bActivated = TryReplayAbility(MirrorHandle, ComboIndex);
	if (Generation != MasterGeneration || MirrorASC.Get() != ASC || !Ability->IsActive())
	{
		return;
	}

	// 분신 자체의 피격·차단으로 거절된 요청은 주인이 종료하기 전까지만 재시도한다.
	RetryHandle = bActivated ? FGameplayAbilitySpecHandle() : MirrorHandle;
	RetryComboIndex = ComboIndex;
	RetryElapsed = 0.f;
	UE_LOG(LogWxAI, Verbose, TEXT("Mirror commit: %s -> %s, activation %s"), *GetNameSafe(AbilityClass.Get()), *GetNameSafe(ASC->GetAvatarActor()), bActivated ? TEXT("accepted") : TEXT("rejected"));
}

bool FWxMirrorAbilityState::TryReplayAbility(FGameplayAbilitySpecHandle Handle, int32 ComboIndex)
{
	UAbilitySystemComponent* ASC = MirrorASC.Get();
	if (ComboIndex != INDEX_NONE)
	{
		const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
		UWxAbility_Combo* Combo = Spec ? Cast<UWxAbility_Combo>(Spec->GetPrimaryInstance()) : nullptr;
		return Combo && Combo->TryMirrorComboStep(ComboIndex);
	}
	return ASC && ASC->TryActivateAbility(Handle, false);
}

void FWxMirrorAbilityState::ClearAutomaticAbilities()
{
	TMap<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle> Handles = MoveTemp(AutomaticHandles);
	AutomaticHandles.Reset();
	RetryHandle = FGameplayAbilitySpecHandle();
	if (UAbilitySystemComponent* ASC = MirrorASC.Get())
	{
		for (const TPair<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle>& Pair : Handles)
		{
			ASC->CancelAbilityHandle(Pair.Value);
			ASC->ClearAbility(Pair.Value);
		}
		UE_LOG(LogWxAI, Verbose, TEXT("Mirror cleanup: %s, %d automatic specs"), *GetNameSafe(ASC->GetAvatarActor()), Handles.Num());
	}
}

FWxStateTreeTask_MirrorAbility::FWxStateTreeTask_MirrorAbility()
{
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_MirrorAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;

	Instance.State = MakeShared<FWxMirrorAbilityState>();
	Instance.State->ExcludedAbilities = Instance.ExcludedAbilities;
	Instance.State->MirrorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);

	// 첫 틱을 기다리지 않고 구독해, 진입한 프레임의 주인 커밋부터 따라 쓴다.
	Instance.State->BindMaster(Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(UWxMinionComponent::GetMaster(*Pawn)) : nullptr);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MirrorAbility::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 따라 쓴 어빌리티가 트리를 멈춰 인스턴스 데이터가 사라져도 이 틱이 끝날 때까지는 상태를 쥔다.
	const TSharedPtr<FWxMirrorAbilityState> State = Instance.State;
	if (!State)
	{
		return EStateTreeRunStatus::Running;
	}

	const AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	State->BindMaster(Pawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(UWxMinionComponent::GetMaster(*Pawn)) : nullptr);
	State->TickRetry(DeltaTime, Instance.RetryDuration);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_MirrorAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (const TSharedPtr<FWxMirrorAbilityState> State = MoveTemp(Instance.State))
	{
		State->CleanUp();
	}
}
