// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "WxCharacterBase.generated.h"

class UWxAbilitySystemComponent;
class UWxAttributeSet;
class UWxGameplayAbility;
class UGameplayEffect;
class AWxWeaponBase;
class UWxRagdollComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnDeathSignature, AWxCharacterBase*, DeadCharacter);

/**
 * 플레이어·에너미 공통 베이스 캐릭터.
 * ASC와 AttributeSet을 캐릭터에 직접 소유.
 * 스탯 초기화는 DefaultAttributeEffect(GameplayEffect)로, 어빌리티는 DefaultAbilities로 부여.
 *
 * ASC를 캐릭터에 직접 소유 (리스폰 시 스탯을 새로 초기화하므로 PlayerState 불필요).
 */
UCLASS(Abstract)
class WXCOMBAT_API AWxCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AWxCharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UWxAttributeSet* GetAttributeSet() const { return AttributeSet; }
	bool IsAlive() const;
	AWxWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	/** HP == 0 시 호출. WxAttributeSet에서 호출하며, 파생 클래스에서 override하여 사망 연출 추가 */
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

	/** BP에서 기본 어빌리티 목록 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|GAS")
	TArray<TSubclassOf<UWxGameplayAbility>> DefaultAbilities;

	/** 초기 스탯 값을 설정하는 Instant GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

	virtual void BeginPlay() override;

	/** 서버·클라이언트 모두 호출되므로 파생 클래스에서 타이밍에 맞게 override */
	virtual void InitAbilityActorInfo();

	void GiveDefaultAbilities();
	void ApplyDefaultAttributes();

	/**
	 * SPD 어트리뷰트 변경 콜백.
	 * MaxWalkSpeed = BaseWalkSpeed * NewSPD 로 실제 이동 속도에 반영.
	 */
	void HandleSPDAttributeChanged(const FOnAttributeChangeData& Data);

	/** 기본 이동 속도 (cm/s). SPD Multiplier의 기준값. BP에서 설정 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Movement")
	float BaseWalkSpeed = 600.f;

	// ── Weapon ─────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	TSubclassOf<AWxWeaponBase> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Weapon")
	FName WeaponSocketName = TEXT("hand_r");

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Weapon")
	TObjectPtr<AWxWeaponBase> EquippedWeapon;

	void SpawnDefaultWeapon();

};
