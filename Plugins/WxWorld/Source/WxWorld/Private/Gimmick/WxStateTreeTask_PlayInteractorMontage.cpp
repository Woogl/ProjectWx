// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_PlayInteractorMontage.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_PlayInteractorMontage::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	const ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	if (!AnimInstance || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AnimInstance->Montage_Play(Instance.Montage);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayInteractorMontage::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	const ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	if (!Character)
	{
		return EStateTreeRunStatus::Failed;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return AnimInstance->Montage_IsPlaying(Instance.Montage) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayInteractorMontage::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("상호작용자 몽타주 재생 ({0})"),
		InstanceData->Montage ? FText::FromString(InstanceData->Montage->GetName()) : INVTEXT("none"));
}
#endif
