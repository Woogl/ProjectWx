// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxDoor.generated.h"

class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxDoorState : uint8
{
	/** 닫힘 — 초기/기본. 콘솔 인터랙션 활성, 문 닫힘. 여닫는 "확정 목표"이며 슬라이드 도중도 이 값을 유지한다. */
	Close,
	/** 열림 — 문 개방. 콘솔 인터랙션은 에셋에서 비활성이라 현재 단방향. 여닫는 "확정 목표"이며 슬라이드 도중도 이 값을 유지한다. */
	Open
};

/**
 * 개폐 문.
 * 콘솔과 상호작용하면 양쪽 문이 반대 방향으로 슬라이드하며 열린다.
 * 구조상 다시 닫을 수도 있으나(Open ──상호작용──> Close), 현재는 Open 상태의 인터랙션을 에셋에서 비활성화해 단방향(열기 전용)으로 동작한다.
 *
 * 상태는 자체 EWxDoorState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * State 는 "여닫는 확정 목표"라 상호작용 시점에 곧장 최종값(Open/Close)으로 확정되고, 슬라이드 애니는 StateTree 의 Wx Component Move 가 그 목표를 향한 순수 비주얼로 처리한다(이동할 문 메시·오프셋은 ST_Door 에셋에서 author).
 *
 *   Close (초기) ──상호작용──> Open ──(양방향 시)상호작용──> Close
 *
 * 전이는 ST_Door 의 Enum Compare 전이 조건(State 가 현재 비주얼 상태와 달라지면 Root 재선택)이 구동한다(이벤트 태그 없음). State 는 슬라이드와 무관하게 즉시 확정되며, 클라 전이도 복제된 State 변화를 같은 Enum Compare 가 게이트한다.
 * 시작/복원 시엔 각 상태의 enter condition(State 에 바인딩한 Enum Compare)이 현재 State 로 초기 선택하고, 그 상태의 Wx Component Move 가 초기 진입 스냅으로 포즈를 맞춘다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDoor : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Component Move 가 Context 액터의 컴포넌트로 바인딩하기 위한 노출(State 의 Enum Compare 바인딩과 동일 패턴).
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorRight;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> Console;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InstigatorActor);

	/** 권한 측에서 현재 상태의 반대 목표로 State 를 확정한다. 동일값/비권위면 노옵. 슬라이드는 ST_Door 의 Wx Component Move 가 처리. */
	void SetDoorState(EWxDoorState NewState);

	/**
	 * 도어 권위/영속 상태. 클라는 Enum Compare 전이가 복제 State 를 폴링하므로 RepNotify 불필요.
	 * VisibleAnywhere + AllowPrivateAccess 는 StateTree 의 Enum Compare 조건(enter/전이)이 바인딩하기 위한 노출이다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxDoorState State = EWxDoorState::Close;
};
