// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "WxInteractable.h"
#include "Spawnable/WxSpawnable.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class AWxSpawner;
class UBehaviorTree;
class UWxLockOnPointComponent;
class UWxNameplateComponent;

/**
 * - AWxEnemyController에 의해 제어
 * - 처치 시 UWxRewardLibrary::GrantReward 로 RewardRow 의 보상을 지급한다(픽업은 사망 위치에서 수직 발사, 재화는 직접 지급)
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
	
	//~ Begin IWxInteractable — 처형 상호작용.
	virtual bool CanInteract() const override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	virtual void BeginPlay() override;

protected:
	/** 피격을 촉각으로 보고하기 위해 대미지 어트리뷰트를 구독한다. */
	virtual void InitAbilitySystem() override;

	/**
	 * 받은 대미지를 AI Perception(촉각)에 보고해 가해자를 즉시 TargetActor 로 인지하게 한다.
	 *
	 * 이 자극은 피격 액터로 리스너를 역추적해 그 컨트롤러의 퍼셉션 컴포넌트에게만 가므로, 보고 주체는 피격자 자신이다.
	 */
	void HandleIncomingDamageChanged(const FOnAttributeChangeData& Data);

	/**
	 * 그로기면 앞잡(Event.Finisher, 방향 무관), 미인지·후방이면 뒤잡(Event.Backstab), 불가면 빈 태그.
	 * 이미 처형 연출 중(State.BeingFinished)이면 무조건 빈 태그다.
	 *
	 * 자격 판정의 단일 소스다 — 표시(CanBeInteractedBy, 클라가 로컬 폰으로)와 발동(OnInteracted, 서버가 실제 instigator 로)이 같은 주체 인자로 이 함수를 지난다.
	 * 판정 입력(HP·상태 태그·트랜스폼)이 전부 복제되므로 어느 머신에서 불러도 같은 주체엔 같은 답이 나온다.
	 */
	FGameplayTag GetEligibleFinisherEventTag(const AActor* Interactor) const;
	
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
