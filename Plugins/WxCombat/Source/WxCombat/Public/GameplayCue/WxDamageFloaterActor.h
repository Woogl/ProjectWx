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
 * BP 서브클래스에서 FloaterWidgetClass를 설정하고, 떠오르기·페이드 아웃·자기 파괴를 처리한다.
 *
 * FloaterWidgetClass는 IWxDamageFloaterInterface를 구현해야 한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxDamageFloaterActor : public AActor
{
	GENERATED_BODY()

public:
	AWxDamageFloaterActor();

	void InitDamageInfo(float InDamageAmount, bool bInIsCritical);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<UUserWidget> FloaterWidgetClass;
};
