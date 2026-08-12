// Copyright Woogle. All Rights Reserved.

#include "Subtitle/WxStateTreeTask_PrintSubtitle.h"

#include "MVVM/WxViewModel_Subtitle.h"
#include "StateTreeExecutionContext.h"
#include "Subtitle/WxSubtitleTableRow.h"
#include "WxUIModule.h"

// ── PrintSubtitle ─────────────────────────────────────────────────────────────

FWxStateTreeTask_PrintSubtitle::FWxStateTreeTask_PrintSubtitle()
{
	// 재선택마다 재진입하면 ExitState 가 자막을 걷어가 표시가 깜빡이고 진행도 첫 줄로 되돌아간다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_PrintSubtitle::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	Instance.CurrentRowName = NAME_None;
	Instance.CurrentDuration = 0.f;
	Instance.ElapsedSeconds = 0.f;
	Instance.SubtitleHandle = INDEX_NONE;

	// 미지정은 기능 미사용이다 — 스텝 템플릿의 옵션 파라미터 자리라 경고 없이 즉시 완료한다.
	if (!Instance.StartRow.DataTable || Instance.StartRow.RowName.IsNone())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (!ShowRow(Context, Instance, Instance.StartRow.RowName))
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PrintSubtitle::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.CurrentDuration <= 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	Instance.ElapsedSeconds += DeltaTime;
	if (Instance.ElapsedSeconds < Instance.CurrentDuration)
	{
		return EStateTreeRunStatus::Running;
	}

	// 다음 줄로 이어간다. 슬롯이 하나라 새 줄이 앞 줄을 덮으므로 넘어가는 자리에선 걷지 않는다.
	const UDataTable* Table = Instance.StartRow.DataTable;
	const FWxSubtitleTableRow* Row = Table ? Table->FindRow<FWxSubtitleTableRow>(Instance.CurrentRowName, TEXT("WxPrintSubtitle")) : nullptr;
	const FName NextRowName = Row ? Row->NextRow : NAME_None;
	if (!NextRowName.IsNone() && ShowRow(Context, Instance, NextRowName))
	{
		return EStateTreeRunStatus::Running;
	}

	// 마지막 줄이었거나 다음 줄을 해석하지 못했다(후자는 ShowRow 가 경고를 남긴다). 어느 쪽이든 자막은 여기서 끝난다.
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

	// 마지막 줄을 다 채우기 전에 상태를 떠나는 경로. 이미 걷었으면 기록이 비어 있어 회수가 무시된다.
	if (UWxViewModel_Subtitle* SubtitleViewModel = UWxViewModel_Subtitle::GetOrCreate(Context.GetOwner()))
	{
		SubtitleViewModel->HideSubtitle(Instance.SubtitleHandle);
	}
	Instance.SubtitleHandle = INDEX_NONE;
}

bool FWxStateTreeTask_PrintSubtitle::ShowRow(FStateTreeExecutionContext& Context, FInstanceDataType& Instance, FName RowName) const
{
	const UDataTable* Table = Instance.StartRow.DataTable;
	const FWxSubtitleTableRow* Row = Table ? Table->FindRow<FWxSubtitleTableRow>(RowName, TEXT("WxPrintSubtitle")) : nullptr;
	if (!Row)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Print Subtitle: 행을 찾지 못했다(테이블 %s / 행 %s). 가리키는 이름이 틀렸거나 행이 지워졌다."),
			*GetNameSafe(Table), *RowName.ToString());
		return false;
	}

	if (Row->Line.IsEmpty())
	{
		// FindRow 의 ContextString 경고는 행이 아예 없을 때만 뜬다 — 본문이 빈 행은 여기서만 드러난다.
		UE_LOG(LogWxUI, Warning, TEXT("Print Subtitle: 본문이 비어 있다(테이블 %s / 행 %s). 종료는 NextRow=None 으로 표시한다."),
			*GetNameSafe(Table), *RowName.ToString());
		return false;
	}

	UWxViewModel_Subtitle* SubtitleViewModel = UWxViewModel_Subtitle::GetOrCreate(Context.GetOwner());
	if (!SubtitleViewModel)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Print Subtitle: 자막 뷰모델을 얻지 못해 표시할 수 없다(행 %s)."), *RowName.ToString());
		return false;
	}

	Instance.CurrentRowName = RowName;
	Instance.CurrentDuration = Row->Duration;
	Instance.ElapsedSeconds = 0.f;
	Instance.SubtitleHandle = SubtitleViewModel->ShowSubtitle(Row->Speaker, Row->Line);

	return true;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PrintSubtitle::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	const FText RowText = InstanceData->StartRow.RowName.IsNone() ? INVTEXT("unset") : FText::FromName(InstanceData->StartRow.RowName);
	return FText::Format(INVTEXT("Print Subtitle ({0})"), RowText);
}
#endif
