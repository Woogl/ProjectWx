// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_MirrorAbility.h"
#include "WxAIModule.h"
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
	MirrorTarget.SelectedKeyName = TEXT("Master");
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
	return FString::Printf(TEXT("%s의 어빌리티 발동을 따라합니다."), *MirrorTarget.SelectedKeyName.ToString());
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanUp();
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI || !AI->HasAuthority())
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
	}
	MasterASC.Reset();
	ClearAutomaticAbilities();
	MasterASC = ASC;
	if (!ASC)
	{
		return;
	}
	ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleCommitted);
}

void UWxBTTask_MirrorAbility::HandleCommitted(UGameplayAbility* Ability)
{
	if (!Ability || IsExcluded(Ability))
	{
		return;
	}
	ReplayAutomatic(Ability);
}

bool UWxBTTask_MirrorAbility::IsExcluded(const UGameplayAbility* Ability) const
{
	return ExcludedAbilities.ContainsByPredicate([Ability](const FGameplayTagContainer& Tags)
	{
		return Ability->GetAssetTags().HasAnyExact(Tags);
	});
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
	// 콤보 상태·대상 이벤트를 추측하지 않고 일반 발동 조건을 그대로 적용한다.
	const bool bActivated = ASC->TryActivateAbility(MirrorHandle, false);
	UE_LOG(LogWxAI, Verbose, TEXT("Mirror commit: %s -> %s, activation %s"), *GetNameSafe(AbilityClass.Get()), *GetNameSafe(ASC->GetAvatarActor()), bActivated ? TEXT("accepted") : TEXT("rejected"));
}

void UWxBTTask_MirrorAbility::ClearAutomaticAbilities()
{
	TMap<FGameplayAbilitySpecHandle, FGameplayAbilitySpecHandle> Handles = MoveTemp(AutomaticHandles);
	AutomaticHandles.Reset();
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
