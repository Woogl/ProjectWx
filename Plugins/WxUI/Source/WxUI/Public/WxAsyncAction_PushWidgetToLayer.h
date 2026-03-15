// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "WxAsyncAction_PushWidgetToLayer.generated.h"

class UCommonActivatableWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxPushWidgetToLayerDelegate, UCommonActivatableWidget*, Widget);

UCLASS()
class WXUI_API UWxAsyncAction_PushWidgetToLayer : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FWxPushWidgetToLayerDelegate BeforePush;

	UPROPERTY(BlueprintAssignable)
	FWxPushWidgetToLayerDelegate AfterPush;

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", GameplayTagFilter = "UI.Layer"))
	static UWxAsyncAction_PushWidgetToLayer* PushWidgetToLayer(UObject* WorldContextObject, FGameplayTag LayerTag, TSoftClassPtr<UCommonActivatableWidget> WidgetClass);

	virtual void Activate() override;

private:
	void HandleWidgetClassLoaded();

	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FGameplayTag LayerTag;

	UPROPERTY()
	TSoftClassPtr<UCommonActivatableWidget> WidgetClass;
};
