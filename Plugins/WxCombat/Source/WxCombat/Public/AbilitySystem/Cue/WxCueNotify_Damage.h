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
 * 큐를 받으면 DamageFloater 액터를 스폰하며, 지정하는 위젯 클래스는 IWxDamageFloaterInterface를 구현해야 한다.
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

	/** 임팩트 위치에 스폰할 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	/** 임팩트 위치에 재생할 사운드 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<USoundBase> HitSound;
};

/**
 * 피격 위치에 스폰돼 WidgetComponent로 데미지 수치를 표시하는 액터.
 * UWxCueNotify_Damage가 직접 스폰하므로 BP 서브클래스를 만들 필요는 없다.
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

/** 데미지 플로터 위젯이 구현해야 하는 인터페이스 */
class WXCOMBAT_API IWxDamageFloaterInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Damage Floater")
	void InitDamageInfo(float DamageAmount, bool bIsCritical);
};