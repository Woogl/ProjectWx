// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/WxEquipmentInterface.h"
#include "WxEquipmentComponent.generated.h"

class AWxWeaponBase;
class UWxItemDefinition;

/**
 * 장비 컴포넌트.
 * 캐릭터(또는 다른 액터)가 항상 소유하는 무기 액터의 스폰/유지와, 현재 장착된 ItemDef에 따른
 * 무기 외형/소켓 갱신을 캡슐화한다.
 *
 * 사용 흐름:
 *  1. 오너 액터의 생성자에서 서브오브젝트로 생성
 *  2. BP에서 WeaponActor / DefaultWeaponSocket 지정
 *  3. BeginPlay(Authority)에서 무기 스폰 → 캐릭터 소켓에 부착
 *  4. EquipItem(ItemDef) 호출 시 fragment에 따라 메시/소켓 갱신
 *
 * IWxEquipmentInterface를 구현하여 WxInventory에서 ItemDef 기반 장착 요청을 받는다.
 * 외형 갱신은 오너가 ACharacter일 때만 동작한다 (AWxWeaponBase::AttachToCharacter 시그니처 제약).
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxEquipmentComponent : public UActorComponent, public IWxEquipmentInterface
{
	GENERATED_BODY()

public:
	UWxEquipmentComponent();

	// IWxEquipmentInterface
	virtual void EquipItem(const UWxItemDefinition* ItemDef) override;

	AWxWeaponBase* GetEquippedWeapon() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 캐릭터가 항상 소유하는 무기 액터의 클래스. BeginPlay에서 스폰되어 DefaultWeaponSocket에 부착된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Equipment")
	TSubclassOf<AWxWeaponBase> WeaponActor;

	/** DefaultWeapon을 부착할 캐릭터 메시의 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Equipment")
	FName DefaultWeaponSocket = TEXT("hand_r");

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedWeapon, Category = "Wx|Equipment")
	TObjectPtr<AWxWeaponBase> EquippedWeapon;

	/** 현재 장착 중인 아이템. 변경 시 EquippedWeapon의 메시/부착 소켓이 fragment 기준으로 갱신된다. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedItemDef, Category = "Wx|Equipment")
	TObjectPtr<const UWxItemDefinition> EquippedItemDef;

private:
	void SpawnDefaultWeapon();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_EquippedItemDef();

	/** EquippedItemDef에 따라 EquippedWeapon의 메시/소켓을 적용. 서버/클라이언트 공통 진입. */
	void ApplyEquipmentVisuals();
};
