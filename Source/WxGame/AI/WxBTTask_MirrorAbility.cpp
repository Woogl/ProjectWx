// Copyright Woogle. All Rights Reserved.

#include "AI/WxBTTask_MirrorAbility.h"
#include "AbilitySystem/Abilities/WxAbility_Combo.h"
#include "WxGame.h"
#include "AI/WxBlackboardKeys.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UWxBTTask_MirrorAbility::UWxBTTask_MirrorAbility()
{
	NodeName = TEXT("Mirror Ability");
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	MirrorTarget.SelectedKeyName = WxBlackboardKeys::Master;
	MirrorTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, MirrorTarget), AActor::StaticClass());
}

void UWxBTTask_MirrorAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (Asset.BlackboardAsset)
	{
		MirrorTarget.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

FString UWxBTTask_MirrorAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s의 어빌리티 발동과 종료를 따라합니다."), *MirrorTarget.SelectedKeyName.ToString());
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanUp();
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI)
	{
		return EBTNodeResult::Failed;
	}
	MirrorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!MirrorASC.IsValid())
	{
		return EBTNodeResult::Failed;
	}
	// OnPossess는 RunBehaviorTree 뒤에 Master를 채운다. 비어 있어도 태스크를 유지한다.
	TickTask(OwnerComp, NodeMemory, 0.f);
	return EBTNodeResult::InProgress;
}

void UWxBTTask_MirrorAbility::BindMaster(UAbilitySystemComponent* ASC)
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
	if (MasterASC.IsValid())
	{
		MasterASC->AbilityCommittedCallbacks.RemoveAll(this);
		MasterASC->OnAbilityEnded.RemoveAll(this);
	}
	MasterASC.Reset();
	ClearAutomaticAbilities();
	MasterASC = ASC;
	if (!ASC)
	{
		return;
	}
	ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleCommitted);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleMasterAbilityEnded);
}

void UWxBTTask_MirrorAbility::HandleMasterAbilityEnded(const FAbilityEndedData& Data)
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

void UWxBTTask_MirrorAbility::HandleCommitted(UGameplayAbility* Ability)
{
	if (!Ability || Ability->GetAssetTags().HasAnyExact(ExcludedAbilities))
	{
		return;
	}
	ReplayAutomatic(Ability);
}

void UWxBTTask_MirrorAbility::ReplayAutomatic(UGameplayAbility* Ability)
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
		// 원본 스펙의 SourceObject나 실행 상태는 Master 소유일 수 있으므로 복사하지 않는다.
		MirrorHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, Level));
		if (Generation != MasterGeneration || MirrorASC.Get() != ASC)
		{
			if (MirrorHandle.IsValid()) { ASC->ClearAbility(MirrorHandle); }
			return;
		}
		if (!MirrorHandle.IsValid()) { return; }
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
	// 분신 자체의 피격·차단으로 거절된 요청은 마스터가 종료하기 전까지만 재시도한다.
	RetryHandle = bActivated ? FGameplayAbilitySpecHandle() : MirrorHandle;
	RetryComboIndex = ComboIndex;
	RetryElapsed = 0.f;
	UE_LOG(LogWxAI, Verbose, TEXT("Mirror commit: %s -> %s, activation %s"), *GetNameSafe(AbilityClass.Get()), *GetNameSafe(ASC->GetAvatarActor()), bActivated ? TEXT("accepted") : TEXT("rejected"));
}

bool UWxBTTask_MirrorAbility::TryReplayAbility(FGameplayAbilitySpecHandle Handle, int32 ComboIndex)
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

void UWxBTTask_MirrorAbility::ClearAutomaticAbilities()
{
	TMap<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle> Handles = MoveTemp(AutomaticHandles);
	AutomaticHandles.Reset();
	RetryHandle = FGameplayAbilitySpecHandle();
	if (UAbilitySystemComponent* ASC = MirrorASC.Get())
	{
		for (const auto& Pair : Handles)
		{
			ASC->CancelAbilityHandle(Pair.Value);
			ASC->ClearAbility(Pair.Value);
		}
		UE_LOG(LogWxAI, Verbose, TEXT("Mirror cleanup: %s, %d automatic specs"), *GetNameSafe(ASC->GetAvatarActor()), Handles.Num());
	}
}

void UWxBTTask_MirrorAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Master = BB ? Cast<AActor>(BB->GetValueAsObject(MirrorTarget.SelectedKeyName)) : nullptr;
	BindMaster(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master));

	if (!RetryHandle.IsValid() || !MirrorASC.IsValid())
	{
		return;
	}
	RetryElapsed += DeltaSeconds;
	const bool bActivated = RetryElapsed <= RetryDuration && TryReplayAbility(RetryHandle, RetryComboIndex);
	if (bActivated || RetryElapsed > RetryDuration)
	{
		UE_LOG(LogWxAI, Verbose, TEXT("Mirror retry: %s -> %s after %.3f s, activation %s"), *RetryHandle.ToString(), *GetNameSafe(MirrorASC->GetAvatarActor()), RetryElapsed, bActivated ? TEXT("accepted") : TEXT("expired"));
		RetryHandle = FGameplayAbilitySpecHandle();
	}
}

void UWxBTTask_MirrorAbility::CleanUp()
{
	BindMaster(nullptr);
	ClearAutomaticAbilities();
	MirrorASC.Reset();
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanUp();
	return EBTNodeResult::Aborted;
}

void UWxBTTask_MirrorAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type Result)
{
	CleanUp();
	
	Super::OnTaskFinished(OwnerComp, NodeMemory, Result);
}
