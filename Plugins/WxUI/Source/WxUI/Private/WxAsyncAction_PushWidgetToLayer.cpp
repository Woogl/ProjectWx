// Copyright Woogle. All Rights Reserved.

#include "WxAsyncAction_PushWidgetToLayer.h"
#include "WxUIManagerSubsystem.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameInstance.h"

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
	if (!WorldContextObject || WidgetClass.IsNull())
	{
		SetReadyToDestroy();
		return;
	}

	if (WidgetClass.Get())
	{
		HandleWidgetClassLoaded();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	StreamableManager.RequestAsyncLoad(
		WidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ThisClass::HandleWidgetClassLoaded)
	);
}

void UWxAsyncAction_PushWidgetToLayer::HandleWidgetClassLoaded()
{
	if (!WorldContextObject || !WidgetClass.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		SetReadyToDestroy();
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		SetReadyToDestroy();
		return;
	}

	UWxUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UWxUIManagerSubsystem>();
	if (!UIManager)
	{
		SetReadyToDestroy();
		return;
	}

	TSubclassOf<UCommonActivatableWidget> LoadedClass = WidgetClass.Get();
	if (!LoadedClass)
	{
		SetReadyToDestroy();
		return;
	}

	UCommonActivatableWidget* WidgetInstance = UIManager->PushContentToLayer(LayerTag, LoadedClass);
	if (!WidgetInstance)
	{
		SetReadyToDestroy();
		return;
	}

	BeforePush.Broadcast(WidgetInstance);
	AfterPush.Broadcast(WidgetInstance);

	SetReadyToDestroy();
}
