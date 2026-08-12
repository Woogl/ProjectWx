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

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 재생 없이 곧바로 완료한다(발동 순간에만 재생; InteractingCharacter 는 비영속이라 복원 시 비어 있음).
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 당사자는 오너 기믹이 권위에서 쓰고 복제하는 값이라 모든 피어가 같은 대상을 본다(에셋 배선 없음).
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	const ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	// 대상/몽타주/애님인스턴스가 없으면 상태가 갇히지 않게 곧바로 완료한다.
	if (!AnimInstance || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 각 머신이 메시 AnimInstance 로 로컬 재생한다(복제형 PlayAnimMontage 아님 — 모든 피어가 각자 재생해 중복이 없다).
	AnimInstance->Montage_Play(Instance.Montage);

	// Tick 이 종료를 폴링해 완료시킨다.
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

	// 재생이 끝나면(또는 다른 몽타주로 교체되면) 상태를 완료시킨다.
	return AnimInstance->Montage_IsPlaying(Instance.Montage) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayInteractorMontage::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Play Interactor Montage ({0})"),
		InstanceData->Montage ? FText::FromString(InstanceData->Montage->GetName()) : INVTEXT("none"));
}
#endif
