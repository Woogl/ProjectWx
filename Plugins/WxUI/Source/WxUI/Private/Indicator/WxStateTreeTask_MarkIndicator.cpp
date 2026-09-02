// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxStateTreeTask_MarkIndicator.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Indicator/WxIndicator.h"
#include "Misc/CoreMisc.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxUIModule.h"

FWxStateTreeTask_MarkIndicator::FWxStateTreeTask_MarkIndicator()
{
	// 재선택마다 재진입하면 ExitState 가 인디케이터를 걷어가 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;

	// 완료 없이 매 프레임 도는 태스크라 바인딩 복사가 그대로 프레임 비용이 된다.
	// 대상과 높이는 진입 시 한 번 받아 두면 되는 저작값이다.
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicator::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.SpawnedIndicator.Reset();

	// 대역 좌표는 로케이터를 지정해야 기록되므로, 빈 로케이터는 가리킬 곳이 아예 없다는 뜻이다.
	if (Instance.Target.IsEmpty())
	{
		UE_LOG(LogWxUI, Warning, TEXT("Mark Indicator: 가리킬 대상이 지정되지 않음."));
		return EStateTreeRunStatus::Running;
	}

	if (!Instance.IndicatorClass)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Mark Indicator: 띄울 인디케이터가 지정되지 않음."));
		return EStateTreeRunStatus::Running;
	}

	// 그릴 화면이 없는 머신에서는 틱만 도는 액터가 된다.
	if (IsRunningDedicatedServer())
	{
		return EStateTreeRunStatus::Running;
	}

	UWorld* World = Context.GetWorld();
	if (!World)
	{
		return EStateTreeRunStatus::Running;
	}

	// 대상이 아직 언로드돼 있을 수 있으므로 기록해 둔 좌표에 먼저 띄우고, 해석되면 그때 부착한다.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWxIndicator* Indicator = World->SpawnActor<AWxIndicator>(Instance.IndicatorClass, FTransform(Instance.TargetLocation), SpawnParameters);
	if (!Indicator)
	{
		return EStateTreeRunStatus::Running;
	}

	Indicator->Initialize(Instance.WorldZOffset);
	Instance.SpawnedIndicator = Indicator;

	// 첫 부착을 Tick 까지 미루면 대상이 이미 로드돼 있어도 한 프레임을 기록 좌표에서 보낸다.
	RefreshTarget(Context, Instance);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	RefreshTarget(Context, Instance);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_MarkIndicator::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (AWxIndicator* Indicator = Instance.SpawnedIndicator.Get())
	{
		Indicator->Destroy();
	}
	Instance.SpawnedIndicator.Reset();
}

void FWxStateTreeTask_MarkIndicator::RefreshTarget(const FStateTreeExecutionContext& Context, FInstanceDataType& Instance) const
{
	AWxIndicator* Indicator = Instance.SpawnedIndicator.Get();

	// 이미 잡고 있으면 해석을 돌리지 않는다 — 정상 표시 중에는 SyncFind 비용이 아예 들지 않는다.
	// 대상이 언로드·파괴되면 엔진이 부착을 풀어 주므로, 그때부터 다시 매 틱 재시도한다.
	if (!Indicator || Indicator->HasTarget())
	{
		return;
	}

	// 빈 로케이터·미로드(스트리밍 아웃)·파괴 대기는 전부 미해석이다.
	AActor* Target = Cast<AActor>(Instance.Target.SyncFind(Context.GetOwner()));
	if (!IsValid(Target))
	{
		return;
	}

	Indicator->SetTarget(Target);
}

#if WITH_EDITOR
void FWxStateTreeTask_MarkIndicator::PostEditInstanceDataChangeChainProperty(const FPropertyChangedChainEvent& PropertyChangedEvent, FStateTreeDataView InstanceDataView)
{
	if (PropertyChangedEvent.GetMemberPropertyName() != GET_MEMBER_NAME_CHECKED(FInstanceDataType, Target))
	{
		return;
	}

	FInstanceDataType& Instance = InstanceDataView.GetMutable<FInstanceDataType>();

	// 지정을 지우거나 해석되지 않는 대상으로 바꾸면 앞 대상의 좌표가 남아선 안 된다.
	const AActor* Target = Cast<AActor>(Instance.Target.SyncFind());
	Instance.TargetLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
}

FText FWxStateTreeTask_MarkIndicator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("인디케이터 표시 ({0})"), FWxLocatorUtils::GetDisplayName(InstanceData->Target));
}
#endif
