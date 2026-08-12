// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_EnableNpcInteraction.h"

#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxDialogueComponent.h"
#include "WxDialogueModule.h"

FWxStateTreeTask_EnableNpcInteraction::FWxStateTreeTask_EnableNpcInteraction()
{
	// 잠금은 그 상태가 선언하는 가용성이라, 같은 상태가 재선택돼도 껐다 켤 이유가 없다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	// 완료를 내지 않는 태스크가 판정에 끼면 대기 태스크와 같은 상태가 영영 완료되지 않는다.
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_EnableNpcInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 잘못된 조립만 여기서 한 번 가른다. 미해석(스트리밍 아웃)은 정상 상황이라 경고하지 않는다 — 매 틱 재시도하는 판이라 로그가 폭주하기도 한다.
	// 빈 배열은 기능 미사용(재사용 스텝의 옵션 파라미터)이라 경고하지 않고, 항목이 있는데 빈 로케이터인 것만 오조립으로 본다.
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxDialogue, Warning, TEXT("Enable Npc Interaction: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Targets.Num());
			break;
		}
	}

	// 이전 실행의 잔존 기록을 비우고 첫 적용을 시도한다. 대상이 아직 언로드면 Tick 이 재시도한다.
	// 대상 해석은 여기서 한 번만 하고, 대화 상대가 아닌 대상 경고도 그 해석 결과로 함께 낸다.
	Instance.AppliedTargets.Reset();
	RefreshNpcInteraction(Context, Instance, /*bWarnNotDialogueTarget=*/true);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_EnableNpcInteraction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	RefreshNpcInteraction(Context, Instance, /*bWarnNotDialogueTarget=*/false);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_EnableNpcInteraction::RefreshNpcInteraction(const FStateTreeExecutionContext& Context, FInstanceDataType& Instance, bool bWarnNotDialogueTarget) const
{
	AActor* Owner = Cast<AActor>(Context.GetOwner());

	Instance.AppliedTargets.SetNum(Instance.Targets.Num());

	for (int32 Index = 0; Index < Instance.Targets.Num(); ++Index)
	{
		// 이 컴포넌트가 상호작용 계약을 들므로 호스트 액터 타입은 보지 않는다.
		UObject* Object = Instance.Targets[Index].SyncFind(Owner);
		const AActor* Target = Cast<AActor>(Object);
		UWxDialogueComponent* Dialogue = Target ? Target->FindComponentByClass<UWxDialogueComponent>() : nullptr;
		if (!Dialogue)
		{
			// 미지정·대화 상대 아님(잘못된 조립)과 스트리밍 아웃(정상)이 여기로 함께 들어온다. 해석은 됐는데 컴포넌트가 없을 때만 잘못된 조립이다.
			if (bWarnNotDialogueTarget && Object)
			{
				UE_LOG(LogWxDialogue, Warning, TEXT("Enable Npc Interaction: 대상 %s 에 대화 컴포넌트가 없어 말을 걸 수 있는 대상이 아님."), *GetNameSafe(Object));
			}

			// 기록을 비워 다시 로드되면 그때 적용한다.
			Instance.AppliedTargets[Index].Reset();
			continue;
		}

		if (Instance.AppliedTargets[Index].Get() == Dialogue)
		{
			continue;
		}

		Dialogue->SetInteractionEnabled(Instance.bEnable);
		Instance.AppliedTargets[Index] = Dialogue;
	}
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableNpcInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 켜기·끄기가 한눈에 갈리도록 표시명 자체를 바꾼다(노드 목록에서 값을 펼치지 않고도 읽힌다).
	const FText Action = InstanceData->bEnable ? INVTEXT("Enable") : INVTEXT("Disable");

	return FText::Format(INVTEXT("{0} Npc Interaction ({1})"), Action, GetTargetsText(InstanceData->Targets));
}

FString FWxStateTreeTask_EnableNpcInteraction::GetTargetDisplayName(const FUniversalObjectLocator& Locator) const
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

FText FWxStateTreeTask_EnableNpcInteraction::GetTargetsText(const TArray<FUniversalObjectLocator>& Targets) const
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
