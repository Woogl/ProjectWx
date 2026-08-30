// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "WxAsyncAction_PushWidgetToLayer.generated.h"

class UCommonActivatableWidget;
class UWxPrimaryGameLayout;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxPushWidgetToLayerDelegate, UCommonActivatableWidget*, Widget);
DECLARE_DELEGATE_OneParam(FWxPushWidgetToLayerNativeDelegate, UCommonActivatableWidget*);

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

	/** C++ 호출자가 성공·실패를 모두 받고 진행 중인 요청 참조를 정리할 때 사용한다. 실패·취소 시 Widget 은 null 이다. */
	void SetCompletionCallback(FWxPushWidgetToLayerNativeDelegate InCompletionCallback);

	/** 아직 끝나지 않은 스트리밍을 취소하고 실패 완료 콜백을 보낸다. */
	void Cancel();

private:
	void HandleWidgetClassLoaded();

	void Finish(UCommonActivatableWidget* Widget, bool bCancelLoad = false);

	UPROPERTY()
	TObjectPtr<UObject> WorldContextObject;

	FGameplayTag LayerTag;

	UPROPERTY()
	TSoftClassPtr<UCommonActivatableWidget> WidgetClass;

	/** 로드 중 플레이어가 바뀌어 레이아웃이 교체되면 옛 요청 결과를 새 화면에 push 하지 않는다. */
	TWeakObjectPtr<UWxPrimaryGameLayout> TargetLayout;

	TSharedPtr<FStreamableHandle> StreamableHandle;

	FWxPushWidgetToLayerNativeDelegate CompletionCallback;

	bool bFinished = false;
};
