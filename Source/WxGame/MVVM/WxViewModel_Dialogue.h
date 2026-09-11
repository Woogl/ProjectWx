// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Dialogue.generated.h"

class UWxDialogueSessionComponent;
class UUserWidget;
class UMVVMView;

UCLASS()
class WXGAME_API UWxViewModel_Dialogue : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 이전 세션을 해제한 뒤 연결한다. nullptr이면 표시까지 비운다. */
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

UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Dialogue : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
