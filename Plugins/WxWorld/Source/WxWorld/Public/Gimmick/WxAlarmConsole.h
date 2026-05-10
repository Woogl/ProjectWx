// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxAlarmConsole.generated.h"

class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 1회성 경보 콘솔.
 * 상호작용 시 Niagara/사운드를 재생한다. 발동 후 재상호작용 불가.
 */
UCLASS(Abstract)
class WXWORLD_API AWxAlarmConsole : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxAlarmConsole();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	/** 상호작용 시 콘솔에 attach 하여 재생할 Niagara 시스템. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraSystem> AlarmNiagaraSystem;

	/** 상호작용 시 콘솔 위치에서 재생할 사운드. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<USoundBase> AlarmSound;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	void PlayAlarmFx();
};
