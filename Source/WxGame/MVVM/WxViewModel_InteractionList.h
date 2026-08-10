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
 * 상호작용 HUD 리스트 뷰모델.
 * 현재 범위 안에 있는 상호작용 대상들을 항목 VM 목록으로 노출하고, 그중 선택된 인덱스를 표시한다.
 *
 * 스캐너 컴포넌트(WxWorld)를 직접 들고 목록·선택 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 선택의 소유자는 어디까지나 스캐너이며, 본 VM 은 받은 값을 표시한다.
 *
 * 스캐너는 주입(서버) 또는 복제(클라)로 붙어 위젯보다 늦게 도착할 수 있고, 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없다.
 * 그래서 인스턴스는 고정한 채 도착 신호를 받아 내부 상태(Initialize)만 갈아끼운다 — UWxViewModel_Inventory 와 같은 구조다.
 *
 * 표시에 더해 뷰(WBP)의 입력을 스캐너로 넘긴다. WBP 가 Enhanced Input 으로 받은 실행·선택이동을 Request 함수로 호출하면 스캐너 진입점을 그대로 부른다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 대상 PC 의 스캐너 관찰을 시작한다. 이미 붙어 있으면 즉시 연결하고, 아니면 도착 신호를 기다린다. */
	void StartObserving(APlayerController* PC);

	/** 스캐너를 물려 목록·선택 변경을 구독하고 현재 상태로 시드한다. */
	void Initialize(UWxInteractionScannerComponent* InScanner);

	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

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

	/** ListView 가 본 프로퍼티에 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;

	/** 현재 선택된 항목 인덱스(없으면 INDEX_NONE). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	int32 SelectedIndex = INDEX_NONE;

private:
	/** 스캐너 도착 수신. 관찰 중인 PC 의 것이면 연결하고 관찰을 끝낸다. */
	void HandleScannerReady(UWxInteractionScannerComponent* Scanner);

	/** 도착 신호 구독을 해제한다. 연결 성공 시와 소멸 시 모두 여기로 모은다. */
	void StopObserving();

	void RebuildEntries(const TArray<FText>& InPrompts);

	/** 선택 인덱스를 클램프해 각 항목의 bSelected 와 SelectedIndex 를 갱신한다. */
	void ApplySelection(int32 InSelectedIndex);

	TWeakObjectPtr<APlayerController> ObservedController;

	FDelegateHandle ScannerReadyHandle;

	TWeakObjectPtr<UWxInteractionScannerComponent> CachedScanner;
};

/**
 * VM_InteractionList 용 View Bindings Resolver.
 *
 * 위젯을 소유한 PlayerController 로 위젯별 UWxViewModel_InteractionList 를 생성하고 관찰을 시작시킨다.
 * 스캐너가 아직 없어도(클라 복제 도착 전, 미등록 모드) VM 은 만들어지며, 연결은 VM 이 도착 신호 관찰로 스스로 처리한다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
