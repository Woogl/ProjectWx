// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_SpawnNiagara.h"

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_SpawnNiagara::FWxStateTreeTask_SpawnNiagara()
{
	// 진입 시 1회 재생만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SpawnNiagara::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 진입 경로(라이브 전이/초기 시작/복원/레이트조인)를 가리지 않고 판단 기준은 하나다 — 이 노드가 띄운 FX 가 아직 재생 중이면 그대로 두고 통과한다.
	// 루프 FX(지속 FX)는 계속 미완료라 여기서 걸려 이미터가 겹쳐 쌓이지 않고, 재생이 끝났거나 아직 띄운 적이 없으면(로드·복원 직후) 아래에서 다시 띄운다.
	if (IsValid(Instance.SpawnedComponent) && !Instance.SpawnedComponent->IsComplete())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Instance.Niagara)
	{
		// attach 대상이 있으면 그 컴포넌트(지정 시 그 소켓)에 붙여 재생, 없으면 액터 위치에 재생.
		// 어느 쪽이든 RelativeLocation 을 더해, 소켓이 없는 메시에서도 불꽃 높이 같은 부착 지점을 에셋에서 잡을 수 있게 한다.
		if (Instance.AttachComponent)
		{
			Instance.SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(Instance.Niagara, Instance.AttachComponent, Instance.AttachSocketName, Instance.RelativeLocation, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		}
		else
		{
			Instance.SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Owner, Instance.Niagara, Owner->GetActorTransform().TransformPosition(Instance.RelativeLocation));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SpawnNiagara::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("나이아가라 스폰 ({0})"),
		InstanceData->Niagara ? FText::FromString(InstanceData->Niagara->GetName()) : INVTEXT("none"));
}
#endif
