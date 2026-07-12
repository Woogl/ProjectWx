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

/**
 * 아이템 등급. 색상/이펙트/드롭 가중치 등의 분기 키로 사용.
 */
UENUM(BlueprintType)
enum class EWxItemGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary
};

/**
 * 아이템 Fragment 의 베이스 UObject.
 *
 * UWxItemDefinition 의 Fragments 컬렉션에 EditInline 인스턴스로 부착되어, 아이템에 데이터·행동을 컴포지션한다.
 * Fragment 의 인스턴스는 정의(CDO) 안에 살아있는 단일 객체이며, 같은 정의를 참조하는 모든 인스턴스가 이를 공유한다.
 *
 * 카테고리 분류는 본 Fragment 가 아니라 UWxItemDefinition::Category(EWxItemCategory) 가 직접 표현한다.
 * Fragment 는 "이 아이템이 무엇을 할 수 있는가"(기능 축)만 책임지며, 카테고리에 종속되지 않는다.
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class WXINVENTORY_API UWxItemFragment : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 권한: AddItemDefinition 으로 새 인스턴스가 생성된 직후 호출.
	 * Fragment 가 인스턴스 초기 상태를 주입할 때 사용한다. 기본 구현은 비어있음.
	 */
	virtual void OnInstanceCreated(UWxItemInstance* Instance) const;
};

/**
 * 장착 시 무기 액터의 스켈레탈 메시를 교체하는 Fragment.
 *
 * 무기 액터 자체는 캐릭터가 항상 소유하므로(WxEquipmentComponent::WeaponActor), 장비 변경은 액터 스폰/디스트로이가 아닌 메시 스왑과 부착 소켓 변경으로만 수행한다.
 */
UCLASS(DisplayName = "Equippable")
class WXINVENTORY_API UWxItemFragment_Equippable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 무기 메시 컴포넌트에 적용할 스켈레탈 메시 */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** 장착 시 소유자 메시에서 이 장비를 부착할 소켓. 비어 있으면 메시 루트에 부착. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	FName AttachSocket = TEXT("hand_r");

	/** 장착 시 적용되고 해제 시 제거되는 GE 들. 스탯 모디파이어/패시브 효과용. */
	UPROPERTY(EditDefaultsOnly, Category = "Equippable")
	TArray<TSubclassOf<UGameplayEffect>> EquipEffects;
};

/**
 * 사용 시 사용자에게 GameplayEffect 를 적용하는 Fragment.
 *
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
 * 인스턴스 단위 충전(charge) 횟수를 부여하는 Fragment.
 *
 * 다크소울 에스트병 방식: 부착된 아이템은 인벤토리 스택이 아니라 인스턴스별 충전량으로 사용 가능 여부가 결정된다.
 * 사용 시 인벤토리 스택은 차감되지 않고(소진돼도 아이템은 인벤토리에 남는다) 충전량만 1 감소하며, 리필(UWxInventoryManagerComponent::RefillItemCharges)로 MaxCharges 까지 회복한다.
 *
 * 인스턴스 생성 시 MaxCharges 만큼 가득 채워진다(OnInstanceCreated).
 * 사용 처리(충전 검증·차감)와 회복 GE 적용은 UWxInventoryManagerComponent::UseItemByDef 가 담당한다.
 *
 * 기능 축은 Usable 과 직교한다.
 * 회복 등 사용 효과는 Usable Fragment 가 담당하므로, 충전형 소비 아이템은 Usable 과 함께 부착한다.
 * 단독 부착 시 충전만 소모된다.
 */
UCLASS(DisplayName = "Refill")
class WXINVENTORY_API UWxItemFragment_Charges : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 최대 충전 횟수. 인스턴스 생성 시 이 값으로 가득 채워지며, 리필의 상한이다. */
	UPROPERTY(EditDefaultsOnly, Category = "Charges", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

	/**
	 * 남은 충전수별 표시 아이콘. 인덱스 = 남은 충전 횟수(0=빈 상태, MaxCharges=가득)이므로 MaxCharges+1 개를 채운다.
	 * 비어 있거나 현재 충전수에 해당하는 항목이 비어 있으면 UWxItemDefinition 의 기본 Icon 으로 폴백한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Charges")
	TArray<TSoftObjectPtr<UTexture2D>> ChargeIcons;

	//~ Begin UWxItemFragment interface
	virtual void OnInstanceCreated(UWxItemInstance* Instance) const override;
	//~ End UWxItemFragment interface
};

/**
 * 스택 가능 아이템임을 선언하는 Fragment.
 *
 * 본 Fragment 가 부착된 아이템은 인벤토리에서 한 슬롯에 MaxStack 개까지 머지된다.
 * 부재 시에는 한 슬롯당 1개로 강제되어 스택되지 않는다 (장비처럼 인스턴스 고유 상태가 있는 아이템에 적합).
 */
UCLASS(DisplayName = "Stackable")
class WXINVENTORY_API UWxItemFragment_Stackable : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * 한 슬롯에 누적 가능한 최대 개수.
	 * 상한은 천만(1e7)으로 제한한다 — 동일 ItemDef 가 다수 슬롯에 분산되어도 합산 시 int32 안에서 안전.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stackable", meta = (ClampMin = "1", ClampMax = "10000000"))
	int32 MaxStack = 99;
};

/**
 * 픽업 액터(AWxItemPickup) 의 클래스와 외형을 정의하는 Fragment.
 *
 * 다양한 스포너(보물 상자, 적 드랍 등) 가 아이템별로 지정된 픽업 액터를 스폰하면서도 동일한 픽업 BP 를 재사용해 아이템별로 다른 메시/이펙트를 출력하도록 한다.
 *
 * 본 Fragment 는 데이터만 기술한다 — 실제 스폰은 스포너(UWxRewardLibrary::GrantReward), 컴포넌트 세팅은 픽업 액터 측에서 수행한다.
 */
UCLASS(DisplayName = "Pickup")
class WXINVENTORY_API UWxItemFragment_Pickup : public UWxItemFragment
{
	GENERATED_BODY()

public:
	/** 월드에 드랍될 때 스폰할 픽업 액터 클래스. 지급 데이터(아이템/수량)는 스포너가 스폰 후 주입한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftClassPtr<AWxItemPickup> ItemActorClass;

	/** 픽업 액터의 메시 컴포넌트에 적용할 스태틱 메시. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 픽업 액터의 나이아가라 컴포넌트에 적용할 시스템. 비어있으면 이펙트 비활성. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;
};

/**
 * 아이템 등급과 그 표시 색상을 선언하는 Fragment.
 *
 * Grade 는 색상/이펙트/드롭 가중치 등의 분기 키이며, Color 는 표시 색상이다.
 * 등급별 기본 색 팔레트는 프로그래머가 C++(GetDefaultColorForGrade)에만 정의하며 에디터에 노출하지 않는다.
 * 생성자가 그 팔레트로 Color 를 시드하고, 에디터에서 Grade 를 바꾸면 Color 가 해당 등급 기본값으로 자동 재시드된다(PostEditChangeProperty).
 * Color 는 EditDefaultsOnly 라 기획자가 아이템별로 오버라이드할 수 있다.
 *
 * Fragment 부재 아이템은 소비처에서 Common 등급/Common 색으로 폴백한다.
 */
UCLASS(DisplayName = "Grade")
class WXINVENTORY_API UWxItemFragment_Grade : public UWxItemFragment
{
	GENERATED_BODY()

public:
	UWxItemFragment_Grade();

	/** 등급별 기본 표시 색상(프로그래머 정의 팔레트). 매핑 없는 등급은 White. Color 시드·폴백에 사용. */
	static FLinearColor GetDefaultColorForGrade(EWxItemGrade Grade);

	/** 아이템 등급. 색상/이펙트/드롭 가중치 등의 분기 키. */
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	EWxItemGrade Grade = EWxItemGrade::Common;

	/** 표시 색상. Grade 의 기본 팔레트로 시드되며 아이템별로 오버라이드 가능. */
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	FLinearColor Color = FLinearColor::White;

#if WITH_EDITOR
	//~ Begin UObject interface — Grade 변경 시 Color 를 등급 기본값으로 재시드.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject interface
#endif
};
