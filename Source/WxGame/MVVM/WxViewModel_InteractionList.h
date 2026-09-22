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
 * 스캐너 컴포넌트(WxWorld)를 직접 들고 목록·선택 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 선택의 소유자는 어디까지나 스캐너이며, 본 VM 은 받은 값을 표시한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_InteractionList : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** nullptr 이면 빈 목록으로 남는다. */
	void Initialize(UWxInteractionScannerComponent* InScanner);

	virtual void Deinitialize() override;

	/** 선택만 바뀌어도 행 전체를 다시 만든다. */
	UFUNCTION()
	void HandleRowsChanged();

	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestInteract();

	UFUNCTION(BlueprintCallable, Category = "Wx|Interaction")
	void RequestCycle(int32 Delta);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Interaction")
	TArray<TObjectPtr<UWxViewModel_Interaction>> Entries;

private:
	TWeakObjectPtr<UWxInteractionScannerComponent> CachedScanner;
};

/**
 * 스캐너는 AWxPlayerController 생성자 컴포넌트라 위젯보다 먼저 있다. 나중에 주입하는 구조로 바꾸면 늦은 도착 처리가 다시 필요하다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
