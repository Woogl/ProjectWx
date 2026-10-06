// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "Blueprint/UserWidget.h"
#include "WxCueNotify_DamageFloater.generated.h"

class UWidgetComponent;

/** 타격 임팩트 연출은 UWxCueNotify_Hit이 맡는다. */
UCLASS(Abstract, Blueprintable)
class WXGAME_API UWxCueNotify_DamageFloater : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_DamageFloater();
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

protected:
	/** UWxViewModel_Damage 를 Manual 뷰모델 소스로 둔 위젯이어야 값이 들어온다. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Floater")
	TSubclassOf<UUserWidget> FloaterWidgetClass;
};

UCLASS()
class WXGAME_API AWxDamageFloaterActor : public AActor
{
	GENERATED_BODY()

public:
	AWxDamageFloaterActor();

	void InitDamageInfo(TSubclassOf<UUserWidget> InWidgetClass, float InDamageAmount, bool bInIsCritical);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComponent;
};