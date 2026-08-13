// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxStateTreeTask_MarkIndicator.h"

#include "Components/SceneComponent.h"
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

	/** 대상이 해석되면 등록하고, 해석되지 않으면(스트리밍 아웃·파괴) 해제한다. 등록증과 대상이 살아 있으면 해석 없이 그대로 둔다. */
	void RefreshIndicator(const FStateTreeExecutionContext& Context, FWxStateTreeTask_MarkIndicatorInstanceData& Instance)
	{
		// 매니저는 대상이 파괴돼도 표시만 접고 등록증은 남기므로 대상 컴포넌트까지 본다.
		const UWxIndicatorDescriptor* Indicator = Instance.RegisteredIndicator.Get();
		if (Indicator && IsValid(Indicator->GetTargetComponent()))
		{
			return;
		}

		// 대상을 잃은 등록증은 먼저 걷어낸다 — 다시 해석되면 새 대상으로 발급받는다.
		UnregisterIndicator(Instance.RegisteredIndicator);

		AActor* Owner = Cast<AActor>(Context.GetOwner());
		AActor* Target = ResolveTargetActor(Instance.Target, Owner);
		if (!Target)
		{
			return;
		}

		// 매니저는 아직 없을 수 있다(폰·컨트롤러 스폰 전).
		// 그때는 등록만 미루고 다음 틱이 재시도한다.
		UWxIndicatorManagerComponent* Manager = FindIndicatorManager(Owner);
		if (!Manager)
		{
			return;
		}

		Instance.RegisteredIndicator = Manager->AddIndicator(Target->GetRootComponent(), FVector(0.f, 0.f, Instance.WorldZOffset));
	}

#if WITH_EDITOR
	/** 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator)
	{
		if (Locator.IsEmpty())
		{
			return TEXT("unset");
		}

		if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
		{
			return Actor->GetActorLabel();
		}

		const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
		const FActorLocatorFragment* Payload = nullptr;
		if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
		{
			const FString SubPath = Payload->Path.GetSubPathString();
			int32 DotIndex = INDEX_NONE;
			return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
		}

		return TEXT("unresolved");
	}
#endif
}

FWxStateTreeTask_MarkIndicator::FWxStateTreeTask_MarkIndicator()
{
	// 재선택마다 재진입하면 ExitState 가 인디케이터를 해제해 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;
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
FText FWxStateTreeTask_MarkIndicator::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("인디케이터 표시 ({0})"), FText::FromString(GetTargetDisplayName(InstanceData->Target)));
}
#endif
