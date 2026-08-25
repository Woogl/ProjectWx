// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TimerHandle.h"
#include "WxInteractable.h"
#include "Spawnable/WxSpawnable.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class AWxSpawner;
class UBehaviorTree;
class UWxLockOnPointComponent;
class UWxNameplateComponent;

/**
 */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	UBehaviorTree* GetBehaviorTree() const;
	
	//~ Begin IWxSpawnable
	/** 사망 시 순회 없이 처치 기록을 남기기 위해 스폰 주체를 기억한다. */
	virtual void OnSpawnedBy(AWxSpawner* Spawner) override;
	//~ End IWxSpawnable
	
	//~ Begin IWxInteractable — Finisher 상호작용
	virtual bool CanInteract() const override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	virtual void BeginPlay() override;

protected:
	/**
	 * 로컬 플레이어 폰이 후방 원뿔 안에 있는가.
	 *
	 * 계약이 주체를 넘겨주지 않으므로 대상이 직접 찾는다 — 클라 표시에선 로컬 플레이어라 정확하고, 서버 검증에선 싱글·리슨호스트 호스트 기준으로 정확하다.
	 * 데디케이티드 멀티에서 2번째 이후 플레이어의 뒤잡은 0번 플레이어 위치로 판정된다.
	 */
	bool IsLocalPlayerInRearCone() const;

	/** 사망 시 자신을 스폰한 Spawner 에 처치 기록을 남긴다. */
	virtual void HandleDeath() override;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	/** 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

	/**
	 * 정면 기준 이 각도(도) 바깥의 후방 원뿔에 플레이어가 있어야 백스탭이 노출된다.
	 * 기본 90 = 후방 반구.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Interaction", meta = (ClampMin = "0", ClampMax = "180"))
	float BackstabRearHalfAngle = 90.f;

	/** 처치 시 지급할 보상. 비우면 보상 없음. */
	UPROPERTY(EditAnywhere, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow"))
	FDataTableRowHandle RewardRow;

	/**
	 * 처치 시 스폰되는 픽업 보상의 발사 속도(cm/s).
	 * 발사 방향은 항상 월드 Z 업(수직)이다.
	 * 비-픽업(재화) 보상엔 영향 없다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Reward", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	/** 직접 배치된 적은 비어 있다. */
	TWeakObjectPtr<AWxSpawner> OwningSpawner;
};
