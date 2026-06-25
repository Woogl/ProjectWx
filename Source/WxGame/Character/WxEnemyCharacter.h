// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxSpawnableInterface.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class AWxSpawner;
class UBehaviorTree;
class UWxLockOnPointComponent;
class UWxNameplateComponent;
class UWxRewardComponent;

/**
 * 에너미 캐릭터.
 * - AWxEnemyController에 의해 제어
 * - BehaviorTree를 BP에서 지정하여 적 종류별 행동 패턴 분리
 * - 처치 시 RewardComponent 가 보상 픽업을 스폰해 사망 위치에 흩뿌린다
 */
UCLASS(Abstract)
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter();

	UBehaviorTree* GetBehaviorTree() const;

	float GetSightRadius() const;
	float GetSightAngle() const;
	float GetMaxHearingRange() const;

	// IWxSpawnableInterface
	/** 스폰 직후 자신을 스폰한 Spawner 를 기억한다. 사망 시 순회 없이 해당 Spawner 에 처치 기록을 남기기 위함. */
	virtual void OnSpawnedBy(AWxSpawner* Spawner) override;
#if WITH_EDITOR
	virtual const UMeshComponent* GetEditorPreviewMeshComponent() const override;
#endif

protected:
	virtual void BeginPlay() override;

	/** 사망 시 자신을 스폰한 Spawner 에 처치 기록을 남긴다. */
	virtual void HandleDeath() override;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// AI 시야 감지 반경(cm). 빙의 시 컨트롤러가 Perception 컴포넌트에 주입한다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	float SightRadius = 1500.f;

	// 정면 기준 편측 시야각(half-angle, 도). 전체 시야각은 이 값의 2배(예: 60 → 120°).
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	float SightAngle = 60.f;

	// AI 청각 감지 최대 거리(cm).
	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	float MaxHearingRange = 1000.f;

	UPROPERTY(VisibleAnywhere, Category = "Wx|UI")
	TObjectPtr<UWxNameplateComponent> NameplateComponent;

	/** 락온 대상이 되는 지점. 메시의 pelvis 본에 부착되어 카메라·캐릭터 시선과 레티클·호밍이 이 위치를 향한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|LockOn")
	TObjectPtr<UWxLockOnPointComponent> LockOnPoint;

	/** 처치 시 지급되는 보상. 보상 로우/픽업 외형/발사 파라미터를 보유한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Reward")
	TObjectPtr<UWxRewardComponent> RewardComponent;

	/** 자신을 스폰한 Spawner. OnSpawnedBy 에서 세팅되며, 사망 시 처치 기록 대상. 직접 배치된 적은 비어 있다. */
	TWeakObjectPtr<AWxSpawner> OwningSpawner;
};
