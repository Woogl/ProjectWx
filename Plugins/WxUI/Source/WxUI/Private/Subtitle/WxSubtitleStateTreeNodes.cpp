// Copyright Woogle. All Rights Reserved.

#include "Subtitle/WxSubtitleStateTreeNodes.h"

#include "MVVM/WxViewModel_Subtitle.h"
#include "StateTreeExecutionContext.h"
#include "WxUIModule.h"

// ── PrintSubtitle ─────────────────────────────────────────────────────────────

FWxStateTreeTask_PrintSubtitle::FWxStateTreeTask_PrintSubtitle()
{
	// 재선택마다 재진입하면 ExitState 가 자막을 걷어가 표시가 깜빡이고 경과 시간도 초기화된다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PrintSubtitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 빈 문구는 띄울 것이 없는 잘못된 조립이다. 침묵 대신 경고를 남긴다.
	if (Instance.SubtitleText.IsEmpty())
	{
		UE_LOG(LogWxUI, Warning, TEXT("Print Subtitle: 자막 문구가 비어 있음."));
	}

	Instance.ElapsedSeconds = 0.f;
	Instance.SubtitleHandle = INDEX_NONE;

	if (UWxViewModel_Subtitle* SubtitleViewModel = UWxViewModel_Subtitle::GetOrCreate(Context.GetOwner()))
	{
		Instance.SubtitleHandle = SubtitleViewModel->ShowSubtitle(Instance.SubtitleText);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PrintSubtitle::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 시간으로 끝내지 않는 설정이면 상태를 떠날 때까지 머문다(완료는 같은 상태의 다른 태스크가 낸다).
	if (Instance.Duration <= 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	Instance.ElapsedSeconds += DeltaTime;
	if (Instance.ElapsedSeconds < Instance.Duration)
	{
		return EStateTreeRunStatus::Running;
	}

	// 시간이 다 차면 상태를 떠나기 전에 여기서 걷는다.
	// 완료를 내도 상태는 같은 상태의 다른 대기 태스크에 붙잡힐 수 있어(TasksCompletion=All), ExitState 만 믿으면 자막이 화면에 남는다.
	if (UWxViewModel_Subtitle* SubtitleViewModel = UWxViewModel_Subtitle::GetOrCreate(Context.GetOwner()))
	{
		SubtitleViewModel->HideSubtitle(Instance.SubtitleHandle);
	}
	Instance.SubtitleHandle = INDEX_NONE;

	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_PrintSubtitle::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// Duration 을 채우기 전에 상태를 떠나는 경로. 이미 걷었으면 기록이 비어 있어 회수가 무시된다.
	if (UWxViewModel_Subtitle* SubtitleViewModel = UWxViewModel_Subtitle::GetOrCreate(Context.GetOwner()))
	{
		SubtitleViewModel->HideSubtitle(Instance.SubtitleHandle);
	}
	Instance.SubtitleHandle = INDEX_NONE;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PrintSubtitle::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText SubtitleText = InstanceData->SubtitleText.IsEmpty() ? INVTEXT("none") : InstanceData->SubtitleText;
	return FText::Format(INVTEXT("Print Subtitle (\"{0}\")"), SubtitleText);
}
#endif
