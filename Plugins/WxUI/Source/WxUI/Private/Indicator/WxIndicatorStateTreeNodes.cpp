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
	void UnregisterIndicator(TWeakObjectPtr<UWxIndicatorDescriptor>& RegisteredIndicator)
	{
		if (UWxIndicatorDescriptor* Indicator = RegisteredIndicator.Get())
		{
			Indicator->Unregister();
		}
		RegisteredIndicator.Reset();
	}

	/** 대상마다 해석되면 등록하고, 해석되지 않으면(스트리밍 아웃·파괴) 해제한다. 이미 등록돼 있으면 그대로 둔다. */
	void RefreshIndicators(const FStateTreeExecutionContext& Context, FWxStateTreeTask_MarkIndicatorsInstanceData& Instance)
	{
		AActor* Owner = Cast<AActor>(Context.GetOwner());

		// 기록은 지정과 같은 인덱스로 짝을 이룬다. 매니저는 아직 없을 수 있으며(폰·컨트롤러 스폰 전) 그때는 등록만 미루고 해제는 그대로 수행한다.
		Instance.RegisteredIndicators.SetNum(Instance.Targets.Num());
		UWxIndicatorManagerComponent* Manager = FindIndicatorManager(Owner);

		for (int32 Index = 0; Index < Instance.Targets.Num(); ++Index)
		{
			TWeakObjectPtr<UWxIndicatorDescriptor>& RegisteredIndicator = Instance.RegisteredIndicators[Index];
			AActor* Target = ResolveTargetActor(Instance.Targets[Index], Owner);
			if (!Target)
			{
				UnregisterIndicator(RegisteredIndicator);
				continue;
			}

			if (RegisteredIndicator.IsValid() || !Manager)
			{
				continue;
			}

			RegisteredIndicator = Manager->AddIndicator(Target->GetRootComponent(), FVector(0.f, 0.f, Instance.WorldZOffset));
		}
	}

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
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

		// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
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

	/** 지정 목록의 표시 텍스트. 라벨 3개까지 나열하고 초과분은 +N 로 줄인다. 빈 배열은 none. */
	FText GetTargetsText(const TArray<FUniversalObjectLocator>& Targets)
	{
		if (Targets.IsEmpty())
		{
			return INVTEXT("none");
		}

		constexpr int32 MaxNames = 3;
		TArray<FString> Names;
		for (int32 Index = 0; Index < Targets.Num() && Index < MaxNames; ++Index)
		{
			Names.Add(GetTargetDisplayName(Targets[Index]));
		}

		FString Joined = FString::Join(Names, TEXT(", "));
		if (Targets.Num() > MaxNames)
		{
			Joined += FString::Printf(TEXT(" +%d"), Targets.Num() - MaxNames);
		}
		return FText::FromString(Joined);
	}
#endif
}

// ── MarkIndicators ────────────────────────────────────────────────────────────

FWxStateTreeTask_MarkIndicators::FWxStateTreeTask_MarkIndicators()
{
	// 완료 없이 머무는 태스크다. 재선택마다 재진입하면 ExitState 가 인디케이터를 해제해 표시가 깜빡인다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	// 대기 태스크와 같은 상태에 얹히므로 판정에서 뺀다 — 완료를 내지 않는 태스크가 판정에 끼면 그 상태가 영영 완료되지 않는다.
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicators::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 대상 미지정·빈 로케이터는 인디케이터를 띄울 수 없는 잘못된 조립이다. 침묵 대신 경고를 남긴다.
	if (Instance.Targets.IsEmpty())
	{
		UE_LOG(LogWxUI, Warning, TEXT("Mark Indicators: 가리킬 대상이 지정되지 않음."));
	}
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxUI, Warning, TEXT("Mark Indicators: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Targets.Num());
			break;
		}
	}

	// 이전 실행의 잔존 기록을 비우고 첫 해석·등록을 시도한다. 실패(대상 언로드·매니저 미부착)면 Tick 이 재시도한다.
	Instance.RegisteredIndicators.Reset();
	RefreshIndicators(Context, Instance);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MarkIndicators::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	RefreshIndicators(Context, Instance);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_MarkIndicators::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	for (TWeakObjectPtr<UWxIndicatorDescriptor>& RegisteredIndicator : Instance.RegisteredIndicators)
	{
		UnregisterIndicator(RegisteredIndicator);
	}
	Instance.RegisteredIndicators.Reset();
}

#if WITH_EDITOR
FText FWxStateTreeTask_MarkIndicators::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Mark Indicators ({0})"), GetTargetsText(InstanceData->Targets));
}
#endif
