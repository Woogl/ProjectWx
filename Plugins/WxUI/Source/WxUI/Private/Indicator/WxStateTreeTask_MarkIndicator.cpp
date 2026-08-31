// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxStateTreeTask_MarkIndicator.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "WxLocatorUtils.h"
#include "WxUIModule.h"

namespace
{
	/** 빈 로케이터·미로드(스트리밍 아웃)·파괴 대기는 nullptr. */
	AActor* ResolveTargetActor(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		AActor* Target = Cast<AActor>(Locator.SyncFind(Owner));
		return IsValid(Target) ? Target : nullptr;
	}

	UWxIndicatorManagerComponent* FindIndicatorManager(const AActor* Owner)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
		return PlayerController ? PlayerController->FindComponentByClass<UWxIndicatorManagerComponent>() : nullptr;
	}

	void UnregisterIndicator(TWeakObjectPtr<UWxIndicatorDescriptor>& RegisteredIndicator)
	{
		if (UWxIndicatorDescriptor* Indicator = RegisteredIndicator.Get())
		{
			Indicator->Unregister();
		}
		RegisteredIndicator.Reset();
	}

	void RefreshIndicator(const FStateTreeExecutionContext& Context, FWxStateTreeTask_MarkIndicatorInstanceData& Instance)
	{
		// 빈 로케이터는 좌표도 기록되지 않았으므로 대역으로 갈 곳이 없다.
		if (Instance.Target.IsEmpty())
		{
			return;
		}

		// 언로드·파괴로 대상을 놓치면 등록증이 스스로 좌표로 내려앉아 다시 여기로 온다.
		const UWxIndicatorDescriptor* Indicator = Instance.RegisteredIndicator.Get();
		if (Indicator && Indicator->GetTargetComponent())
		{
			return;
		}

		AActor* Owner = Cast<AActor>(Context.GetOwner());
		AActor* Target = ResolveTargetActor(Instance.Target, Owner);
		USceneComponent* TargetComponent = Target ? Target->GetRootComponent() : nullptr;

		if (Indicator && !TargetComponent)
		{
			return;
		}

		UnregisterIndicator(Instance.RegisteredIndicator);

		// 매니저는 아직 없을 수 있다(폰·컨트롤러 스폰 전).
		UWxIndicatorManagerComponent* Manager = FindIndicatorManager(Owner);
		if (!Manager)
		{
			return;
		}

		const FVector WorldOffset(0.f, 0.f, Instance.WorldZOffset);
		Instance.RegisteredIndicator = Manager->AddIndicator(TargetComponent, Instance.TargetLocation, WorldOffset);
	}
}

FWxStateTreeTask_MarkIndicator::FWxStateTreeTask_MarkIndicator()
{
	// 재선택마다 재진입하면 ExitState 가 인디케이터를 해제해 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;

	// 완료 없이 매 프레임 도는 태스크라 바인딩 복사가 그대로 프레임 비용이 된다. 대상과 높이는 진입 시 한 번 받아 두면 되는 저작값이다.
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicator::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.Target.IsEmpty())
	{
		UE_LOG(LogWxUI, Warning, TEXT("Mark Indicator: 가리킬 대상이 지정되지 않음."));
	}

	// 첫 해석·등록이 실패(대상 언로드·매니저 미부착)해도 Tick 이 재시도한다.
	Instance.RegisteredIndicator.Reset();
	RefreshIndicator(Context, Instance);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	RefreshIndicator(Context, Instance);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_MarkIndicator::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	UnregisterIndicator(Instance.RegisteredIndicator);
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
