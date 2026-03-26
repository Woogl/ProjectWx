// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxDamageFloaterActor.generated.h"

class UWidgetComponent;
class UUserWidget;

/**
 * 데미지 플로터 액터.
 * 피격 위치에 스폰되어 WidgetComponent로 데미지 수치를 표시한다.
 * UWxCueNotify_Damage에서 직접 스폰하며, BP 서브클래스를 만들 필요 없다.
 *
 * InitDamageInfo로 전달받는 위젯 클래스는 IWxDamageFloaterInterface를 구현해야 한다.
 */
UCLASS()
class WXCOMBAT_API AWxDamageFloaterActor : public AActor
{
	GENERATED_BODY()

public:
	AWxDamageFloaterActor();

	void InitDamageInfo(TSubclassOf<UUserWidget> InWidgetClass, float InDamageAmount, bool bInIsCritical);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComponent;
};
