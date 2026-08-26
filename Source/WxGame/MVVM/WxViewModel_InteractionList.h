// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_InteractionList.generated.h"

class APlayerController;
class UWxInteractionScannerComponent;
class UWxViewModel_Interaction;
class UUserWidget;
class UMVVMView;

/**
 * 현재 범위 안에 있는 상호작용 대상들을 항목 VM 목록으로 노출하고, 그중 선택된 인덱스를 표시한다.
 *
 * 스캐너 컴포넌트(WxWorld)를 직접 들고 목록·선택 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 선택의 소유자는 어디까지나 스캐너이며, 본 VM 은 받은 값을 표시한다.
 *
 * 표시에 더해 뷰의 실행·선택이동 요청을 스캐너 진입점으로 그대로 넘긴다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 스캐너가 이미 붙어 있으면 즉시 연결하고, 아니면 도착 신호를 기다린다. */
	void StartObserving(APlayerController* PC);

	/** 스캐너를 물려 목록·선택 변경을 구독하고 현재 상태로 시드한다. */
	void Initialize(UWxInteractionScannerComponent* InScanner);

	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	UFUNCTION()
	void HandleListChanged(const TArray<FText>& InPrompts);

	/** 표시만 갱신한다. */
	UFUNCTION()
	void HandleSelectionChanged(int32 InSelectedIndex);

	/** 선택 대상은 스캐너가 알고 있으므로 인자가 없다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestInteract();

	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestCycle(int32 Delta);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	int32 SelectedIndex = INDEX_NONE;

private:
	/** 관찰 중인 PC 의 것이면 연결하고 관찰을 끝낸다. */
	void HandleScannerReady(UWxInteractionScannerComponent* Scanner);

	/** 연결 성공 시와 소멸 시 모두 여기로 모은다. */
	void StopObserving();

	void RebuildEntries(const TArray<FText>& InPrompts);

	void ApplySelection(int32 InSelectedIndex);

	TWeakObjectPtr<APlayerController> ObservedController;

	FDelegateHandle ScannerReadyHandle;

	TWeakObjectPtr<UWxInteractionScannerComponent> CachedScanner;
};

/**
 * 위젯을 소유한 PlayerController 로 위젯별 UWxViewModel_InteractionList 를 생성하고 관찰을 시작시킨다.
 * 스캐너가 아직 없어도(클라 복제 도착 전, 미등록 모드) VM 은 만들어지며, 연결은 VM 이 도착 신호 관찰로 스스로 처리한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
