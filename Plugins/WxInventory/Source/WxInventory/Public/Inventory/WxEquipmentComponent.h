// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "WxEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UWxItemDefinition;

/**
 * 현재 장착된 ItemDef 의 보관/복제와 EquipEffect GE 라이프사이클을 캡슐화한다.
 *
 * 미구현: EquipItem 의 유일한 호출부인 UWxInventoryComponent::EquipItemByDef 를 부르는 곳이 없어(BlueprintCallable 도 아니라 BP 진입도 불가) EquippedItemDef 는 항상 null 이다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxEquipmentComponent();

	/**
	 * 권한: ItemDef 의 Equippable Fragment 에 따라 장착 효과를 반영.
	 * nullptr 이면 장착 해제.
	 */
	void EquipItem(const UWxItemDefinition* ItemDef);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Wx|Equipment")
	TObjectPtr<const UWxItemDefinition> EquippedItemDef;

private:
	void ApplyEquipEffects(const UWxItemDefinition* SourceDef, const TArray<TSubclassOf<UGameplayEffect>>& Effects);

	void RemoveActiveEquipEffects();

	UAbilitySystemComponent* ResolveOwnerASC() const;

	/**
	 * 권한 측에서만 채워진다.
	 * 클라는 ASC 가 ActiveGameplayEffects 를 자동 복제 받음.
	 */
	TArray<FActiveGameplayEffectHandle> ActiveEquipEffectHandles;
};
