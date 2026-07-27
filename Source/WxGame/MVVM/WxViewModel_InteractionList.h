// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_InteractionList.generated.h"

class UWxInteractionScannerComponent;
class UWxViewModel_Interaction;
class UUserWidget;
class UMVVMView;

/**
 * 상호작용 HUD 리스트 뷰모델.
 * 현재 범위 안에 있는 상호작용 대상들을 항목 VM 목록으로 노출하고, 그중 선택된 인덱스를 표시한다.
 *
 * 스캐너 컴포넌트(WxWorld)를 직접 들고 목록·선택 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 선택의 소유자는 어디까지나 스캐너이며, 본 VM 은 받은 값을 표시한다.
 *
 * 표시에 더해 뷰(WBP)의 입력을 스캐너로 넘긴다. WBP 가 Enhanced Input 으로 받은 실행·선택이동을 Request 함수로 호출하면 스캐너 진입점을 그대로 부른다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 스캐너를 물려 목록·선택 변경을 구독하고 현재 상태로 시드한다. */
	void Initialize(UWxInteractionScannerComponent* InScanner);

	virtual void Deinitialize() override;

	/** 인-레인지 목록 변경 수신. */
	UFUNCTION()
	void HandleListChanged(const TArray<FText>& InPrompts);

	/** 선택 변경 수신. 표시만 갱신한다. */
	UFUNCTION()
	void HandleSelectionChanged(int32 InSelectedIndex);

	/** 뷰(WBP)의 상호작용 실행 요청. 선택 대상은 스캐너가 알고 있으므로 인자가 없다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestInteract();

	/** 뷰(WBP)의 선택 이동 요청(휠 등). Delta 만큼 순환 이동을 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestCycle(int32 Delta);

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

	TWeakObjectPtr<UWxInteractionScannerComponent> CachedScanner;
};

/**
 * VM_InteractionList 용 View Bindings Resolver.
 *
 * 위젯을 소유한 PlayerController 에서 스캐너 컴포넌트를 조회해 위젯별 UWxViewModel_InteractionList 를 생성/초기화한다.
 * 스캐너는 GameMode 의 FrameworkComponents 주입으로 붙으므로, 등록되지 않은 모드에서는 조회가 비고 VM 도 만들어지지 않는다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
