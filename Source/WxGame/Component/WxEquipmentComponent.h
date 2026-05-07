// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "Inventory/WxEquipmentInterface.h"
#include "Templates/SubclassOf.h"
#include "WxEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UWxItemDefinition;

/**
 * 장비 컴포넌트.
 * 현재 장착된 ItemDef 의 보관/복제, 외형(메시/소켓) 갱신, EquipEffect GE 라이프사이클을 캡슐화한다.
 *
 * 무기 액터의 스폰/유지는 본 컴포넌트가 아니라 캐릭터의 ChildActorComponent(WeaponChildActor)가 담당한다.
 * 본 컴포넌트는 ApplyEquipmentVisuals 에서 캐릭터의 GetEquippedWeapon() 을 통해 무기 액터에 접근한다.
 *
 * IWxEquipmentInterface를 구현하여 WxInventory에서 ItemDef 기반 장착 요청을 받는다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxEquipmentComponent : public UActorComponent, public IWxEquipmentInterface
{
	GENERATED_BODY()

public:
	UWxEquipmentComponent();

	// IWxEquipmentInterface
	virtual void EquipItem(const UWxItemDefinition* ItemDef) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 현재 장착 중인 아이템. 변경 시 무기 액터의 메시/부착 소켓이 fragment 기준으로 갱신된다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedItemDef, Category = "Wx|Equipment")
	TObjectPtr<const UWxItemDefinition> EquippedItemDef;

private:
	UFUNCTION()
	void OnRep_EquippedItemDef();

	/** EquippedItemDef에 따라 캐릭터의 무기 액터의 메시/소켓을 적용. 서버/클라이언트 공통 진입. */
	void ApplyEquipmentVisuals();

	/** 권한: 새 장비의 EquipEffects 를 소유자 ASC 에 적용하고 핸들을 보관. */
	void ApplyEquipEffects(const UWxItemDefinition* SourceDef, const TArray<TSubclassOf<UGameplayEffect>>& Effects);

	/** 권한: 현재 보관 중인 모든 EquipEffect 핸들을 제거하고 비운다. */
	void RemoveActiveEquipEffects();

	UAbilitySystemComponent* ResolveOwnerASC() const;

	/** 권한 측에서만 채워진다. 클라는 ASC 가 ActiveGameplayEffects 를 자동 복제 받음. */
	TArray<FActiveGameplayEffectHandle> ActiveEquipEffectHandles;
};
