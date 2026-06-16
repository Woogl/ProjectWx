// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_InteractionList.generated.h"

class UWxViewModel_Interaction;

/**
 * 상호작용 HUD 리스트 뷰모델.
 * 현재 범위 안에 있는 상호작용 대상들을 항목 VM 목록으로 노출하고, 그중 선택된 인덱스를 표시한다.
 *
 * WxUI 는 WxWorld 레지스트리를 참조할 수 없으므로, WxGame 리졸버가 레지스트리의 목록/선택 변경 델리게이트를
 * 본 VM 의 HandleListChanged/HandleSelectionChanged(엔진 타입 인자)에 연결해 흘려준다.
 * 선택은 레지스트리가 소유하며, 본 VM 은 받은 값을 표시만 한다(휠/방향키 입력은 WBP 가 레지스트리에 직접 전달).
 */
UCLASS()
class WXUI_API UWxViewModel_InteractionList : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 초기 목록/선택으로 항목을 구성하고 초기화 완료로 표시한다. */
	void Initialize(const TArray<FText>& InPrompts, int32 InSelectedIndex);

	virtual void Deinitialize() override;

	/** 인-레인지 목록 변경 수신(리졸버가 레지스트리 OnListChanged 에 연결). */
	UFUNCTION()
	void HandleListChanged(const TArray<FText>& InPrompts);

	/** 선택 변경 수신(리졸버가 레지스트리 OnSelectionChanged 에 연결). 표시만 갱신한다. */
	UFUNCTION()
	void HandleSelectionChanged(int32 InSelectedIndex);

	/** 상호작용 대상 항목 목록. ListView 가 본 프로퍼티에 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;

	/** 현재 선택된 항목 인덱스(없으면 INDEX_NONE). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	int32 SelectedIndex = INDEX_NONE;

private:
	/** 프롬프트 목록으로 항목 VM 들을 재구성한다. */
	void RebuildEntries(const TArray<FText>& InPrompts);

	/** 선택 인덱스를 클램프해 각 항목의 bSelected 와 SelectedIndex 를 갱신한다. */
	void ApplySelection(int32 InSelectedIndex);
};
