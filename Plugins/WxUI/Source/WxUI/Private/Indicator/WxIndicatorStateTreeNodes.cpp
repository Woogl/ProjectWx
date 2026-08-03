// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxUIModule.h"

namespace
{
	/** 로케이터를 ST 오너 컨텍스트로 대상 액터로 해석한다. 빈 로케이터·미로드(스트리밍 아웃)·파괴 대기는 nullptr. */
	AActor* ResolveTargetActor(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		AActor* Target = Cast<AActor>(Locator.SyncFind(Owner));
		return IsValid(Target) ? Target : nullptr;
	}

	/** 로컬 플레이어(0번 컨트롤러)의 인디케이터 매니저. 표시는 보는 사람의 사건이라 권위 러너가 아니라 로컬 컨트롤러에서 찾는다. */
	UWxIndicatorManagerComponent* FindIndicatorManager(const AActor* Owner)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
		return PlayerController ? PlayerController->FindComponentByClass<UWxIndicatorManagerComponent>() : nullptr;
	}

	/** 자기가 등록한 기록만 근거로 해제한다. */
	void UnregisterIndicator(FWxStateTreeTask_MarkIndicatorInstanceData& Instance)
	{
		if (UWxIndicatorDescriptor* Indicator = Instance.RegisteredIndicator.Get())
		{
			Indicator->Unregister();
		}
		Instance.RegisteredIndicator.Reset();
	}

	/** 대상이 해석되면 등록하고, 해석되지 않으면(스트리밍 아웃·파괴) 해제한다. 이미 등록돼 있으면 그대로 둔다. */
	void RefreshIndicator(const FStateTreeExecutionContext& Context, FWxStateTreeTask_MarkIndicatorInstanceData& Instance)
	{
		AActor* Owner = Cast<AActor>(Context.GetOwner());
		AActor* Target = ResolveTargetActor(Instance.Target.Locator, Owner);
		if (!Target)
		{
			UnregisterIndicator(Instance);
			return;
		}

		if (Instance.RegisteredIndicator.IsValid())
		{
			return;
		}

		UWxIndicatorManagerComponent* Manager = FindIndicatorManager(Owner);
		if (!Manager)
		{
			return;
		}

		Instance.RegisteredIndicator = Manager->AddIndicator(Target->GetRootComponent(), FVector(0.f, 0.f, Instance.WorldZOffset));
	}

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FText GetTargetText(const FUniversalObjectLocator& Locator)
	{
		if (Locator.IsEmpty())
		{
			return INVTEXT("unset");
		}

		if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
		{
			return FText::FromString(Actor->GetActorLabel());
		}

		// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
		const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
		const FActorLocatorFragment* Payload = nullptr;
		if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
		{
			const FString SubPath = Payload->Path.GetSubPathString();
			int32 DotIndex = INDEX_NONE;
			return FText::FromString(SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath);
		}

		return INVTEXT("unresolved");
	}
#endif
}

// ── MarkIndicator ─────────────────────────────────────────────────────────────

FWxStateTreeTask_MarkIndicator::FWxStateTreeTask_MarkIndicator()
{
	// 완료 없이 머무는 태스크다. 재선택마다 재진입하면 ExitState 가 인디케이터를 해제해 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicator::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 대상 미지정은 인디케이터를 띄울 수 없는 잘못된 조립이다. 침묵 대신 경고를 남긴다.
	if (Instance.Target.Locator.IsEmpty())
	{
		UE_LOG(LogWxUI, Warning, TEXT("Mark Indicator: 대상이 지정되지 않음(Target 빈 로케이터)."));
	}

	// 이전 실행의 잔존 기록을 비우고 첫 해석·등록을 시도한다. 실패(대상 언로드·매니저 미부착)면 Tick 이 재시도한다.
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
	UnregisterIndicator(Instance);
}

#if WITH_EDITOR
FText FWxStateTreeTask_MarkIndicator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Mark Indicator ({0})"), GetTargetText(InstanceData->Target.Locator));
}
#endif
