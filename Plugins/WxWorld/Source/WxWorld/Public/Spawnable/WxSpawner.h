// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxSavable.h"
#include "WxSpawner.generated.h"

class UBillboardComponent;
class UChildActorComponent;
class USceneComponent;

/** Spawner 의 스폰 트리거 방식. */
UENUM(BlueprintType)
enum class EWxSpawnerMode : uint8
{
	/** BeginPlay 에서 자동으로 스폰한다. */
	Auto,

	/** BeginPlay 자동 스폰을 건너뛰고 외부 트리거로만 스폰한다. */
	Manual
};

/**
 * SpawnableActorClass 인스턴스를 스폰하는 레벨 배치 액터.
 *
 * 처치 상태(bIsKilled) 를 자체적으로 보유한다:
 *  - 처치 시 MarkKilled() → bIsKilled=true. 셀 언로드/리로드 사이엔 GUID 키로 보존 (WxSave 슬롯).
 *  - bNeverRevive=false (일반): Respawn 호출 시 bIsKilled=false 로 리셋 후 새 인스턴스 생성.
 *  - bNeverRevive=true (보스 등): 죽은 뒤 Respawn 이 호출돼도 부활하지 않음(bIsKilled 유지, 생성 스킵). 살아있을 땐 일반 대상처럼 리셋됨.
 */
UCLASS(NotBlueprintable)
class WXWORLD_API AWxSpawner : public AActor, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxSpawner();

	/** 현재 스폰된 액터를 파괴하고 SpawnableActorClass로 새로 스폰한다. 서버 권한 필요. 영구 처치는 스킵. */
	void Respawn();

	/** 스폰 트리거 방식. Manual 은 일괄 리스폰(TryRespawnAll) 대상에서 제외되고 개별 트리거로만 스폰된다. */
	EWxSpawnerMode GetSpawnMode() const;

	/** 본 Spawner 가 처치 상태인지. */
	bool IsKilled() const;

	/** 서버 권위 호출. 처치 상태로 마킹. 인스턴스 destroy 는 호출자(또는 spawnable 자체) 가 별도 처리. */
	void MarkKilled();

	//~ Begin IWxSavable
	virtual FGuid GetSaveId() const override;
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnTarget();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxWorld.WxSpawnableInterface", AllowAbstract = "false"))
	TSubclassOf<AActor> SpawnableActorClass;

	/** 스폰 트리거 방식. Auto 면 BeginPlay 에서 자동 스폰, Manual 이면 외부 트리거(Respawn) 로만 스폰. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxSpawnerMode SpawnMode = EWxSpawnerMode::Auto;
	
	/** true 면 처치 후 부활 금지(보스 등): 죽은 뒤 Respawn 이 호출돼도 새 인스턴스를 생성하지 않는다. 살아있을 땐 일반 대상처럼 리셋됨. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (EditCondition = "SpawnMode == EWxSpawnerMode::Auto", EditConditionHides))
	bool bNeverRevive = false;

	/** 본 Spawner 가 처치 상태인지. WxSave 슬롯에 보존되어 셀 리로드/세션 간에 유지된다. */
	UPROPERTY(SaveGame)
	bool bIsKilled = false;

	/** WxSave 슬롯 레코드의 안정적 키. 에디터에서 부여되어 에셋에 직렬화되고, 런타임/세션 간 불변이다. */
	UPROPERTY()
	FGuid SaveId;

	TWeakObjectPtr<AActor> SpawnedActor;

#if WITH_EDITOR
public:
	virtual void PostActorCreated() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** 아웃라이너 기본 라벨. 스폰 대상 클래스를 밝힌 "Spawner_Enemy" 형태로, 엔진이 중복 시 번호를 덧붙인다. */
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
