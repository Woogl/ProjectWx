// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxAlarmConsole.generated.h"

class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxAlarmConsoleState : uint8
{
	/** 대기 — 초기/기본. 인터랙션 활성. */
	Idle,
	/** 경보 완료 — 1회성 발동 완료. 인터랙션 비활성. */
	Alarmed
};

/**
 * 1회성 경보 콘솔.
 * 상호작용 시 권위 측이 State 를 Alarmed 로 확정한다. 발동 후 재상호작용 불가.
 * 상태는 자체 EWxAlarmConsoleState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * Niagara/사운드 재생과 인터랙션 비활성은 GimmickStateTree(ST_AlarmConsole)가 복제 State 를 추종해 적용한다(라이브 발동에서만 FX 재생, 복원 시 침묵).
 */
UCLASS(Abstract)
class WXWORLD_API AWxAlarmConsole : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxAlarmConsole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Play Fx 가 attach 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	/** 권위 측에서 State 를 Alarmed 로 확정한다. 동일값/비권위면 노옵. FX·인터랙션 토글은 StateTree 가 복제 State 를 추종해 적용한다. */
	void SetAlarmConsoleState(EWxAlarmConsoleState NewState);

	/** 콘솔 권위/영속 상태. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxAlarmConsoleState State = EWxAlarmConsoleState::Idle;
};
