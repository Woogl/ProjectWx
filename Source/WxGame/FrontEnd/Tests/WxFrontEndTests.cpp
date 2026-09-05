// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "FrontEnd/WxFrontEndLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxFrontEndContextTest, "Wx.FrontEnd.InvalidContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxFrontEndContextTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Missing world rejects entry"), UWxFrontEndLibrary::RequestNewGame(nullptr, {}, {}));
	bool bBusy = false;
	FText Message;
	UWxFrontEndLibrary::GetTravelStatus(nullptr, bBusy, Message);
	TestTrue(TEXT("Unavailable game flow disables entry"), bBusy);
	return true;
}

#endif
