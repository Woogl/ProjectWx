// Copyright Woogle. All Rights Reserved.


#include "Widget/WxTabListWidgetBase.h"

#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/PanelWidget.h"

void UWxTabListWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 바인딩된 스위처가 있으면 자동 링크한다(외부 배선/이벤트 그래프 불필요).
	if (TabContentSwitcher && GetLinkedSwitcher() == nullptr)
	{
		SetLinkedSwitcher(TabContentSwitcher);
	}
}

void UWxTabListWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	SetupTabs();
}

void UWxTabListWidgetBase::NativeDestruct()
{
	for (FWxTabDescriptor& TabInfo : PreregisteredTabInfoArray)
	{
		if (TabInfo.CreatedTabContentWidget)
		{
			TabInfo.CreatedTabContentWidget->RemoveFromParent();
			TabInfo.CreatedTabContentWidget = nullptr;
		}
	}

	Super::NativeDestruct();
}

bool UWxTabListWidgetBase::GetPreregisteredTabInfo(const FName TabNameId, FWxTabDescriptor& OutTabInfo)
{
	const FWxTabDescriptor* const FoundTabInfo = PreregisteredTabInfoArray.FindByPredicate([&](FWxTabDescriptor& TabInfo) -> bool
	{
		return TabInfo.TabId == TabNameId;
	});

	if (!FoundTabInfo)
	{
		return false;
	}

	OutTabInfo = *FoundTabInfo;
	return true;
}

const TArray<FWxTabDescriptor>& UWxTabListWidgetBase::GetAllPreregisteredTabInfos()
{
	return PreregisteredTabInfoArray;
}

void UWxTabListWidgetBase::SetTabHiddenState(FName TabNameId, bool bHidden)
{
	for (FWxTabDescriptor& TabInfo : PreregisteredTabInfoArray)
	{
		if (TabInfo.TabId == TabNameId)
		{
			TabInfo.bHidden = bHidden;
			break;
		}
	}
}

bool UWxTabListWidgetBase::RegisterDynamicTab(const FWxTabDescriptor& TabDescriptor)
{
	// 숨김 탭이면 그냥 무시한다.
	if (TabDescriptor.bHidden)
	{
		return true;
	}

	PendingTabLabelInfoMap.Add(TabDescriptor.TabId, TabDescriptor);

	return RegisterTab(TabDescriptor.TabId, TabDescriptor.TabButtonType, TabDescriptor.CreatedTabContentWidget);
}

void UWxTabListWidgetBase::HandlePreLinkedSwitcherChanged()
{
	for (const FWxTabDescriptor& TabInfo : PreregisteredTabInfoArray)
	{
		// 연결이 해제되는 스위처에서 탭 콘텐츠 위젯을 떼어낸다.
		if (TabInfo.CreatedTabContentWidget)
		{
			TabInfo.CreatedTabContentWidget->RemoveFromParent();
		}
	}

	Super::HandlePreLinkedSwitcherChanged();
}

void UWxTabListWidgetBase::HandlePostLinkedSwitcherChanged()
{
	if (!IsDesignTime() && GetCachedWidget().IsValid())
	{
		// 디자이너거나 아직 생성 전이면 탭을 만들지 않는다.
		SetupTabs();
	}

	Super::HandlePostLinkedSwitcherChanged();
}

void UWxTabListWidgetBase::HandleTabCreation_Implementation(FName TabId, UCommonButtonBase* TabButton)
{
	if (!TabButton)
	{
		return;
	}

	FWxTabDescriptor* TabInfoPtr = nullptr;

	FWxTabDescriptor TabInfo;
	if (GetPreregisteredTabInfo(TabId, TabInfo))
	{
		TabInfoPtr = &TabInfo;
	}
	else
	{
		TabInfoPtr = PendingTabLabelInfoMap.Find(TabId);
	}

	if (TabButton->GetClass()->ImplementsInterface(UWxTabButtonInterface::StaticClass()))
	{
		if (ensureMsgf(TabInfoPtr, TEXT("A tab button was created with id %s but no label info was specified. RegisterDynamicTab should be used over RegisterTab to provide label info."), *TabId.ToString()))
		{
			IWxTabButtonInterface::Execute_SetTabLabelInfo(TabButton, *TabInfoPtr);
		}
	}

	// 바인딩된 컨테이너가 있으면 생성된 탭 버튼을 붙인다(WBP 이벤트 그래프 불필요).
	if (TabButtonContainer)
	{
		TabButtonContainer->AddChild(TabButton);
	}

	PendingTabLabelInfoMap.Remove(TabId);
}

bool UWxTabListWidgetBase::IsFirstTabActive() const
{
	if (PreregisteredTabInfoArray.Num() > 0)
	{
		return GetActiveTab() == PreregisteredTabInfoArray[0].TabId;
	}

	return false;
}

bool UWxTabListWidgetBase::IsLastTabActive() const
{
	if (PreregisteredTabInfoArray.Num() > 0)
	{
		return GetActiveTab() == PreregisteredTabInfoArray.Last().TabId;
	}

	return false;
}

bool UWxTabListWidgetBase::IsTabVisible(FName TabId)
{
	if (const UCommonButtonBase* Button = GetTabButtonBaseByID(TabId))
	{
		const ESlateVisibility TabVisibility = Button->GetVisibility();
		return (TabVisibility == ESlateVisibility::Visible
			|| TabVisibility == ESlateVisibility::HitTestInvisible
			|| TabVisibility == ESlateVisibility::SelfHitTestInvisible);
	}

	return false;
}

int32 UWxTabListWidgetBase::GetVisibleTabCount()
{
	int32 Result = 0;
	const int32 TabCount = GetTabCount();
	for (int32 Index = 0; Index < TabCount; Index++)
	{
		if (IsTabVisible(GetTabIdAtIndex(Index)))
		{
			Result++;
		}
	}

	return Result;
}

void UWxTabListWidgetBase::SetupTabs()
{
	for (FWxTabDescriptor& TabInfo : PreregisteredTabInfoArray)
	{
		if (TabInfo.bHidden)
		{
			continue;
		}

		// 탭 콘텐츠가 아직 없으면 생성한다.
		if (!TabInfo.CreatedTabContentWidget && TabInfo.TabContentType)
		{
			TabInfo.CreatedTabContentWidget = CreateWidget<UCommonUserWidget>(GetOwningPlayer(), TabInfo.TabContentType);
			OnTabContentCreatedNative.Broadcast(TabInfo.TabId, Cast<UCommonUserWidget>(TabInfo.CreatedTabContentWidget));
			OnTabContentCreated.Broadcast(TabInfo.TabId, Cast<UCommonUserWidget>(TabInfo.CreatedTabContentWidget));
		}

		if (UCommonAnimatedSwitcher* CurrentLinkedSwitcher = GetLinkedSwitcher())
		{
			// 새로 연결된 스위처에 탭 콘텐츠를 추가한다.
			if (!CurrentLinkedSwitcher->HasChild(TabInfo.CreatedTabContentWidget))
			{
				CurrentLinkedSwitcher->AddChild(TabInfo.CreatedTabContentWidget);
			}
		}

		// 아직 등록되지 않은 탭이면 등록한다.
		if (GetTabButtonBaseByID(TabInfo.TabId) == nullptr)
		{
			RegisterTab(TabInfo.TabId, TabInfo.TabButtonType, TabInfo.CreatedTabContentWidget);
		}
	}
}
