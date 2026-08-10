// Copyright Woogle. All Rights Reserved.

#include "WxDialogueStateTreeNodes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxDialogueComponent.h"
#include "WxDialogueModule.h"
#include "WxDialogueSessionComponent.h"

namespace
{
	UWxDialogueSessionComponent* FindDialogueSession(const AActor* Owner)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Owner, 0);
		return PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	}

	/** 이 컴포넌트가 상호작용 계약을 들므로 호스트 액터 타입은 보지 않는다. */
	UWxDialogueComponent* FindTargetDialogue(const FUniversalObjectLocator& Locator, AActor* Owner)
	{
		const AActor* Target = Cast<AActor>(Locator.SyncFind(Owner));
		return Target ? Target->FindComponentByClass<UWxDialogueComponent>() : nullptr;
	}

	/** 재로드로 액터가 새로 만들어지면 콜리전이 레벨 값으로 돌아가 있으므로 다시 걸고, 같은 액터면 이미 적용돼 있어 건드리지 않는다. */
	void RefreshNpcInteraction(const FStateTreeExecutionContext& Context, FWxStateTreeTask_EnableNpcInteractionInstanceData& Instance)
	{
		AActor* Owner = Cast<AActor>(Context.GetOwner());

		Instance.AppliedTargets.SetNum(Instance.Targets.Num());

		for (int32 Index = 0; Index < Instance.Targets.Num(); ++Index)
		{
			UWxDialogueComponent* Dialogue = FindTargetDialogue(Instance.Targets[Index], Owner);
			if (!Dialogue)
			{
				// 미지정·대화 상대 아님(잘못된 조립)과 스트리밍 아웃(정상)이 여기로 함께 들어온다. 기록을 비워 다시 로드되면 그때 적용한다.
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
	FText GetRowText(const FDataTableRowHandle& Row)
	{
		return Row.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(Row.RowName);
	}

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

// ── WaitDialogueCompleted ─────────────────────────────────────────────────────

FWxStateTreeTask_WaitDialogueCompleted::FWxStateTreeTask_WaitDialogueCompleted()
{
	// 완주 대기 중 같은 상태가 재선택되어도 목격 기록을 끊을 이유가 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_WaitDialogueCompleted::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 행 미지정은 어떤 대사와도 같지 않아 완료될 수 없는 잘못된 조립이다. 침묵 대기 대신 경고를 남긴다.
	if (!Instance.DialogueRow.DataTable || Instance.DialogueRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Wait Dialogue Completed: 대기할 대화가 지정되지 않음(DialogueRow)."));
	}

	// 이전 실행의 잔존 기록을 비운다. 진입 이전의 대화는 세지 않는다.
	Instance.bObservedDialogue = false;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_WaitDialogueCompleted::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 세션 부재(컨트롤러 미생성 등) 동안은 판정 없이 대기한다.
	const UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));
	if (!Session)
	{
		return EStateTreeRunStatus::Running;
	}

	// 대화 중이 아닐 때의 빈 핸들이 미지정 인자와 같아 보이므로 활성 대화만 맞춰 본다.
	if (Session->HasActiveDialogue())
	{
		if (Session->GetCurrentRowHandle() == Instance.DialogueRow)
		{
			Instance.bObservedDialogue = true;
		}
	}
	// 지정 행 다음 대사로 넘어간 것만으로는 아직 완주가 아니다 — 대화창이 닫힌 뒤 단계가 넘어가야 연출이 끊기지 않는다.
	else if (Instance.bObservedDialogue)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_WaitDialogueCompleted::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Wait Dialogue Completed ({0})"), GetRowText(InstanceData->DialogueRow));
}
#endif

// ── PlayDialogue ──────────────────────────────────────────────────────────────

FWxStateTreeTask_PlayDialogue::FWxStateTreeTask_PlayDialogue()
{
	// 진행 중인 대사를 같은 상태의 재선택으로 처음부터 다시 열지 않는다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 아래 셋은 모두 완주할 수 없는 잘못된 조립이다 — 대사 없이 상태에 눌러앉는 대신 실패를 낸다.
	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 시작 행이 지정되지 않음(StartRow)."));
		return EStateTreeRunStatus::Failed;
	}

	UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));
	if (!Session)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 0번 컨트롤러에서 대화 세션을 찾지 못함."));
		return EStateTreeRunStatus::Failed;
	}

	// 대상 없는 대사다 — 카메라는 플레이어에 머문다. 관찰자(Wait Dialogue Completed)는 대상이 아니라 행으로 판정하므로 이 대화도 게이트로 쓸 수 있다.
	Session->StartDialogueRow(Instance.StartRow, nullptr);

	// 소유 클라와 권위가 같은 머신이라 세션은 위 호출 안에서 열린다. 열리지 않았다면 행이 없거나 대사가 빈 것이다.
	if (!Session->HasActiveDialogue())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 대화를 열지 못함(행 없음·대사 빔): %s"), *Instance.StartRow.RowName.ToString());
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 세션이 사라졌다면(컨트롤러 교체 등) 더 기다릴 대화가 없으므로 종료와 같이 본다.
	const UWxDialogueSessionComponent* Session = FindDialogueSession(Cast<AActor>(Context.GetOwner()));

	return Session && Session->HasActiveDialogue() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("Play Dialogue ({0})"), GetRowText(InstanceData->StartRow));
}
#endif

// ── EnableNpcInteraction ──────────────────────────────────────────────────────

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
	if (Instance.Targets.IsEmpty())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Enable Npc Interaction: 토글할 대상이 지정되지 않음."));
	}
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		if (Locator.IsEmpty())
		{
			UE_LOG(LogWxDialogue, Warning, TEXT("Enable Npc Interaction: 빈 로케이터 항목이 있음(지정 %d개)."), Instance.Targets.Num());
			break;
		}
	}
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	for (const FUniversalObjectLocator& Locator : Instance.Targets)
	{
		const UObject* Object = Locator.SyncFind(Owner);
		if (Object && !FindTargetDialogue(Locator, Owner))
		{
			UE_LOG(LogWxDialogue, Warning, TEXT("Enable Npc Interaction: 대상 %s 에 대화 컴포넌트가 없어 말을 걸 수 있는 대상이 아님."), *GetNameSafe(Object));
		}
	}

	// 이전 실행의 잔존 기록을 비우고 첫 적용을 시도한다. 대상이 아직 언로드면 Tick 이 재시도한다.
	Instance.AppliedTargets.Reset();
	RefreshNpcInteraction(Context, Instance);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_EnableNpcInteraction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	RefreshNpcInteraction(Context, Instance);

	return EStateTreeRunStatus::Running;
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
#endif
