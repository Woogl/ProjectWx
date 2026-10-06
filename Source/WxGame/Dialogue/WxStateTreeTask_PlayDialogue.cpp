// Copyright Woogle. All Rights Reserved.

#include "Dialogue/WxStateTreeTask_PlayDialogue.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/WxCombatAttributeSet.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "WxGame.h"
#include "WxGameplayTags.h"
#include "Dialogue/WxDialogueSessionComponent.h"

FWxStateTreeTask_PlayDialogue::FWxStateTreeTask_PlayDialogue()
{
	bShouldCallTick = true;

	// 진행 중인 대사를 같은 상태의 재선택으로 처음부터 다시 열지 않는다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return StartDialogue(Context);
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::StartDialogue(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.bNeedsRestart = false;

	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 시작 행이 지정되지 않음(StartRow)."));
		return EStateTreeRunStatus::Failed;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	UWxDialogueSessionComponent* Session = PlayerController ? PlayerController->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 0번 컨트롤러에서 대화 세션을 찾지 못함."));
		return EStateTreeRunStatus::Failed;
	}

	Session->StartDialogueRow(Instance.StartRow, nullptr);

	// 소유 클라와 권위가 같은 머신이라 세션은 위 호출 안에서 열린다.
	if (!Session->HasActiveDialogue())
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("Play Dialogue: 대화를 열지 못함(사유는 직전 경고): %s"), *Instance.StartRow.RowName.ToString());
		return EStateTreeRunStatus::Failed;
	}

	// 약한 실행 컨텍스트를 넘기는 것이 엔진이 제시하는 방식이라 여기선 람다를 쓴다.
	Instance.Session = Session;
	Instance.EndedHandle = Session->OnDialogueEnded.AddLambda([WeakContext = Context.MakeWeakExecutionContext()](bool bCompleted)
	{
		if (bCompleted)
		{
			WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
		}
		else
		{
			TStateTreeStrongExecutionContext<true> StrongContext(WeakContext);
			if (FInstanceDataType* Data = StrongContext.GetInstanceDataPtr<FInstanceDataType>())
			{
				Data->bNeedsRestart = true;
			}
		}
	});

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayDialogue::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (!Instance.bNeedsRestart)
	{
		return EStateTreeRunStatus::Running;
	}
	const APlayerController* Controller = UGameplayStatics::GetPlayerController(Cast<AActor>(Context.GetOwner()), 0);
	const UAbilitySystemComponent* ASC = Controller ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Controller->GetPawn()) : nullptr;
	const UWxDialogueSessionComponent* Session = Controller ? Controller->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!ASC || !Session || Session->HasActiveDialogue() || ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)
		|| ASC->GetNumericAttribute(UWxCombatAttributeSet::GetHPAttribute()) <= 0.f)
	{
		return EStateTreeRunStatus::Running;
	}
	return StartDialogue(Context);
}

void FWxStateTreeTask_PlayDialogue::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (UWxDialogueSessionComponent* Session = Instance.Session.Get())
	{
		Session->OnDialogueEnded.Remove(Instance.EndedHandle);
	}
	Instance.EndedHandle.Reset();
	Instance.Session.Reset();
	Instance.bNeedsRestart = false;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayDialogue::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText RowText = InstanceData->StartRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->StartRow.RowName);
	return FText::Format(INVTEXT("Play Dialogue ({0})"), RowText);
}
#endif
