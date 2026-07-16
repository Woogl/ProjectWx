// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "WxInteractionRegistrySubsystem.generated.h"

class UWxInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionListChanged, const TArray<FText>&, Prompts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionSelectionChanged, int32, SelectedIndex);

/**
 * 상호작용 레지스트리 서브시스템.
 * 로컬 플레이어마다 자동 생성되어, 현재 범위 안에 있는 UWxInteractionComponent들을 모은다.
 * HUD 리스트 뷰모델(UWxViewModel_InteractionList)이 이 목록을 표시한다.
 *
 * 플레이어 측 스캐너(상호작용 어빌리티의 주기 스캔)가 매 스캔마다 UpdateInRange로 후보 집합을 push 한다.
 * 기존 항목 순서는 보존하고 신규만 뒤에 추가하므로(이탈은 제거), 거리 변동에 목록 순서가 흔들리지 않는다.
 * 선택(SelectedIndex)을 본 서브시스템이 소유하고, 선택된 컴포넌트만 외곽선 강조하도록 조율한다.
 * 입력(휠/방향키)은 WBP가 CycleSelection을 호출해 흘려준다.
 * VM은 목록/선택을 받아 표시만 한다. 로컬 표시 전용이다.
 */
UCLASS()
class WXWORLD_API UWxInteractionRegistrySubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 플레이어 측 스캐너가 매 스캔마다 호출한다. 후보 집합으로 인-레인지 멤버십을 갱신한다.
	 * 기존 항목 순서는 보존하고 신규만 뒤에 추가(후보는 거리순이라 가까운 것부터), 이탈은 제거한다.
	 * 멤버십이 실제로 바뀐 경우에만 강조/목록/선택을 갱신·발화한다(불변이면 침묵).
	 */
	void UpdateInRange(const TArray<UWxInteractionComponent*>& InCandidates);

	/** 현재 인-레인지 컴포넌트들의 프롬프트 텍스트를 순서대로 반환한다. */
	TArray<FText> GetPrompts() const;

	/** 현재 선택 인덱스(없으면 INDEX_NONE). 리졸버가 초기 시드로 읽는다. */
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/** 현재 선택된 인-레인지 컴포넌트(없으면 nullptr). 상호작용 어빌리티가 실행 대상으로 읽는다. */
	UWxInteractionComponent* GetSelectedComponent() const;

	/** 선택을 Delta 만큼 순환 이동한다(휠/방향키). 목록이 비면 무시. WBP 입력이 호출한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void CycleSelection(int32 Delta);

	/** 인-레인지 목록 변경 시 발사. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionListChanged OnListChanged;

	/** 선택 인덱스 변경 시 발사. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionSelectionChanged OnSelectionChanged;

private:
	/** 선택 인덱스를 갱신하고(변경 시) 강조 갱신 + 선택 변경을 알린다. */
	void UpdateSelection(int32 NewIndex);

	/** 선택된 컴포넌트만 외곽선 강조 ON, 나머지는 OFF. */
	void ApplyHighlight();

	TArray<TWeakObjectPtr<UWxInteractionComponent>> InRangeComponents;

	int32 SelectedIndex = INDEX_NONE;
};
