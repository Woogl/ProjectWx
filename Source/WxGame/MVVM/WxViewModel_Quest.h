// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Quest.generated.h"

class UWxQuestComponent;
class UWxViewModel_QuestObjective;
class UUserWidget;
class UMVVMView;

/**
 * 퀘스트 추적 HUD 뷰모델.
 *
 * GameState 의 퀘스트 컴포넌트(WxQuest)를 직접 들고 저널 변경을 구독한다. 그래서 WxUI 가 아니라 양쪽에 의존할 수 있는 본 모듈에 있다.
 * 저널의 소유자는 어디까지나 퀘스트 컴포넌트이며, 본 VM 은 변경 통지를 받아 현재 값을 pull 해 표시한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_Quest : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UWxQuestComponent* InQuestComponent);

	virtual void Deinitialize() override;

	UFUNCTION()
	void HandleJournalChanged();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	bool bHasActiveQuest = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	FText QuestTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Quest")
	TArray<TObjectPtr<UWxViewModel_QuestObjective>> Objectives;

private:
	void RebuildObjectives(const TArray<FText>& InObjectiveTexts);

	TWeakObjectPtr<UWxQuestComponent> CachedQuestComponent;
};

/**
 * 퀘스트 컴포넌트는 Experience 에셋 주입으로 부착되므로, 미등록 게임모드에선 null 을 반환한다(WBP 에서 뷰모델을 optional 로 둔다).
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Quest : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
