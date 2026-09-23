// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WxQuestTracker.generated.h"

class UWxQuestComponent;
class UWxViewModel_Quest;

/** GameState의 저널을 표시 VM에 연결한다. */
UCLASS(Abstract)
class WXGAME_API UWxQuestTracker : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void UnbindJournal();

	UFUNCTION()
	void HandleJournalChanged();

	TWeakObjectPtr<UWxQuestComponent> ObservedQuest;
	TWeakObjectPtr<UWxViewModel_Quest> QuestViewModel;
};
