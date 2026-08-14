// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Dialogue.generated.h"

class UWxDialogueSessionComponent;
class UUserWidget;
class UMVVMView;

/**
 * 대화 창 뷰모델.
 * 현재 대사(화자·본문)를 노출한다.
 *
 * 세션 컴포넌트(WxDialogue)를 직접 들고 대사 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 진행의 소유자는 어디까지나 세션이며, 본 VM 은 받은 값을 표시한다.
 *
 * 표시에 더해 뷰(WBP)의 넘기기 요청을 세션으로 넘긴다. 뷰는 그래프 없이 MVVM Event 바인딩으로 버튼 클릭을 RequestAdvance 에 잇는다.
 * 대화 창을 닫는 것은 창을 띄운 UWxUIManagerSubsystem 의 몫이라 본 VM 은 종료를 알지 않는다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Dialogue : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 세션을 물려 대사 변경을 구독하고 현재 대사로 시드한다. */
	void Initialize(UWxDialogueSessionComponent* InSession);

	virtual void Deinitialize() override;

	UFUNCTION()
	void HandleLineChanged(const FText& InSpeaker, const FText& InLine);

	UFUNCTION(BlueprintCallable, Category = "Wx|Dialogue")
	void RequestAdvance();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Wx|Dialogue")
	bool HasSpeaker() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Dialogue")
	FText LineText;

private:
	TWeakObjectPtr<UWxDialogueSessionComponent> CachedSession;
};

/**
 * VM_Dialogue 용 View Bindings Resolver.
 *
 * 대화 위젯은 세션이 열린 뒤에만 푸시되므로 생성 시점엔 항상 활성 세션이 있다 — 늦은 도착을 기다릴 필요가 없다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Dialogue : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
