// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonTabListWidgetBase.h"
#include "Styling/SlateBrush.h"

#include "WxTabListWidgetBase.generated.h"

class UCommonAnimatedSwitcher;
class UCommonButtonBase;
class UCommonUserWidget;
class UPanelWidget;
class UWidget;

/** 프리레지스터 배열 또는 런타임 동적 등록으로 쓰인다. */
USTRUCT(BlueprintType)
struct FWxTabDescriptor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName TabId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText TabText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHidden = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UCommonButtonBase> TabButtonType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UCommonUserWidget> TabContentType;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CreatedTabContentWidget = nullptr;
};

UINTERFACE(BlueprintType)
class UWxTabButtonInterface : public UInterface
{
	GENERATED_BODY()
};

class IWxTabButtonInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Tab Button")
	void SetTabLabelInfo(const FWxTabDescriptor& TabDescriptor);
};

UCLASS(Blueprintable, BlueprintType, Abstract, meta = (DisableNativeTick))
class WXUI_API UWxTabListWidgetBase : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tab List")
	bool GetPreregisteredTabInfo(const FName TabNameId, FWxTabDescriptor& OutTabInfo);

	const TArray<FWxTabDescriptor>& GetAllPreregisteredTabInfos();

	// 스위처와 연결되기 전에만 호출할 수 있다.
	UFUNCTION(BlueprintCallable, Category = "Tab List")
	void SetTabHiddenState(FName TabNameId, bool bHidden);

	UFUNCTION(BlueprintCallable, Category = "Tab List")
	bool RegisterDynamicTab(const FWxTabDescriptor& TabDescriptor);

	UFUNCTION(BlueprintCallable, Category = "Tab List")
	bool IsFirstTabActive() const;

	UFUNCTION(BlueprintCallable, Category = "Tab List")
	bool IsLastTabActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tab List")
	bool IsTabVisible(FName TabId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tab List")
	int32 GetVisibleTabCount();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTabContentCreated, FName, TabId, UCommonUserWidget*, TabWidget);
	DECLARE_EVENT_TwoParams(UWxTabListWidgetBase, FOnTabContentCreatedNative, FName /* TabId */, UCommonUserWidget* /* TabWidget */);

	UPROPERTY(BlueprintAssignable, Category = "Tab List")
	FOnTabContentCreated OnTabContentCreated;
	FOnTabContentCreatedNative OnTabContentCreatedNative;

protected:
	// UUserWidget interface
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// End UUserWidget

	virtual void HandlePreLinkedSwitcherChanged() override;
	virtual void HandlePostLinkedSwitcherChanged() override;

	virtual void HandleTabCreation_Implementation(FName TabId, UCommonButtonBase* TabButton) override;

	/** 있으면 생성된 탭 버튼을 이 패널에 자동으로 붙인다. */
	UPROPERTY(BlueprintReadOnly, Category = "Tab List", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TabButtonContainer;

	/** 있으면 NativeOnInitialized에서 이 스위처를 자동으로 링크한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Tab List", meta = (BindWidgetOptional))
	TObjectPtr<UCommonAnimatedSwitcher> TabContentSwitcher;

private:
	void SetupTabs();

	UPROPERTY(EditAnywhere, meta = (TitleProperty = "TabId"))
	TArray<FWxTabDescriptor> PreregisteredTabInfoArray;

	/** 런타임에 등록됐지만 아직 생성되지 않은 탭의 라벨 정보를 보관한다. 탭이 생성되면 제거된다. */
	UPROPERTY()
	TMap<FName, FWxTabDescriptor> PendingTabLabelInfoMap;
};
