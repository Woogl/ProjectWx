// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Item.generated.h"

/** 원본의 타입을 해석하지 않는다. 표시 데이터는 이 VM 을 소유한 도메인 VM 이 공급한다. */
UCLASS()
class WXUI_API UWxViewModel_Item : public UWxViewModel
{
	GENERATED_BODY()

public:
	void SetSourceObject(const UObject* InSourceObject);
	void SetDisplayName(const FText& InDisplayName);
	void SetIcon(const TSoftObjectPtr<UObject>& InIcon);
	void SetTotalCount(int32 InTotalCount);
	void SetCurrentCharges(int32 InCurrentCharges);
	void SetGradeColor(const FLinearColor& InGradeColor);

	UPROPERTY(Transient, BlueprintReadOnly, FieldNotify, Category = "Wx|UI")
	TObjectPtr<const UObject> SourceObject;

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

protected:
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
};
