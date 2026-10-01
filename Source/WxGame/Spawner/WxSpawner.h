// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSpawner.generated.h"

class UBillboardComponent;
class UChildActorComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EWxSpawnerMode : uint8
{
	/** BeginPlay 에서 곧바로 스폰한다. */
	Auto,

	/** BeginPlay 자동 스폰을 건너뛰고 Respawn() 외부 트리거로만 스폰한다. */
	Manual
};

/** SpawnableActorClass 인스턴스를 스폰하고, 그 처치 상태를 자체적으로 보유하는 레벨 배치 액터. */
UCLASS(NotBlueprintable)
class WXGAME_API AWxSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWxSpawner();

	/** 현재 로드된 스포너를 서버에서 재생성한다. Manual 모드는 제외한다. */
	static void RespawnAll(const UWorld* World);

	/** 서버 권한 필요. 영구 처치(bNeverRevive) 대상은 스킵. */
	void Respawn();

	EWxSpawnerMode GetSpawnMode() const;

	bool IsKilled() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnTarget();

	/** 서버 전용. */
	void DestroySpawnedActor();

	/** 시체는 치우지 않는다 — 다음 Respawn 이 정리한다. */
	void HandleSpawnedActorKilled();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxGame.WxSpawnable", AllowAbstract = "false"))
	TSubclassOf<AActor> SpawnableActorClass;

	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxSpawnerMode SpawnMode = EWxSpawnerMode::Auto;
	
	/** 처치된 뒤에는 Respawn 이 와도 새로 생성하지 않는다(보스 등). 살아 있으면 일반 대상처럼 리셋한다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bNeverRevive = false;

	/** 서버 런타임 상태다 — 셀이 스트림 아웃되면 함께 사라진다. */
	bool bIsKilled = false;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
public:
	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** "Spawner_BP_Enemy" 형태로 스폰 대상 클래스를 밝히고, 중복이면 엔진이 번호를 덧붙인다. */
	virtual FString GetDefaultActorLabel() const override;

	void UpdateEditorPreviewFromSpawnableClass();
#endif

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY(Transient)
	TObjectPtr<UBillboardComponent> SpriteComponent;

	/**
	 * SpawnableActorClass 인스턴스를 에디터 뷰포트에 그대로 세우는 프리뷰.
	 * 에디터 월드에서만 RF_Transient 로 생성되므로 게임 월드에는 존재하지 않고, 자식 액터도 스포너 패키지에 직렬화되지 않는다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UChildActorComponent> PreviewChildActorComponent;
#endif
};
