// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Item.generated.h"

struct FStreamableHandle;
class UWxInventoryComponent;
class UWxItemDefinition;
class UWxItemInstance;

/** 슬롯 VM 은 아이템 인스턴스를, 정의 단위 합계 VM 은 아이템 정의를 표시한다. Inventory VM 이 만들고 갱신을 지시한다. */
UCLASS()
class WXGAME_API UWxViewModel_Item : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 정의의 정적 정보를 채운다. 합계 VM 이면 InInstance 를 비운다. */
	void Initialize(const UWxItemDefinition& InDefinition, const UWxItemInstance* InInstance);

	/** 수량·충전·아이콘을 인벤토리의 현재 값으로 다시 채운다. 합계 VM 은 첫 스택을 대표로 보여 준다. */
	void Refresh(const UWxInventoryComponent& Inventory);

	const UWxItemDefinition* GetDefinition() const;

	/** 합계 VM 이면 nullptr. */
	const UWxItemInstance* GetInstance() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	FText DisplayName;

	/** 소프트 참조의 이미지를 비동기 로드하여 노출한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	TObjectPtr<UObject> Icon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	int32 TotalCount = 0;

	/** 충전형이 아니면 0. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	int32 CurrentCharges = 0;

	/**
	 * 토스트 등 1회용 표시 채널.
	 * 생성한 쪽이 공개 직전에 써넣고, 수신측은 OneTime 바인딩으로 읽는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wx|UI")
	int32 AcquiredCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	FLinearColor GradeColor = FLinearColor::White;

	/** 인벤토리 VM 의 탭 필터 키. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|UI", meta = (Categories = "Item.Category"))
	FGameplayTag Category;

private:
	void SetIcon(const TSoftObjectPtr<UObject>& InIcon);

	UPROPERTY(Transient)
	TObjectPtr<const UWxItemDefinition> Definition;

	UPROPERTY(Transient)
	TObjectPtr<const UWxItemInstance> Instance;

	TSharedPtr<FStreamableHandle> IconHandle;
};
