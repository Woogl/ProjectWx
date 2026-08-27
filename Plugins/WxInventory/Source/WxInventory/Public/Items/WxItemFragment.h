// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"

#include "WxItemFragment.generated.h"

class AWxItemPickup;
class UGameplayEffect;
class UNiagaraSystem;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UWxItemInstance;

UENUM(BlueprintType)
enum class EWxItemGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

/**
 * UWxItemDefinition 의 Fragments 컬렉션에 EditInline 인스턴스로 부착되어, 아이템에 데이터·행동을 컴포지션한다.
 * Fragment 는 정의 자산 안에 사는 단일 객체이며, 같은 정의를 참조하는 모든 아이템 인스턴스가 이를 공유한다.
 *
 * 카테고리 분류는 UWxItemDefinition::Category 가 직접 표현하며, Fragment 는 "이 아이템이 무엇을 할 수 있는가"(기능 축)만 책임진다.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class WXINVENTORY_API UWxItemFragment : public UObject
{
	GENERATED_BODY()

public:
	/** 권한: AddItemDefinition 으로 새 인스턴스가 생성된 직후 호출. */
	virtual void OnInstanceCreated(UWxItemInstance* Instance) const;
};

/**
 * 무기 액터 자체는 캐릭터가 ChildActor(AWxCharacterBase::WeaponActor) 로 항상 소유하므로, 장비 변경은 액터 스폰/디스트로이가 아닌 메시 스왑과 부착 소켓 변경으로만 수행한다.
 */
UCLASS(DisplayName = "Equippable")
class WXINVENTORY_API UWxItemFragment_Equippable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** 비어 있으면 재부착 자체를 건너뛰므로 직전 소켓이 그대로 유지된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	FName AttachSocket = TEXT("hand_r");

	/** 장착 시 적용되고 해제 시 제거되는 GE 들. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TArray<TSubclassOf<UGameplayEffect>> EquipEffects;
};

/**
 * 인벤토리 매니저의 UseItemByDef 가 Effect 적용과 스택 1 차감을 함께 수행한다.
 */
UCLASS(DisplayName = "Usable")
class WXINVENTORY_API UWxItemFragment_Usable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 사용 시 사용자(소유 폰)의 ASC에 적용할 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "Usable")
	TSubclassOf<UGameplayEffect> Effect;
};

/**
 * 다크소울 에스트병 방식: 부착된 아이템은 인벤토리 스택이 아니라 인스턴스별 충전량으로 사용 가능 여부가 결정된다.
 * 사용 시 인벤토리 스택은 차감되지 않고(소진돼도 아이템은 인벤토리에 남는다) 충전량만 1 감소하며, 리필(UWxInventoryManagerComponent::RefillItemCharges)로 MaxCharges 까지 회복한다.
 *
 * 사용 처리(충전 검증·차감)와 회복 GE 적용은 UWxInventoryManagerComponent::UseItemByDef 가 담당한다.
 *
 * 기능 축은 Usable 과 직교하며, Usable 없이 단독 부착하면 사용 자체가 성립하지 않아 충전도 소모되지 않는다.
 */
UCLASS(DisplayName = "Refill")
class WXINVENTORY_API UWxItemFragment_Charges : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 인스턴스 생성 시 이 값으로 가득 채워진다. */
	UPROPERTY(EditDefaultsOnly, Category = "Charges", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

	/**
	 * 인덱스 = 남은 충전 횟수(0=빈 상태, MaxCharges=가득)이므로 MaxCharges+1 개를 채운다.
	 * 비어 있거나 현재 충전수에 해당하는 항목이 비어 있으면 UWxItemDefinition 의 기본 Icon 으로 폴백한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Charges", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
	TArray<TSoftObjectPtr<UObject>> ChargeIcons;

	//~ Begin UWxItemFragment interface
	virtual void OnInstanceCreated(UWxItemInstance* Instance) const override;
	//~ End UWxItemFragment interface
};

/**
 * 본 Fragment 가 부착된 아이템은 인벤토리에서 한 슬롯에 MaxStack 개까지 머지된다.
 * 부재 시에는 한 슬롯당 1개로 강제되어 스택되지 않는다.
 */
UCLASS(DisplayName = "Stackable")
class WXINVENTORY_API UWxItemFragment_Stackable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * 상한을 둔 것은 동일 ItemDef 가 다수 슬롯에 분산되어도 합산이 int32 안에 들어오게 하기 위함이다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stackable", meta = (ClampMin = "1", ClampMax = "10000000"))
	int32 MaxStack = 99;
};

/**
 * 다양한 스포너(보물 상자, 적 드랍 등) 가 아이템별로 지정된 픽업 액터를 스폰하면서도 동일한 픽업 BP 를 재사용해 아이템별로 다른 메시/이펙트를 출력하도록 한다.
 *
 * 본 Fragment 는 데이터만 기술한다 — 실제 스폰은 스포너(UWxRewardLibrary::GrantReward), 컴포넌트 세팅은 픽업 액터 측에서 수행한다.
 */
UCLASS(DisplayName = "Pickup")
class WXINVENTORY_API UWxItemFragment_Pickup : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 지급 데이터(아이템/수량)는 스포너가 스폰 후 주입한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftClassPtr<AWxItemPickup> ItemActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 비어있으면 이펙트 비활성. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;
};

/**
 * 등급별 기본 색 팔레트는 프로그래머가 C++(GetDefaultColorForGrade)에만 정의하며 에디터에 노출하지 않는다.
 * 생성자가 그 팔레트로 Color 를 시드하고, 에디터에서 Grade 를 바꾸면 해당 등급 기본값으로 재시드된다.
 *
 * Fragment 부재 아이템은 소비처에서 Common 등급/Common 색으로 폴백한다.
 */
UCLASS(DisplayName = "Grade")
class WXINVENTORY_API UWxItemFragment_Grade : public UWxItemFragment
{
	GENERATED_BODY()

public:
	UWxItemFragment_Grade();

	/** 매핑 없는 등급은 White. */
	static FLinearColor GetDefaultColorForGrade(EWxItemGrade Grade);

	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	EWxItemGrade Grade = EWxItemGrade::Common;

	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	FLinearColor Color = FLinearColor::White;

#if WITH_EDITOR
	//~ Begin UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject interface
#endif
};
