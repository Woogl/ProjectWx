// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Widget/WxActivatableWidget.h"
#include "WxGamePopup.generated.h"

UENUM(BlueprintType)
enum class EWxPopupResult : uint8
{
	Confirmed,
	Declined,
	Cancelled,
	/** 사용자 입력 없이 강제로 종료됐다. */
	Killed,
	Unknown UMETA(Hidden)
};

DECLARE_DELEGATE_OneParam(FWxPopupResultDelegate, EWxPopupResult /*Result*/);

/** 팝업에 표시할 버튼 하나의 정의. */
USTRUCT(BlueprintType)
struct FWxConfirmationPopupAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWxPopupResult Result = EWxPopupResult::Unknown;

	/** 결과 기본 라벨 대신 표시할 텍스트(선택). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText OptionalDisplayText;

	bool operator==(const FWxConfirmationPopupAction& Other) const;
};

UCLASS(BlueprintType)
class WXUI_API UWxGamePopupDescriptor : public UObject
{
	GENERATED_BODY()

public:
	static UWxGamePopupDescriptor* CreateConfirmationOk(const FText& Header, const FText& Body);
	static UWxGamePopupDescriptor* CreateConfirmationOkCancel(const FText& Header, const FText& Body);
	static UWxGamePopupDescriptor* CreateConfirmationYesNo(const FText& Header, const FText& Body);
	static UWxGamePopupDescriptor* CreateConfirmationYesNoCancel(const FText& Header, const FText& Body);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Header;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Body;

	UPROPERTY(BlueprintReadWrite)
	TArray<FWxConfirmationPopupAction> ButtonActions;
};

UCLASS(Abstract, meta = (DisableNativeTick))
class WXUI_API UWxGamePopup : public UWxActivatableWidget
{
	GENERATED_BODY()

public:
	UWxGamePopup();
	
	virtual void SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback);

	/** 사용자 입력 없이 팝업을 강제로 닫는다. */
	virtual void KillPopup();
};
