// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Actor.generated.h"

/**
 * 범용 액터 표시 정보 뷰모델.
 * 네임플레이트 등에서 액터의 표시 이름을 UI 바인딩용으로 노출한다.
 *
 * WxUI 는 구체 액터 타입을 알지 못하므로, 표시할 이름은 소비 측(게임 모듈)이 대상 액터에서 적절한 값을 찾아 Initialize 로 주입한다.
 * 이름은 주입 시점에 1회 세팅되며 런타임에 변하지 않는다.
 */
UCLASS()
class WXUI_API UWxViewModel_Actor : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 표시할 이름을 주입한다. */
	void Initialize(const FText& InDisplayName);

	virtual void Deinitialize() override;

	/** 네임플레이트 등 UI 에 출력되는 액터 표시 이름. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Actor")
	FText DisplayName;
};
