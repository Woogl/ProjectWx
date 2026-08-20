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

	/** 대상이 해석되면 그 컴포넌트에, 해석되지 않으면(스트리밍 아웃·파괴) 기록 좌표에 건다. 이미 원하는 곳에 걸려 있으면 그대로 둔다. */
	void RefreshIndicator(const FStateTreeExecutionContext& Context, FWxStateTreeTask_MarkIndicatorInstanceData& Instance)
	{
		// 빈 로케이터는 좌표도 기록되지 않았으므로 대역으로 갈 곳이 없다.
		if (Instance.Target.IsEmpty())
		{
			return;
		}

		AActor* Owner = Cast<AActor>(Context.GetOwner());
		AActor* Target = ResolveTargetActor(Instance.Target, Owner);
		USceneComponent* TargetComponent = Target ? Target->GetRootComponent() : nullptr;

		// 매니저는 대상이 파괴돼도 표시만 접고 등록증은 남기므로, 등록증이 지금 무엇에 걸려 있는지까지 본다.
		// 둘 다 비었으면 이미 좌표 대역으로 걸린 것이라 그대로 둔다 — 매 틱 갈아 끼우면 등록증만 버려진다.
		if (const UWxIndicatorDescriptor* Indicator = Instance.RegisteredIndicator.Get())
		{
			if (Indicator->GetTargetComponent() == TargetComponent)
			{
				return;
			}
		}

		// 걸 곳이 바뀐 등록증은 먼저 걷어낸다.
		UnregisterIndicator(Instance.RegisteredIndicator);

		// 매니저는 아직 없을 수 있다(폰·컨트롤러 스폰 전).
		// 그때는 등록만 미루고 다음 틱이 재시도한다.
		UWxIndicatorManagerComponent* Manager = FindIndicatorManager(Owner);
		if (!Manager)
		{
			return;
		}

		const FVector WorldOffset(0.f, 0.f, Instance.WorldZOffset);
		if (TargetComponent)
		{
			Instance.RegisteredIndicator = Manager->AddIndicator(TargetComponent, WorldOffset);
		}
		else
		{
			Instance.RegisteredIndicator = Manager->AddIndicator(Instance.TargetLocation, WorldOffset);
		}
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

	return FText::Format(INVTEXT("인디케이터 표시 ({0})"), FText::FromString(GetTargetDisplayName(InstanceData->Target)));
}
#endif
