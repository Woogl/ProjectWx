// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxSpawnConsole.generated.h"

class AWxSpawner;
class UStaticMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxSpawnConsoleState : uint8
{
	/** 대기 — 초기/기본. 인터랙션 활성. */
	Idle,
	/** 스폰 완료 — 1회성 발동 완료. 인터랙션 비활성. */
	Spawned
};

/**
 * 1회성 스폰 콘솔.
 * 상호작용 시 권위 측이 State 를 Spawned 로 확정한다. 발동 후 재상호작용 불가.
 * 상태는 자체 EWxSpawnConsoleState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * 스포너 트리거(Respawn)와 인터랙션 비활성은 GimmickStateTree(ST_SpawnConsole)가 복제 State 를 추종해 적용한다(라이브 발동 권위에서만 Respawn, 복원 시 재실행 안 함).
 *
 * 타겟 Spawner 들은 콘솔 상호작용 전엔 자동 스폰을 막아야 하므로, 디자이너가 각 Spawner 의
 * SpawnMode 를 Manual 로 설정해야 한다. ST_SpawnConsole 의 Wx Trigger Spawners 가 이 TargetSpawners 를 바인딩한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxSpawnConsole : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxSpawnConsole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	//~ Begin AWxGimmick — State(EWxSpawnConsoleState) ↔ uint8 쓰기 매핑.
	virtual void SetGimmickState(uint8 NewStateValue) override;
	//~ End AWxGimmick

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> ConsoleMesh;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	// EditInstanceOnly + AllowPrivateAccess: 디자이너가 인스턴스마다 지정하고, StateTree 의 Wx Trigger Spawners 가 Context 액터 프로퍼티로 바인딩하기 위한 노출.
	/** 발동 시 Respawn() 을 호출할 외부 WxSpawner 들. */
	UPROPERTY(EditInstanceOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TArray<TSoftObjectPtr<AWxSpawner>> TargetSpawners;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	/** 콘솔 권위/영속 상태(복제 + SaveGame). State 쓰기는 권위 전용(CommitGimmickState)이며, 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxSpawnConsoleState State = EWxSpawnConsoleState::Idle;
};
