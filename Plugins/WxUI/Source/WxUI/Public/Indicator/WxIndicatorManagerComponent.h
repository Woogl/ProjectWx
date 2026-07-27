// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WxIndicatorManagerComponent.generated.h"

class UWxIndicatorDescriptor;
class UWxIndicatorWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnIndicatorChanged, UWxIndicatorDescriptor*);

/**
 * 로컬 플레이어가 화면에 띄울 인디케이터 목록을 들고 있는 컨트롤러 컴포넌트.
 * 등록증을 발급·회수하고 변경을 알리기만 하며, 화면에 어떻게 그리는지는 알지 않는다(HUD 의 인디케이터 캔버스가 구독해 그린다).
 * 표시는 보는 사람마다 다른 로컬 사건이라 월드가 아니라 컨트롤러에 매달린다.
 *
 * 부착은 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 목록으로 한다(컨트롤러는 본 클래스를 모른다).
 * 목록에는 사이드 구분이 없으므로 원격 사본(데디 서버가 들고 있는 PC)에서 등록을 거부하는 것은 본 클래스의 책임이다.
 */
UCLASS()
class WXUI_API UWxIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	/**
	 * 등록증을 발급해 목록에 넣고 캔버스에 알린다.
	 * 대상이나 위젯 클래스가 비면, 또는 오너가 로컬 컨트롤러가 아니면 표시될 수 없으므로 발급하지 않는다(null 반환).
	 */
	UWxIndicatorDescriptor* AddIndicator(USceneComponent* InTargetComponent, const TSoftClassPtr<UWxIndicatorWidget>& InIndicatorWidgetClass, const FVector& InWorldOffset);

	/** 등록증을 목록에서 빼고 캔버스에 알린다. 목록에 없는 등록증은 무시한다. */
	void RemoveIndicator(UWxIndicatorDescriptor* Indicator);

	/** 캔버스가 뒤늦게 붙을 때 이미 등록된 것들을 따라잡는 데 쓴다. */
	const TArray<TObjectPtr<UWxIndicatorDescriptor>>& GetIndicators() const { return Indicators; }

	/** 캔버스가 구독한다. 캔버스는 UObject 가 아닌 Slate 위젯이라 동적 델리게이트를 쓸 수 없다. */
	FWxOnIndicatorChanged OnIndicatorAdded;
	FWxOnIndicatorChanged OnIndicatorRemoved;

private:
	UPROPERTY()
	TArray<TObjectPtr<UWxIndicatorDescriptor>> Indicators;
};
