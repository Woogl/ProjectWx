// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "WxEquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USkeletalMesh;
class UWxItemDefinition;

/**
 * 메시가 nullptr 이면 외형 유지(베이스), 소켓이 NAME_None 이면 재부착을 생략한다.
 * 실제 반영 정책은 바인딩한 게임 측이 결정한다.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FWxOnEquipVisualChanged, USkeletalMesh* /*Mesh*/, FName /*Socket*/);

/**
 * 현재 장착된 ItemDef 의 보관/복제와 EquipEffect GE 라이프사이클을 캡슐화한다.
 *
 * 무기 외형 반영(메시 스왑/소켓 재부착)은 본 컴포넌트가 직접 수행하지 않는다.
 * 무기 액터·캐릭터의 ChildActorComponent 는 게임 모듈 소유라 인벤토리 도메인에서 접근할 수 없으므로, Equippable 프래그먼트에서 뽑은 메시/소켓을 OnEquipVisualChanged 로 방송하고 게임 측이 반영한다.
 *
 * 미구현: EquipItem 의 유일한 호출부인 UWxInventoryManagerComponent::EquipItemByDef 를 부르는 곳이 없어(BlueprintCallable 도 아니라 BP 진입도 불가) EquippedItemDef 는 항상 null 이다.
 * 따라서 구독 측(캐릭터)이 붙어 있어도 OnEquipVisualChanged 방송과 EquipEffects 적용은 일어나지 않는다.
 * 경로를 닫을 때 함께 볼 것: 늦게 relevant 해진 클라는 초기 복제 RepNotify 가 구독보다 앞서면 방송을 유실하는데, 현재 상태를 되물을 pull API 가 없다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxEquipmentComponent();

	/**
	 * 권한: ItemDef 의 Equippable Fragment 에 따라 장착 효과/외형을 반영.
	 * nullptr 이면 장착 해제.
	 */
	void EquipItem(const UWxItemDefinition* ItemDef);

	/** 서버 EquipItem·클라 OnRep 양쪽에서 fire 된다. */
	FWxOnEquipVisualChanged OnEquipVisualChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EquippedItemDef, Category = "Wx|Equipment")
	TObjectPtr<const UWxItemDefinition> EquippedItemDef;

private:
	UFUNCTION()
	void OnRep_EquippedItemDef();

	void BroadcastEquipVisual();

	void ApplyEquipEffects(const UWxItemDefinition* SourceDef, const TArray<TSubclassOf<UGameplayEffect>>& Effects);

	void RemoveActiveEquipEffects();

	UAbilitySystemComponent* ResolveOwnerASC() const;

	/**
	 * 권한 측에서만 채워진다.
	 * 클라는 ASC 가 ActiveGameplayEffects 를 자동 복제 받음.
	 */
	TArray<FActiveGameplayEffectHandle> ActiveEquipEffectHandles;
};
