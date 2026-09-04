// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayEffectTypes.h"
#include "Character/WxTeamTypes.h"
#include "WxCharacterBase.generated.h"

class UBehaviorTree;
class UChildActorComponent;
class UMotionWarpingComponent;
class UWxAbilitySystemComponent;
class UWxCombatAttributeSet;
class UWxEquipmentComponent;
class UWxHitStopComponent;
class UWxMetaHumanComponent;
class UWxMinionComponent;
class UWxProjectileComponent;
class AWxWeaponBase;
class USkeletalMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnDeathSignature, AWxCharacterBase*, DeadCharacter);

/**
 * 플레이어·에너미 공통 베이스 캐릭터.
 * ASC를 캐릭터에 직접 소유 (리스폰 시 스탯을 새로 초기화하므로 PlayerState 불필요).
 * ModularGameplay 컴포넌트 receiver 다 — 폰 대상 주입 요청(Experience 액션)의 컴포넌트가 자동 부착된다.
 *
 * 서브오브젝트 멤버는 모두 생성자에서 만들어 수명 내내 널이 아니다 — 파생 전부가 널 검사 없이 역참조한다.
 */
UCLASS(Abstract)
class WXGAME_API AWxCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AWxCharacterBase(const FObjectInitializer& ObjectInitializer);
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	//~ End IGameplayTagAssetInterface

	//~ Begin IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface

	bool IsAlive() const;
	AWxWeaponBase* GetEquippedWeapon() const;

	/** 비우면 AI 컨트롤러가 트리를 돌리지 않는다 — 플레이어처럼 사람이 모는 캐릭터가 그렇다. */
	UBehaviorTree* GetBehaviorTree() const;

	const FText& GetCharacterName() const;

	const TSoftObjectPtr<UObject>& GetPortrait() const;

	UPROPERTY(BlueprintAssignable, Category = "Wx|Character")
	FWxOnDeathSignature OnDeath;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Wx|GAS")
	TObjectPtr<UWxCombatAttributeSet> CombatAttributeSet;

	UPROPERTY(VisibleAnywhere, Category = "Wx|Combat")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	/** 히트스톱 GE의 추가·제거를 받아 메시의 애니메이션 시간을 세우고 되돌린다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Combat")
	TObjectPtr<UWxHitStopComponent> HitStopComponent;

	/** AnimNotify GameplayEvent를 받아 서버 권위로 투사체를 생성한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Combat")
	TObjectPtr<UWxProjectileComponent> ProjectileComponent;

	/** 소환 AnimNotify GameplayEvent를 받아 서버 권위로 소환물을 생성한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Combat")
	TObjectPtr<UWxMinionComponent> MinionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx|Equipment")
	TObjectPtr<UWxEquipmentComponent> EquipmentComponent;

	/**
	 * BP 의 ChildActorClass 에 구체 무기 BP 를 지정한다.
	 * 장착 변경 시 메시 스왑/소켓 재부착의 대상이 된다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Equipment")
	TObjectPtr<UChildActorComponent> WeaponActor;

	/**
	 * BP 디폴트에서 에셋을 지정한 캐릭터만 부착물을 만들고, 비워두면 아무것도 만들지 않는다.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Visual")
	TObjectPtr<UWxMetaHumanComponent> MetaHumanComponent;

	/**
	 * 서버: PossessedBy에서 호출.
	 * 클라이언트: 파생 클래스에서 OnRep을 통해 호출.
	 */
	virtual void InitAbilitySystem();

	void HandleSPDAttributeChanged(const FOnAttributeChangeData& Data);

	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void HandleRagdollTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void EnterRagdoll();

	void HandleEquipVisualChanged(USkeletalMesh* MeshAsset, FName Socket);

	virtual void HandleDeath();

	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI")
	FText CharacterName;

	/** UI 측에서 비동기 로드한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Portrait;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** 소비자가 모두 질의 시점에 이 값을 직접 읽으므로 복제 통지를 받을 대상이 없다. */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Wx|Team")
	EWxTeam Team = EWxTeam::Player;

	/** SPD Multiplier의 기준값 (cm/s) */
	float BaseWalkSpeed;
};
