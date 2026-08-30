// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSavable.h"
#include "WxSpawner.generated.h"

class UBillboardComponent;
class UChildActorComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EWxSpawnerMode : uint8
{
	/** BeginPlay 에서 자동으로 스폰한다. */
	Auto,

	/** BeginPlay 자동 스폰을 건너뛰고 외부 트리거로만 스폰한다. */
	Manual
};

/** SpawnableActorClass 인스턴스를 스폰하고, 그 처치 상태를 자체적으로 보유하는 레벨 배치 액터. */
UCLASS(NotBlueprintable)
class WXWORLD_API AWxSpawner : public AActor, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxSpawner();

	/** 서버 권한 필요. 영구 처치(bNeverRevive) 대상은 스킵. */
	void Respawn();

	/** Manual 은 일괄 리스폰(TryRespawnAll) 대상에서 제외되고 개별 트리거로만 스폰된다. */
	EWxSpawnerMode GetSpawnMode() const;

	bool IsKilled() const;

	/** 서버 권위 호출. 인스턴스 destroy 는 호출자(또는 spawnable 자체) 가 별도 처리. */
	void MarkKilled();

	//~ Begin IWxSavable
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnTarget();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxWorld.WxSpawnable", AllowAbstract = "false"))
	TSubclassOf<AActor> SpawnableActorClass;

	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxSpawnerMode SpawnMode = EWxSpawnerMode::Auto;
	
	/** true 면 처치 후 부활 금지(보스 등): 죽은 뒤 Respawn 이 호출돼도 새 인스턴스를 생성하지 않는다. 살아있을 땐 일반 대상처럼 리셋됨. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	bool bNeverRevive = false;

	/** WxSave 슬롯에 보존되어 셀 리로드/세션 간에 유지된다. */
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Wx")
	bool bIsKilled = false;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
public:
	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** 아웃라이너 기본 라벨. 스폰 대상 클래스를 밝힌 "Spawner_BP_Enemy" 형태로, 엔진이 중복 시 번호를 덧붙인다. */
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
