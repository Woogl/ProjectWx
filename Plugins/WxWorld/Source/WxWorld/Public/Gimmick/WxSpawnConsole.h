// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawnConsole.generated.h"

class AWxSpawner;
class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 1회성 스폰 콘솔.
 * 상호작용 시 외부에 배치된 WxSpawner 들의 Respawn() 을 호출해 몬스터 스폰을 트리거한다.
 * 한 번 발동된 뒤에는 재상호작용 불가.
 *
 * 타겟 Spawner 들은 콘솔 상호작용 전엔 자동 스폰을 막아야 하므로, 디자이너가 각 Spawner 의
 * bSpawnOnBeginPlay 를 false 로 설정해야 한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxSpawnConsole : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawnConsole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> ConsoleMesh;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> ConsoleInteraction;

	/** 발동 시 Respawn() 을 호출할 외부 WxSpawner 들. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TSoftObjectPtr<AWxSpawner>> TargetSpawners;

private:
	UFUNCTION()
	void HandleConsoleInteracted(AActor* InteractingActor);

	UFUNCTION()
	void OnRep_bTriggered();

	/** 발동 여부. 서버 권위로만 set, 발동 후엔 영구적으로 true. */
	UPROPERTY(ReplicatedUsing = OnRep_bTriggered)
	bool bTriggered = false;
};
