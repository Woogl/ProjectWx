// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_PlayAnimation.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USkeletalMeshComponent* Mesh = Instance.TargetMesh;
	if (!Mesh || !Instance.Animation)
	{
		return EStateTreeRunStatus::Failed;
	}

	Mesh->PlayAnimation(Instance.Animation, false);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayAnimation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	USkeletalMeshComponent* Mesh = Instance.TargetMesh;
	if (!Mesh || !Instance.Animation)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 논루프 재생이 끝나면 싱글노드가 멈추므로(또는 애니가 교체되면) 그 시점에 완료한다.
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
