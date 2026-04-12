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
	StreamableHandle = StreamableManager.RequestAsyncLoad(
		WidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ThisClass::HandleWidgetClassLoaded)
	);
}

void UWxAsyncAction_PushWidgetToLayer::HandleWidgetClassLoaded()
{
	if (!IsValid(WorldContextObject) || !WidgetClass.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
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

	UWxPrimaryGameLayout* Layout = UIManager->GetPrimaryGameLayout();
	if (!Layout)
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

	APlayerController* OwningPlayer = Layout->GetOwningPlayer();
	if (!OwningPlayer)
	{
		SetReadyToDestroy();
		return;
	}

	UCommonActivatableWidget* WidgetInstance = CreateWidget<UCommonActivatableWidget>(OwningPlayer, LoadedClass);
	if (!WidgetInstance)
	{
		SetReadyToDestroy();
		return;
	}

	BeforePush.Broadcast(WidgetInstance);

	if (!UIManager->PushWidgetInstanceToLayer(LayerTag, WidgetInstance))
	{
		SetReadyToDestroy();
		return;
	}

	AfterPush.Broadcast(WidgetInstance);

	SetReadyToDestroy();
}
