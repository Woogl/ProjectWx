// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawner.generated.h"

class UArrowComponent;
class UBillboardComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS()
class WXWORLD_API AWxSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawner();

	/** 현재 스폰된 액터를 파괴하고 SpawnableActorClass로 새로 스폰한다. 서버 권한 필요. */
	void Respawn();

	/** 체크포인트 상호작용 등 일괄 리스폰의 대상인지 여부. false면 처치 시 영구 사망 처리(보스 등). */
	bool IsRespawnEnabled() const;

	/** 현재 이 Spawner가 보유한 spawned 액터. Subsystem 의 spawnable→spawner 역조회 등에 사용. */
	AActor* GetSpawnedActor() const;

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
