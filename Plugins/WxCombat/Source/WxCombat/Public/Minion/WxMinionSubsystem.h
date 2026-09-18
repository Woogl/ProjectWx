// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxMinionSubsystem.generated.h"

class APawn;
class UWxMinionComponent;

/**
 * 소환물을 서버 권위로 생성해 주인별로 관리한다. AI 빙의는 소환물의 AutoPossessAI에 맡긴다.
 *
 * 주인·소환물 참조 규칙:
 * - 소환물 → 주인은 Instigator 하나가 답한다(UWxMinionComponent::GetMaster). 소환물이 죽어 로스터에서 내려가도 남는 영구 사실이다.
 * - 주인 → 소환물은 이 로스터 하나가 답한다. 살아 있는 소환물만 담고, 생성은 서버 권위여도 로스터는 모든 머신이 채운다.
 * - Owner는 소환 관계에 쓰지 않는다. 폰의 Owner는 빙의 시 Controller로 덮인다.
 * - 주인은 Pawn이다. 소환물이 주인을 Instigator로 무는 이상 다른 타입은 관계를 절반만 맺는다.
 *
 * 살아 있는 소환물 보유 여부는 로스터 변경 시 소환물이 선언한 태그로 주인 ASC에 복제한다.
 */
UCLASS()
class WXCOMBAT_API UWxMinionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 로스터에서 가장 먼저 소환된 활성 소환물을 반환한다. */
	APawn* FindActiveMinion(const APawn& Master) const;

	/**
	 * SpawnTransform 은 월드 기준이다. 소환물 클래스가 선언한 상한을 넘치면 주인의 가장 오래된 소환물부터 파괴한다.
	 * 소환물이 선언한 소환 가능 조건을 만족하지 못하거나 취소 조건을 이미 만족하면 아무것도 하지 않고 null 을 돌려준다.
	 * Master 가 소환물이어도 null 을 돌려준다.
	 */
	APawn* SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform);

	/** 주인의 소환물 중 MinionClass(하위 포함)인 것에 Event.Death 를 보내 사망 어빌리티로 거둔다. 파괴가 아니라 사망 연출과 시체 수명을 거친다. */
	void DespawnMinions(const APawn& Master, TSubclassOf<APawn> MinionClass);

	/**
	 * 소환물 컴포넌트가 BeginPlay·EndPlay에서 자기를 올리고 내린다. 사망도 내림으로 처리한다.
	 * 스폰 통지를 쓰지 않는 이유는 그 시점엔 복제 스폰의 Instigator가 아직 비어 주인을 못 읽기 때문이다.
	 */
	void RegisterMinion(UWxMinionComponent& MinionComponent);
	void UnregisterMinion(UWxMinionComponent& MinionComponent);

protected:
	/** 에디터 월드에서는 시퀀서 프리뷰의 노티파이가 권위를 통과해 레벨에 스폰해 버리므로 게임 월드에만 만든다. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/**
	 * 주인의 활성 소환물을 소환 순서로 모은다. 순회 중 파괴가 로스터를 바꿔도 이번 집합은 유지된다.
	 * StateTag 가 유효하면 그 태그를 선언한 소환물만 담는다 — 상한은 이 단위로 다툰다.
	 */
	TArray<TWeakObjectPtr<UWxMinionComponent>> CollectMinions(const APawn& Master, FGameplayTag StateTag = FGameplayTag()) const;

	UFUNCTION()
	void HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 로스터가 바뀐 쪽이 영향받는 태그를 넘기므로 직전 태그 집합을 들고 있지 않아도 된다. */
	void RefreshMasterStateTag(APawn& Master, FGameplayTag StateTag) const;

	/** 이 머신에 살아 있는 소환물 컴포넌트를 소환 순서로 담는다. 주인은 질의할 때 GetMaster로 거른다. */
	TArray<TWeakObjectPtr<UWxMinionComponent>> Minions;
};
