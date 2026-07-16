// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Selection.generated.h"

class UTexture2D;

/**
 * "현재 선택된 대상 하나" 의 상세 표시를 노출하는 범용 뷰모델.
 * 상호작용/인벤토리 등 소스와 무관하게 상세·설명 패널이 동일 계약에 바인딩하도록, 도메인 타입을 참조하지 않는 평면 표시 필드만 노출한다(그래서 WxUI 에 둔다).
 *
 * 값은 외부 소스(도메인별 브리지)가 SetSelection/ClearSelection 으로 push 하며, 본 VM 은 무엇이 선택되었는지·어떻게 선택되는지 알지 못한다(순수 표시 계약).
 */
UCLASS()
class WXUI_API UWxViewModel_Selection : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 선택 대상의 상세를 반영하고 bHasSelection 을 켠다. 소스가 없는 필드는 빈 값으로 넘긴다. */
	void SetSelection(const FText& InDisplayName, const FText& InDescription, const TSoftObjectPtr<UTexture2D>& InIcon);

	/** 선택 없음 상태로 되돌린다(패널 숨김용). 표시 필드를 비운다. */
	void ClearSelection();

	/** 현재 선택된 대상이 있는지 여부. 상세 패널의 표시/숨김 바인딩용. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	bool bHasSelection = false;

	/** 선택 대상의 표시 이름. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	FText DisplayName;

	/** 선택 대상의 설명. 소스가 제공하지 않으면 빈 텍스트. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	FText Description;

	/**
	 * 선택 대상의 아이콘 Soft 참조. View 측 UCommonLazyImage 가 비동기 로드/수명 관리한다.
	 * VM 은 Soft 참조를 그대로 노출만 하며 LoadSynchronous 를 호출하지 않는다(WxViewModel_Item::Icon 과 동일 관례).
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	TSoftObjectPtr<UTexture2D> Icon;
};
