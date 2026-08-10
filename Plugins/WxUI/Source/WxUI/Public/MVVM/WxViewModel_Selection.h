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
 * 값은 외부 소스(도메인별 브리지)가 SetSelection/ClearSelection 으로 push 한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Selection : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 소스가 없는 필드는 빈 값으로 넘긴다. */
	void SetSelection(const FText& InDisplayName, const FText& InDescription, const TSoftObjectPtr<UObject>& InIcon);

	void ClearSelection();

	/** 상세 패널의 표시/숨김 바인딩용. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	bool bHasSelection = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	FText DisplayName;

	/** 소스가 제공하지 않으면 빈 텍스트. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	FText Description;

	/** 소스가 넘긴 Soft 참조를 베이스가 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Selection")
	TObjectPtr<UObject> Icon;

protected:
	//~ Begin UWxViewModel
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
	//~ End UWxViewModel
};
