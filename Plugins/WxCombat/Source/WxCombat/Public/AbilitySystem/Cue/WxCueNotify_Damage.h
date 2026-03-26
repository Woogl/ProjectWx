// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Blueprint/UserWidget.h"
#include "WxCueNotify_Damage.generated.h"

class UWidgetComponent;
class UNiagaraSystem;

/**
 * 데미지 플로터 GameplayCue 베이스 클래스.
 * HandleGameplayCue에서 DamageFloater 액터를 스폰한다.
 * InitDamageInfo로 전달받는 위젯 클래스는 IWxDamageFloaterInterface를 구현해야 한다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_Damage : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_Damage();

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	/** IWxDamageFloaterInterface를 구현하는 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<UUserWidget> FloaterWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;
};

/**
 * 데미지 플로터 액터.
 * 피격 위치에 스폰되어 WidgetComponent로 데미지 수치를 표시한다.
 * UWxCueNotify_Damage에서 직접 스폰하며, BP 서브클래스를 만들 필요 없다.
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