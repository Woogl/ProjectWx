// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Texture2D.h"
#include "Curves/CurveFloat.h"
#include "Misc/AutomationTest.h"
#include "MVVM/WxViewModel_Item.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

namespace WxItemViewModelTests
{
	struct FWxNotifications
	{
		void HandleChanged(UObject* Object, UE::FieldNotification::FFieldId Field);
		int32 Count = 0;
	};

	void FWxNotifications::HandleChanged(UObject* Object, UE::FieldNotification::FFieldId Field)
	{
		++Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxItemViewModelObjectTest, "Wx.MVVM.Item.ObjectLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxItemViewModelObjectTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWxViewModel_Item> ViewModel(NewObject<UWxViewModel_Item>());
	UObject* Source = NewObject<UCurveFloat>();
	TWeakObjectPtr<UObject> WeakSource(Source);
	TStrongObjectPtr<UTexture2D> Icon(NewObject<UTexture2D>());
	ViewModel->Initialize(Source, FText::FromString(TEXT("Object")), Icon.Get());
	CollectGarbage(RF_NoFlags);
	TestTrue(TEXT("ViewModel retains arbitrary source"), WeakSource.IsValid());
	TestEqual(TEXT("No interface required"), ViewModel->SourceObject.Get(), static_cast<const UObject*>(WeakSource.Get()));
	TestEqual(TEXT("Explicit display name"), ViewModel->DisplayName.ToString(), FString(TEXT("Object")));
	TestEqual(TEXT("Loaded image exposed"), ViewModel->Icon.Get(), static_cast<UObject*>(Icon.Get()));

	WxItemViewModelTests::FWxNotifications Notifications;
	const FDelegateHandle Handle = ViewModel->AddFieldValueChangedDelegate(UWxViewModel_Item::FFieldNotificationClassDescriptor::DisplayName,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateRaw(&Notifications, &WxItemViewModelTests::FWxNotifications::HandleChanged));
	ViewModel->SetDisplayName(FText::FromString(TEXT("Renamed")));
	TestEqual(TEXT("Display change notifies"), Notifications.Count, 1);
	ViewModel->Initialize(Icon.Get(), ViewModel->DisplayName, TSoftObjectPtr<UObject>());
	TestEqual(TEXT("Reinitialization preserves aliased display name"), ViewModel->DisplayName.ToString(), FString(TEXT("Renamed")));
	TestEqual(TEXT("Different UObject subclass accepted"), ViewModel->SourceObject.Get(), static_cast<const UObject*>(Icon.Get()));
	TestNull(TEXT("Replacing source clears previous image"), ViewModel->Icon.Get());
	CollectGarbage(RF_NoFlags);
	TestFalse(TEXT("Replacing source releases previous object"), WeakSource.IsValid());

	const int32 PreviousNotifications = Notifications.Count;
	ViewModel->Deinitialize();
	TestEqual(TEXT("Release notifies display binding"), Notifications.Count, PreviousNotifications + 1);
	TestNull(TEXT("Release clears source"), ViewModel->SourceObject.Get());
	TestTrue(TEXT("Release clears name"), ViewModel->DisplayName.IsEmpty());
	TestNull(TEXT("Release clears icon"), ViewModel->Icon.Get());
	ViewModel->Deinitialize();
	TestEqual(TEXT("Repeated release is idempotent"), Notifications.Count, PreviousNotifications + 1);
	ViewModel->Initialize(nullptr, FText::GetEmpty(), TSoftObjectPtr<UObject>());
	TestNull(TEXT("Null source supported"), ViewModel->SourceObject.Get());
	ViewModel->RemoveFieldValueChangedDelegate(UWxViewModel_Item::FFieldNotificationClassDescriptor::DisplayName, Handle);
	return true;
}

#endif
