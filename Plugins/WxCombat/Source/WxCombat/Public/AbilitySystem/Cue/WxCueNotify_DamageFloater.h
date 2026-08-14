// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Blueprint/UserWidget.h"
#include "WxCueNotify_DamageFloater.generated.h"

class UWidgetComponent;

/**
 * 데미지 플로터 GameplayCue 베이스 클래스.
 * 큐를 받으면 DamageFloater 액터를 스폰한다.
 *
 * 타격 임팩트 연출은 UWxCueNotify_Hit이 맡는다 — 그쪽은 예측되고 이쪽은 서버 권위다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_DamageFloater : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_DamageFloater();
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	/** IWxDamageFloaterInterface를 구현하는 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<UUserWidget> FloaterWidgetClass;
};

/**
 * 피격자 위치에 스폰돼 WidgetComponent로 데미지 수치를 표시하는 액터.
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

UINTERFACE(MinimalAPI)
class UWxDamageFloaterInterface : public UInterface
{
	GENERATED_BODY()
};

class WXCOMBAT_API IWxDamageFloaterInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Damage Floater")
	void InitDamageInfo(float DamageAmount, bool bIsCritical);
};