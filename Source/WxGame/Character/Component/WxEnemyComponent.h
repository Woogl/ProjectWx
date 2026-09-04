// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/WxEnemyTypes.h"
#include "Engine/DataTable.h"
#include "WxEnemyComponent.generated.h"

class AWxCharacterBase;
class AWxSpawner;
class USceneComponent;
class UWxNameplateComponent;
class UWxEnemyComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossReady, UWxEnemyComponent* /*EnemyComponent*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossEndPlay, UWxEnemyComponent* /*EnemyComponent*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnEnemyEngagementChanged, bool /*bEngaged*/);

/**
 * AWxEnemyCharacter의 적 역할 로직을 소유한다. 액터 단위 상호작용·스폰 계약은 조립 클래스가 이 컴포넌트로 전달한다.
 * 팀 소속과는 독립적으로 네임플레이트, 피니셔, 보상, 스포너 문맥만 소유한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxEnemyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AWxCharacterBase* GetEnemyCharacter() const;
	AWxSpawner* GetOwningSpawner() const;
	EWxEnemyRank GetEnemyRank() const;
	bool IsBoss() const;
	bool IsEngaged() const;

	FWxOnBossEndPlay OnBossEndPlay;
	FWxOnEnemyEngagementChanged OnEngagementChanged;

	/** Boss 등급의 EnemyComponent가 BeginPlay를 마칠 때 발행된다. */
	static FWxOnBossReady OnAnyBossReady;

	void HandleSpawnedBy(AWxSpawner* Spawner);

	bool CanInteract(const AActor* Interactor) const;
	void Interact(AActor* Interactor);
	FText GetInteractionPrompt() const;

private:
	UFUNCTION()
	void HandleAITargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleOwnerDeath(AWxCharacterBase* DeadCharacter);

	void HandleLockedOnChanged(bool bLockedOn);
	void RefreshNameplateVisibility();
	bool IsInRearCone(const AActor* Interactor) const;
	void SetEngaged(bool bInEngaged);

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Enemy")
	EWxEnemyRank EnemyRank = EWxEnemyRank::Normal;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Interaction", meta = (ClampMin = "0", ClampMax = "180"))
	float BackstabRearHalfAngle = 90.f;

	/** 처치 시 지급할 보상. 비우면 보상 없음. */
	UPROPERTY(EditAnywhere, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle RewardRow;

	/** 처치 시 스폰되는 픽업 보상의 수직 발사 속도(cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Reward", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	UPROPERTY(Transient)
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	TWeakObjectPtr<AWxSpawner> OwningSpawner;
	bool bDeathHandled = false;
	bool bEngaged = false;
};
