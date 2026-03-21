// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayEffectTypes.h"
#include "WxCharacterBase.generated.h"

class UWxAbilitySystemComponent;
class UWxAttributeSet;
class AWxWeaponBase;
class UWxRagdollComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnDeathSignature, AWxCharacterBase*, DeadCharacter);

/**
 * 플레이어·에너미 공통 베이스 캐릭터.
 * ASC를 캐릭터에 직접 소유 (리스폰 시 스탯을 새로 초기화하므로 PlayerState 불필요).
 */
UCLASS(Abstract)
class WXGAME_API AWxCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AWxCharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	bool IsAlive() const;
	AWxWeaponBase* GetEquippedWeapon() const;

	/** HP == 0 시 호출. 파생 클래스에서 override하여 사망 연출 추가 */
	virtual void HandleDeath();

	UPROPERTY(BlueprintAssignable, Category = "Wx|Character")
	FWxOnDeathSignature OnDeath;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|Ragdoll")
	TObjectPtr<UWxRagdollComponent> RagdollComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAttributeSet> AttributeSet;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void BeginPlay() override;

	/**
	 * ASC ActorInfo 설정, 어트리뷰트 콜백 등록, AbilitySet 부여를 수행.
	 * 서버: PossessedBy에서 호출. 클라이언트: 파생 클래스에서 OnRep을 통해 호출.
	 */
	virtual void InitAbilitySystem();

	/**
	 * SPD 어트리뷰트 변경 콜백.
	 * MaxWalkSpeed = BaseWalkSpeed * NewSPD 로 실제 이동 속도에 반영.
	 */
	void HandleSPDAttributeChanged(const FOnAttributeChangeData& Data);

	/** State_Dead 태그 변경 콜백. 태그 부여 시 HandleDeath 호출 */
	void HandleDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	/** 기본 이동 속도 (cm/s). SPD Multiplier의 기준값 */
	float BaseWalkSpeed = 600.f;

	// ── Weapon ─────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	TSubclassOf<AWxWeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	FName WeaponSocketName = TEXT("hand_r");

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Wx|Weapon")
	TObjectPtr<AWxWeaponBase> EquippedWeapon;

	void SpawnDefaultWeapon();

};
