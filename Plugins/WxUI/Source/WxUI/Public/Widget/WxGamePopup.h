// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Widget/WxActivatableWidget.h"
#include "WxGamePopup.generated.h"

/** 팝업이 돌려주는 결과. */
UENUM(BlueprintType)
enum class EWxPopupResult : uint8
{
	/** "예/확인" 버튼을 눌렀다. */
	Confirmed,
	/** "아니오" 버튼을 눌렀다. */
	Declined,
	/** "취소" 버튼을 눌렀다. */
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
	/** 이 버튼을 눌렀을 때 돌려줄 결과. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWxPopupResult Result = EWxPopupResult::Unknown;

	/** 결과 기본 라벨 대신 표시할 텍스트(선택). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText OptionalDisplayText;

	bool operator==(const FWxConfirmationPopupAction& Other) const
	{
		return Result == Other.Result && OptionalDisplayText.EqualTo(Other.OptionalDisplayText);
	}
};

/** 팝업에 표시할 헤더/본문/버튼 구성을 담는 데이터 객체. */
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
	/** 표시할 헤더 텍스트. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Header;

	/** 표시할 본문 텍스트. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Body;

	/** 표시할 버튼 목록. */
	UPROPERTY(BlueprintReadWrite)
	TArray<FWxConfirmationPopupAction> ButtonActions;
};

/** 팝업 위젯 베이스. 실제 표시 로직은 서브클래스에서 구현한다. */
UCLASS(Abstract)
class WXUI_API UWxGamePopup : public UWxActivatableWidget
{
	GENERATED_BODY()

public:
	/** 서술자 내용으로 팝업을 채우고, 결과 콜백을 등록한다. */
	virtual void SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback);

	/** 사용자 입력 없이 팝업을 강제로 닫는다. */
	virtual void KillPopup();
};
