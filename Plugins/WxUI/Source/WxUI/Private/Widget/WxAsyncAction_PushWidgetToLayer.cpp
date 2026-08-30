// Copyright Woogle. All Rights Reserved.

#include "Widget/WxAsyncAction_PushWidgetToLayer.h"
#include "System/WxUIManagerSubsystem.h"
#include "System/WxPrimaryGameLayout.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UWxAsyncAction_PushWidgetToLayer* UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(UObject* InWorldContextObject, FGameplayTag InLayerTag, TSoftClassPtr<UCommonActivatableWidget> InWidgetClass)
{
	UWxAsyncAction_PushWidgetToLayer* Action = NewObject<UWxAsyncAction_PushWidgetToLayer>();
	Action->WorldContextObject = InWorldContextObject;
	Action->LayerTag = InLayerTag;
	Action->WidgetClass = InWidgetClass;
	Action->RegisterWithGameInstance(InWorldContextObject);
	return Action;
}

void UWxAsyncAction_PushWidgetToLayer::Activate()
{
	if (bFinished || !WorldContextObject || WidgetClass.IsNull())
	{
		Finish(nullptr);
		return;
	}

	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	UWxUIManagerSubsystem* UIManager = GameInstance ? GameInstance->GetSubsystem<UWxUIManagerSubsystem>() : nullptr;
	TargetLayout = UIManager ? UIManager->GetPrimaryGameLayout() : nullptr;
	if (!TargetLayout.IsValid())
	{
		Finish(nullptr);
		return;
	}

	if (WidgetClass.Get())
	{
		HandleWidgetClassLoaded();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	const TSharedPtr<FStreamableHandle> NewHandle = StreamableManager.RequestAsyncLoad(
		WidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ThisClass::HandleWidgetClassLoaded)
	);
	if (bFinished)
	{
		return;
	}

	StreamableHandle = NewHandle;
	if (!StreamableHandle.IsValid())
	{
		Finish(nullptr);
	}
}

void UWxAsyncAction_PushWidgetToLayer::SetCompletionCallback(FWxPushWidgetToLayerNativeDelegate InCompletionCallback)
{
	CompletionCallback = MoveTemp(InCompletionCallback);
}

void UWxAsyncAction_PushWidgetToLayer::Cancel()
{
	Finish(nullptr, true);
}

void UWxAsyncAction_PushWidgetToLayer::HandleWidgetClassLoaded()
{
	if (bFinished)
	{
		return;
	}

	TSubclassOf<UCommonActivatableWidget> LoadedClass = WidgetClass.Get();
	if (!LoadedClass)
	{
		Finish(nullptr);
		return;
	}

	UWxPrimaryGameLayout* Layout = TargetLayout.Get();
	if (!Layout)
	{
		Finish(nullptr);
		return;
	}

	UGameInstance* GameInstance = IsValid(WorldContextObject) ? UGameplayStatics::GetGameInstance(WorldContextObject) : nullptr;
	UWxUIManagerSubsystem* UIManager = GameInstance ? GameInstance->GetSubsystem<UWxUIManagerSubsystem>() : nullptr;
	if (!UIManager || UIManager->GetPrimaryGameLayout() != Layout)
	{
		Finish(nullptr);
		return;
	}

	APlayerController* OwningPlayer = Layout->GetOwningPlayer();
	if (!OwningPlayer)
	{
		Finish(nullptr);
		return;
	}

	UCommonActivatableWidget* WidgetInstance = CreateWidget<UCommonActivatableWidget>(OwningPlayer, LoadedClass);
	if (!WidgetInstance)
	{
		Finish(nullptr);
		return;
	}

	BeforePush.Broadcast(WidgetInstance);
	if (bFinished)
	{
		return;
	}

	if (!UIManager->PushWidgetInstanceToLayer(LayerTag, WidgetInstance))
	{
		Finish(nullptr);
		return;
	}
	if (bFinished)
	{
		WidgetInstance->DeactivateWidget();
		return;
	}

	AfterPush.Broadcast(WidgetInstance);
	if (bFinished)
	{
		WidgetInstance->DeactivateWidget();
		return;
	}
	Finish(WidgetInstance);
}

void UWxAsyncAction_PushWidgetToLayer::Finish(UCommonActivatableWidget* Widget, bool bCancelLoad)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	if (bCancelLoad && StreamableHandle.IsValid())
	{
		StreamableHandle->CancelHandle();
	}
	StreamableHandle.Reset();

	CompletionCallback.ExecuteIfBound(Widget);
	CompletionCallback.Unbind();
	TargetLayout.Reset();
	WorldContextObject = nullptr;
	SetReadyToDestroy();
}
