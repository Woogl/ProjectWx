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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnDeathSignature, AWxCharacterBase*, DeadCharacter);

/**
 * 플레이어·에너미 공통 베이스 캐릭터.
 * ASC와 AttributeSet을 캐릭터에 직접 소유.
 * 스탯 초기화는 DefaultAttributeEffect(GameplayEffect)로, 어빌리티는 DefaultAbilities로 부여.
 */
UCLASS(Abstract)
class WXCOMBAT_API AWxCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AWxCharacterBase();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Wx|Character")
	UWxAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintCallable, Category = "Wx|Character")
	bool IsAlive() const;

	UPROPERTY(BlueprintAssignable, Category = "Wx|Character")
	FWxOnDeathSignature OnDeath;

protected:
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

	/** HP == 0 시 호출. 파생 클래스에서 override하여 사망 연출 추가 */
	virtual void HandleDeath();

	/**
	 * SPD 어트리뷰트 변경 콜백.
	 * MaxWalkSpeed = BaseWalkSpeed * NewSPD 로 실제 이동 속도에 반영.
	 */
	void OnSPDAttributeChanged(const FOnAttributeChangeData& Data);

	/** 기본 이동 속도 (cm/s). SPD Multiplier의 기준값. BP에서 설정 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Movement", meta = (AllowPrivateAccess = "true"))
	float BaseWalkSpeed = 600.f;

private:
	FGameplayTag DeadTag;
};
