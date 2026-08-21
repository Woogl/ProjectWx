// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayEffectTypes.h"
#include "WxTeamTypes.h"
#include "WxCharacterBase.generated.h"

class UChildActorComponent;
class UMotionWarpingComponent;
class UWxAbilitySystemComponent;
class UWxCombatAttributeSet;
class UWxEquipmentComponent;
class UWxMetaHumanComponent;
class AWxWeaponBase;
class USkeletalMesh;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnDeathSignature, AWxCharacterBase*, DeadCharacter);

/**
 * 플레이어·에너미 공통 베이스 캐릭터.
 * ASC를 캐릭터에 직접 소유 (리스폰 시 스탯을 새로 초기화하므로 PlayerState 불필요).
 * ModularGameplay 컴포넌트 receiver 다 — 폰 대상 주입 요청(Experience 액션)의 컴포넌트가 자동 부착된다.
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
	virtual bool CanJumpInternal_Implementation() const override;

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

	const FText& GetCharacterName() const;

	const TSoftObjectPtr<UObject>& GetPortrait() const;

	/** Ability.Death 태그 부여 시 호출. */
	virtual void HandleDeath();

	UPROPERTY(BlueprintAssignable, Category = "Wx|Character")
	FWxOnDeathSignature OnDeath;

protected:
	/** 생성자 서브오브젝트라 수명 내내 널이 아니다 — 파생 전부가 널 검사 없이 역참조한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Wx|GAS")
	TObjectPtr<UWxCombatAttributeSet> CombatAttributeSet;

	UPROPERTY(VisibleAnywhere, Category = "Wx|Combat")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

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

	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI")
	FText CharacterName;

	/** UI 측에서 비동기 로드한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|UI", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UObject> Portrait;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Team")
	EWxTeam Team = EWxTeam::Player;

	/** 기본 이동 속도 (cm/s). SPD Multiplier의 기준값 */
	float BaseWalkSpeed;
};
