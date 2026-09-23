// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxSpawnable.h"
#include "WxInteractable.h"
#include "Character/WxCharacterBase.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "WxEnemyCharacter.generated.h"

class UWxAIBehaviorComponent;
class UWxLockOnPointComponent;
class UWxMinionComponent;
class UWxNameplateSourceComponent;
class UAbilitySystemComponent;
class USceneComponent;

/** 적 캐릭터의 AI 조립, 상호작용, 보상, 교전 상태를 소유한다. */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	//~ Begin IWxSpawnable
	virtual FWxOnSpawnableKilled& GetOnKilledDelegate() override;
	//~ End IWxSpawnable

	//~ Begin IWxInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx|AI")
	TObjectPtr<UWxAIBehaviorComponent> AIBehaviorComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateSourceComponent> NameplateSourceComponent;

	/** 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

	/** 소환물로 태어난 개체만 값이 의미를 갖는다. 그 판정은 컴포넌트가 Instigator로 한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Minion")
	TObjectPtr<UWxMinionComponent> MinionComponent;

private:
	UFUNCTION()
	void HandleAITargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleOwnerDeath(AWxCharacterBase* DeadCharacter);

	bool IsInRearCone(const AActor* Interactor) const;

	void RefreshEngagement();

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Interaction", meta = (ClampMin = "0", ClampMax = "180"))
	float BackstabRearHalfAngle = 90.f;

	UPROPERTY(EditAnywhere, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle RewardRow;

	FWxOnSpawnableKilled OnSpawnableKilled;
};
