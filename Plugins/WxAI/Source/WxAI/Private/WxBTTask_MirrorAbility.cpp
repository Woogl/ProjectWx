// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_MirrorAbility.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

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
	if (Asset.BlackboardAsset) { MirrorTarget.ResolveSelectedKey(*Asset.BlackboardAsset); }
}

FString UWxBTTask_MirrorAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %d ability/montage mappings; listen until aborted"), *MirrorTarget.SelectedKeyName.ToString(), AbilityMappings.Num());
}

EBTNodeResult::Type UWxBTTask_MirrorAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanUp();
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!AI || !AI->HasAuthority()) { return EBTNodeResult::Failed; }
	MirrorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AI->GetPawn());
	if (!MirrorASC.IsValid()) { return EBTNodeResult::Failed; }
	for (const FWxMirrorAbilityMapping& Mapping : AbilityMappings)
	{
		GrantedHandles.Add(Mapping.MirrorAbility ? MirrorASC->GiveAbility(FGameplayAbilitySpec(Mapping.MirrorAbility, 1)) : FGameplayAbilitySpecHandle());
	}
	// OnPossess는 RunBehaviorTree 뒤에 Master를 채운다. 비어 있어도 태스크를 유지한다.
	TickTask(OwnerComp, NodeMemory, 0.f);
	return EBTNodeResult::InProgress;
}

void UWxBTTask_MirrorAbility::StopMirror(FGameplayAbilitySpecHandle Handle)
{
	if (MirrorASC.IsValid() && Handle.IsValid()) { MirrorASC->CancelAbilityHandle(Handle); }
}

void UWxBTTask_MirrorAbility::BindMaster(UAbilitySystemComponent* ASC)
{
	if (ASC == MirrorASC.Get()) { ASC = nullptr; }
	if (MasterASC.Get() == ASC && (ASC || MasterASC.IsExplicitlyNull())) { return; }
	if (MasterASC.IsValid())
	{
		MasterASC->AbilityCommittedCallbacks.RemoveAll(this);
		MasterASC->OnAbilityEnded.RemoveAll(this);
		for (const FGameplayTag& Tag : ObservedEventTags)
		{
			if (FGameplayEventMulticastDelegate* Delegate = MasterASC->GenericGameplayEventCallbacks.Find(Tag)) { Delegate->RemoveAll(this); }
		}
	}
	ObservedEventTags.Reset();
	for (const auto& Pair : Active) { StopMirror(Pair.Value.MirrorHandle); }
	Active.Empty();
	MasterASC = ASC;
	if (!ASC) { return; }
	ASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::HandleCommitted);
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleEnded);
	for (const FWxMirrorAbilityMapping& Mapping : AbilityMappings)
	{
		if (Mapping.SourceEventTag.IsValid() && !ObservedEventTags.HasTagExact(Mapping.SourceEventTag))
		{
			ObservedEventTags.AddTag(Mapping.SourceEventTag);
			ASC->GenericGameplayEventCallbacks.FindOrAdd(Mapping.SourceEventTag).AddUObject(this, &ThisClass::HandleGameplayEvent, Mapping.SourceEventTag);
		}
	}
	FScopedAbilityListLock Lock(*ASC);
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive()) { HandleCommitted(Spec.GetPrimaryInstance()); }
	}
}

void UWxBTTask_MirrorAbility::HandleCommitted(UGameplayAbility* Ability)
{
	if (!Ability) { return; }
	for (const FWxMirrorAbilityMapping& Mapping : AbilityMappings)
	{
		if (Ability->GetClass() == Mapping.SourceAbility && !Mapping.bReplayOnSuccessfulEnd)
		{
			const FGameplayAbilitySpecHandle Handle = Ability->GetCurrentAbilitySpecHandle();
			if (const FWxMirroredAbilityState* Previous = Active.Find(Handle)) { StopMirror(Previous->MirrorHandle); }
			FWxMirroredAbilityState State;
			State.Source = Ability;
			Active.Add(Handle, State);
			// 커밋 콜백은 원본 PlayMontage보다 앞선다. 다음 BT 틱에서 실제 단계를 읽는다.
			return;
		}
	}
}

void UWxBTTask_MirrorAbility::HandleEnded(const FAbilityEndedData& Data)
{
	if (!Data.AbilityThatEnded) { return; }
	if (!Data.bWasCancelled && MirrorASC.IsValid())
	{
		for (int32 Index = 0; Index < AbilityMappings.Num(); ++Index)
		{
			const FWxMirrorAbilityMapping& Mapping = AbilityMappings[Index];
			if (Mapping.bReplayOnSuccessfulEnd && Mapping.SourceAbility == Data.AbilityThatEnded->GetClass())
			{
				MirrorASC->TryActivateAbility(GrantedHandles[Index]);
				break;
			}
		}
	}
	FWxMirroredAbilityState State;
	if (Active.RemoveAndCopyValue(Data.AbilitySpecHandle, State)) { StopMirror(State.MirrorHandle); }
}

void UWxBTTask_MirrorAbility::HandleGameplayEvent(const FGameplayEventData* Data, FGameplayTag EventTag)
{
	if (!Data) { return; }
	for (auto& Pair : Active)
	{
		for (const FWxMirrorAbilityMapping& Mapping : AbilityMappings)
		{
			if (Mapping.SourceEventTag == EventTag && Pair.Value.Source.IsValid() && Mapping.SourceAbility == Pair.Value.Source->GetClass())
			{
				Pair.Value.EventData = *Data;
				Pair.Value.EventData.EventTag = EventTag;
				Pair.Value.bHasEventData = true;
				break;
			}
		}
	}
}

void UWxBTTask_MirrorAbility::Replay(FGameplayAbilitySpecHandle SourceHandle)
{
	FWxMirroredAbilityState* State = Active.Find(SourceHandle);
	if (!State || !State->Source.IsValid() || !State->Source->IsActive()) { return; }
	UGameplayAbility* Source = State->Source.Get();
	UAnimMontage* SourceMontage = MasterASC->GetAnimatingAbility() == Source ? MasterASC->GetCurrentMontage() : nullptr;
	int32 MappingIndex = INDEX_NONE;
	for (int32 Index = 0; Index < AbilityMappings.Num(); ++Index)
	{
		const FWxMirrorAbilityMapping& Mapping = AbilityMappings[Index];
		if (Mapping.SourceAbility == Source->GetClass() && Mapping.SourceMontage == SourceMontage) { MappingIndex = Index; break; }
	}
	if (MappingIndex == INDEX_NONE || (State->bStarted && State->LastMontage == SourceMontage)) { return; }
	const FWxMirrorAbilityMapping& Mapping = AbilityMappings[MappingIndex];
	if (Mapping.SourceEventTag.IsValid() && !State->bHasEventData) { return; }
	StopMirror(State->MirrorHandle);
	const FGameplayAbilitySpecHandle MirrorHandle = GrantedHandles[MappingIndex];
	State->MirrorHandle = MirrorHandle;
	State->LastMontage = SourceMontage;
	State->bStarted = true;
	// 방향을 읽는 회피 어빌리티에는 대형 보정 입력 대신 Master의 실제 입력을 넘긴다.
	APawn* MasterPawn = Cast<APawn>(MasterASC->GetAvatarActor());
	APawn* Pawn = Cast<APawn>(MirrorASC->GetAvatarActor());
	if (Pawn && MasterPawn)
	{
		Pawn->ConsumeMovementInputVector();
		Pawn->AddMovementInput(MasterPawn->GetLastMovementInputVector(), 1.f, true);
		Pawn->ConsumeMovementInputVector();
	}
	const bool bActivated = State->bHasEventData
		? MirrorASC->TriggerAbilityFromGameplayEvent(MirrorHandle, MirrorASC->AbilityActorInfo.Get(), Mapping.SourceEventTag, &State->EventData, *MirrorASC)
		: MirrorASC->TryActivateAbility(MirrorHandle);
	if (!bActivated) { return; }
	ACharacter* MasterCharacter = Cast<ACharacter>(MasterPawn);
	ACharacter* Character = Cast<ACharacter>(Pawn);
	UAnimInstance* SourceAnim = MasterCharacter ? MasterCharacter->GetMesh()->GetAnimInstance() : nullptr;
	UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = MirrorASC->GetCurrentMontage();
	if (SourceMontage && Montage && SourceAnim && Anim)
	{
		Anim->Montage_SetPlayRate(Montage, SourceAnim->Montage_GetPlayRate(SourceMontage));
		Anim->Montage_SetPosition(Montage, SourceAnim->Montage_GetPosition(SourceMontage));
	}
}

void UWxBTTask_MirrorAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* Master = BB ? Cast<AActor>(BB->GetValueAsObject(MirrorTarget.SelectedKeyName)) : nullptr;
	BindMaster(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master));
	if (!MasterASC.IsValid() || !MirrorASC.IsValid()) { return; }
	TArray<FGameplayAbilitySpecHandle> Handles;
	Active.GetKeys(Handles);
	for (FGameplayAbilitySpecHandle Handle : Handles) { Replay(Handle); }
}

void UWxBTTask_MirrorAbility::CleanUp()
{
	BindMaster(nullptr);
	if (MirrorASC.IsValid())
	{
		for (FGameplayAbilitySpecHandle Handle : GrantedHandles)
		{
			StopMirror(Handle);
			MirrorASC->ClearAbility(Handle);
		}
	}
	GrantedHandles.Empty();
	Active.Empty();
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
