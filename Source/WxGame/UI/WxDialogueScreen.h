// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widget/WxActivatableWidget.h"
#include "WxDialogueScreen.generated.h"

class UWxDialogueSessionComponent;
class UWxViewModel_Dialogue;

/** 활성화된 동안 세션의 대사를 표시 VM에 공급하고 진행 입력을 전달한다. */
UCLASS(Abstract)
class WXGAME_API UWxDialogueScreen : public UWxActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wx|Dialogue")
	void RequestAdvance();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	void BindDialogue();
	void UnbindDialogue();

	TWeakObjectPtr<UWxDialogueSessionComponent> ObservedSession;
	TWeakObjectPtr<UWxViewModel_Dialogue> DialogueViewModel;
};
