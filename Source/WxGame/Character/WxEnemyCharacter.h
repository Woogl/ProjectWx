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
 * 에너미 캐릭터.
 * - AWxEnemyController에 의해 제어
 * - BehaviorTree를 BP에서 지정하여 적 종류별 행동 패턴 분리
 * - 처치 시 UWxRewardLibrary::GrantReward 로 RewardRow 의 보상을 지급한다(픽업은 사망 위치에서 수직 발사, 재화는 직접 지급)
 */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnable, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter();

	UBehaviorTree* GetBehaviorTree() const;

	/** 사망 시 자신을 스폰한 Spawner 에 처치 기록을 남긴다. */
	virtual void HandleDeath() override;

	//~ Begin IWxSpawnable
	/**
	 * 스폰 직후 자신을 스폰한 Spawner 를 기억한다.
	 * 사망 시 순회 없이 해당 Spawner 에 처치 기록을 남기기 위함.
	 */
	virtual void OnSpawnedBy(AWxSpawner* Spawner) override;
	//~ End IWxSpawnable

protected:
	virtual void BeginPlay() override;

	/**
	 * 주어진 상호작용 주체(Interactor) 기준으로 발동 가능한 처형 변형의 송출 이벤트 태그를 반환한다.
	 * 그로기면 앞잡(Event.Finisher, 방향 무관), 미인지·후방이면 뒤잡(Event.Backstab), 불가면 빈 태그. 이미 처형 연출 중(State.Finisher)이면 무조건 빈 태그다.
	 * 앞잡은 Interactor 위치를 쓰지 않고, 뒤잡의 후방 판정만 Interactor 위치를 쓴다.
	 *
	 * 자격 판정의 단일 소스다 — 표시(CanBeInteractedBy, 클라가 로컬 폰으로)와 발동(OnInteracted, 서버가 실제 instigator 로)이 같은 주체 인자로 이 함수를 지난다.
	 * 판정 입력(HP·상태 태그·트랜스폼)이 전부 복제되므로 어느 머신에서 불러도 같은 주체엔 같은 답이 나온다.
	 */
	FGameplayTag GetEligibleFinisherEventTag(const AActor* Interactor) const;

	//~ Begin IWxInteractable — 처형 상호작용(영역 + 자격 + 응답 + "Finisher" 프롬프트).
	// 파라미터명이 InMesh 인 것은 ACharacter::Mesh 멤버를 가리지 않기 위해서다.
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* InMesh) const override;
	virtual bool CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const override;
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	/**
	 * 락온 대상이 되는 지점.
	 * 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

	/**
	 * 백스탭 후방 판정 반각(도).
	 * 정면 기준 이 각도 바깥(후방 원뿔)에 플레이어가 있어야 백스탭이 노출된다.
	 * 기본 90 = 후방 반구.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Interaction", meta = (ClampMin = "0", ClampMax = "180"))
	float BackstabRearHalfAngle = 90.f;

	/**
	 * 처치 시 지급할 보상.
	 * FWxRewardTableRow 로우.
	 * 비우면 보상 없음.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow"))
	FDataTableRowHandle RewardRow;

	/**
	 * 처치 시 스폰되는 픽업 보상의 발사 속도(cm/s).
	 * 발사 방향은 항상 월드 Z 업(수직)이다.
	 * 비-픽업(재화) 보상엔 영향 없다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Reward", meta = (ClampMin = "0"))
	float LaunchSpeed = 300.f;

	/**
	 * 자신을 스폰한 Spawner.
	 * OnSpawnedBy 에서 세팅되며, 사망 시 처치 기록 대상.
	 * 직접 배치된 적은 비어 있다.
	 */
	TWeakObjectPtr<AWxSpawner> OwningSpawner;
};
