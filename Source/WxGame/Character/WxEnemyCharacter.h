// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxSpawnable.h"
#include "WxInteractable.h"
#include "Character/WxCharacterBase.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "WxEnemyCharacter.generated.h"

class AWxEnemyCharacter;
class AWxSpawner;
class UWxAIBehaviorComponent;
class UWxLockOnPointComponent;
class UWxNameplateComponent;
class UAbilitySystemComponent;
class USceneComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWxOnBossEngagementChanged, AWxEnemyCharacter* /*BossCharacter*/, bool /*bEngaged*/);

/** 적 캐릭터의 AI 조립, 상호작용, 보상, 보스 표시 상태를 소유한다. */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	bool IsBoss() const;
	AWxSpawner* GetOwningSpawner() const;

	/**
	 * 보스로 설정된 AI 캐릭터의 교전 상태가 바뀔 때 발행된다. 소멸도 비교전으로 알린다.
	 * 관찰자가 보스보다 먼저 생길 수 있어 클래스 차원에 둔다 — 어느 월드의 보스인지는 인자로 온 캐릭터가 말해 준다.
	 */
	static FWxOnBossEngagementChanged OnAnyBossEngagementChanged;

	//~ Begin IWxSpawnable
	virtual void OnSpawnedBy(AWxSpawner* Spawner) override;
	//~ End IWxSpawnable

	//~ Begin IWxInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx|AI")
	TObjectPtr<UWxAIBehaviorComponent> AIBehaviorComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	/** 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

private:
	UFUNCTION()
	void HandleAITargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleOwnerDeath(AWxCharacterBase* DeadCharacter);

	bool IsInRearCone(const AActor* Interactor) const;

	void RefreshEngagement();

	void ReleaseMasterStateTag();

	/** 소환물일 때만 주인 ASC를 돌려준다. 올릴 태그를 비워 두면 발행할 것이 없으므로 그때도 null. */
	UAbilitySystemComponent* GetMasterASC() const;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	bool bIsBoss = false;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Interaction", meta = (ClampMin = "0", ClampMax = "180"))
	float BackstabRearHalfAngle = 90.f;

	/**
	 * 소환되어 살아 있는 동안 주인 ASC에 카운트로 올려 두는 상태 태그.
	 * 소환물 종류를 가려야 하면 자식 태그를 지정한다 — 소유 태그는 부모까지 세므로 부모를 보는 요건은 그대로 물린다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion", meta = (Categories = "State.Minion"))
	FGameplayTag MasterStateTag;

	/** 처치 시 지급할 보상. */
	UPROPERTY(EditAnywhere, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle RewardRow;

	TWeakObjectPtr<AWxSpawner> OwningSpawner;
};
