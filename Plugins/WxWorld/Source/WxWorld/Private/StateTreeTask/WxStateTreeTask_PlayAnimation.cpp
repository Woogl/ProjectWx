// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_PlayAnimation.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "WxWorldModule.h"

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	Instance.Mesh = Cast<USkeletalMeshComponent>(Instance.TargetMesh.Resolve(Owner));

	USkeletalMeshComponent* Mesh = Instance.Mesh;
	if (!Mesh || !Instance.Animation)
	{
		UE_LOG(LogWxWorld, Error, TEXT("Play Animation: %s 에서 메시 '%s' 를 찾지 못했거나 애니메이션이 비었다."), *GetNameSafe(Owner), *Instance.TargetMesh.Name.ToString());
		return EStateTreeRunStatus::Failed;
	}

	Mesh->PlayAnimation(Instance.Animation, false);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USkeletalMeshComponent* Mesh = Instance.Mesh;
	if (!Mesh || !Instance.Animation)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 다른 애니로 교체된 경우도 종료로 본다.
	const UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance();
	const bool bStillPlaying = SingleNode && SingleNode->GetAnimationAsset() == Instance.Animation && SingleNode->IsPlaying();
	return bStillPlaying ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayAnimation::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("애니메이션 재생 ({0})"),
		InstanceData->Animation ? FText::FromString(InstanceData->Animation->GetName()) : INVTEXT("none"));
}
#endif
