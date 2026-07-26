// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Quest.generated.h"

class UWxQuestComponent;
class UUserWidget;
class UMVVMView;

/**
 * 퀘스트 추적 HUD 뷰모델.
 * 현재 추적 중인 퀘스트의 저널(활성 여부·제목·목표)을 노출한다.
 *
 * GameState 의 퀘스트 컴포넌트(WxQuest)를 직접 들고 저널 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 저널의 소유자는 어디까지나 퀘스트 컴포넌트이며, 본 VM 은 변경 통지를 받아 현재 값을 pull 해 표시한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Quest : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 퀘스트 컴포넌트를 물려 저널 변경을 구독하고 현재 상태로 시드한다. */
	void Initialize(UWxQuestComponent* InQuestComponent);

	virtual void Deinitialize() override;

	/** 저널 변경 수신. 컴포넌트에서 현재 값을 pull 한다. */
	UFUNCTION()
	void HandleJournalChanged();

	/** 추적 중인 퀘스트가 있는지. 위젯 표시 여부로 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	bool bHasActiveQuest = false;

	/** 퀘스트 제목. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText QuestTitle;

	/** 현재 목표 문구. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText ObjectiveText;

private:
	TWeakObjectPtr<UWxQuestComponent> CachedQuestComponent;
};

/**
 * VM_Quest 용 View Bindings Resolver.
 *
 * 위젯이 속한 월드의 GameState 에서 퀘스트 컴포넌트를 끌어와 위젯별 UWxViewModel_Quest 를 생성/초기화한다.
 * 퀘스트 컴포넌트는 GameMode 에셋의 FrameworkComponents 주입으로 부착되므로, 미부착 게임모드에선 null 을 반환한다(WBP 에서 뷰모델을 optional 로 둔다).
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Quest : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
