// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxAlarmConsole.generated.h"

class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 1회성 경보 콘솔.
 * 상호작용 시 Niagara/사운드를 재생한다. 한 번 발동된 뒤에는 재상호작용 불가.
 */
UCLASS(Abstract)
class WXWORLD_API AWxAlarmConsole : public AActor
{
	GENERATED_BODY()

public:
	AWxAlarmConsole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

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
	void HandleConsoleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bTriggered();

	void PlayAlarmFx();

	/** 발동 여부. 서버 권위로만 set, 발동 후엔 영구적으로 true. */
	UPROPERTY(ReplicatedUsing = OnRep_bTriggered)
	bool bTriggered = false;
};
