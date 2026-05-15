// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSavableInterface.h"
#include "WxSpawner.generated.h"

class UArrowComponent;
class UBillboardComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * SpawnableActorClass 인스턴스를 스폰하는 레벨 배치 액터.
 *
 * 처치 상태(bIsKilled) 를 자체적으로 보유한다:
 *  - 처치 시 MarkKilled() → bIsKilled=true. 셀 언로드/리로드 사이엔 GUID 키로 보존 (WxSave 슬롯).
 *  - bEnableRespawn=true (일반): RespawnAll 호출 시 bIsKilled=false 로 리셋 후 새 인스턴스 생성.
 *  - bEnableRespawn=false (보스 등 영구 사망): RespawnAll 에서도 bIsKilled 그대로 유지, 새 인스턴스 생성 스킵.
 */
UCLASS()
class WXWORLD_API AWxSpawner : public AActor, public IWxSavableInterface
{
	GENERATED_BODY()

public:
	AWxSpawner();

	/** 현재 스폰된 액터를 파괴하고 SpawnableActorClass로 새로 스폰한다. 서버 권한 필요. 영구 처치는 스킵. */
	void Respawn();

	/** 체크포인트 상호작용 등 일괄 리스폰의 대상인지 여부. false면 처치 시 영구 사망 처리(보스 등). */
	bool IsRespawnEnabled() const;

	/** 현재 이 Spawner가 보유한 spawned 액터. Subsystem 의 spawnable→spawner 역조회 등에 사용. */
	AActor* GetSpawnedActor() const;

	/** 본 Spawner 가 처치 상태인지. */
	bool IsKilled() const;

	/** 서버 권위 호출. 처치 상태로 마킹. 인스턴스 destroy 는 호출자(또는 spawnable 자체) 가 별도 처리. */
	void MarkKilled();

	//~ Begin IWxSavableInterface
	virtual void OnWxSaveRestored() override;
	//~ End IWxSavableInterface

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnTarget();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxWorld.WxSpawnableInterface", AllowAbstract = "false"))
	TSubclassOf<AActor> SpawnableActorClass;

	/** false 면 BeginPlay 자동 스폰을 건너뛴다. 외부 트리거(Respawn) 로만 스폰하고 싶을 때는 false로 사용. (예: SpawnConsole 의 타겟 Spawner) */
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bSpawnOnBeginPlay = true;
	
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bEnableRespawn = true;

	/** 본 Spawner 가 처치 상태인지. WxSave 슬롯에 보존되어 셀 리로드/세션 간에 유지된다. */
	UPROPERTY(SaveGame)
	bool bIsKilled = false;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void UpdateEditorPreviewFromSpawnableClass();
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> PreviewSkeletalMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PreviewStaticMeshComponent;
#endif
};
