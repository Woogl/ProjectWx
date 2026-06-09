// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxSpawnableInterface.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class AWxSpawner;
class UBehaviorTree;
class UWxNameplateComponent;
class UWxPatrolComponent;

/**
 * 에너미 캐릭터.
 * - AWxEnemyController에 의해 제어
 * - BehaviorTree를 BP에서 지정하여 적 종류별 행동 패턴 분리
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

	/** 스폰 시 스포너에서 찾은 정찰 컴포넌트(없으면 null). 빙의한 컨트롤러가 읽는다. */
	UWxPatrolComponent* GetPatrolComponent() const;

	// IWxSpawnableInterface
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

	/** 스폰 시 스포너에서 찾은 정찰 컴포넌트. */
	UPROPERTY(Transient)
	TObjectPtr<UWxPatrolComponent> PatrolComponent;
};
