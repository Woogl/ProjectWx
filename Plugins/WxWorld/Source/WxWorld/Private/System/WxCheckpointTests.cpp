// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "System/WxCheckpointSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxCheckpointLifetimeTest, "Wx.Checkpoint.MapRestartAndNewGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxCheckpointLifetimeTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UWxCheckpointSubsystem* Checkpoint = NewObject<UWxCheckpointSubsystem>(GameInstance);
	UPackage* MapPackage = CreatePackage(TEXT("/Temp/WxCheckpointMap"));
	UWorld* OriginalWorld = NewObject<UWorld>(MapPackage);
	UWorld* ReloadedWorld = NewObject<UWorld>(MapPackage);
	UWorld* OtherWorld = NewObject<UWorld>(CreatePackage(TEXT("/Temp/WxCheckpointOtherMap")));
	FTransform Result;
	TestFalse(TEXT("No checkpoint uses normal player start"), Checkpoint->TryGetCheckpoint(OriginalWorld, Result));
	const FTransform First(FRotator(0, 45, 0), FVector(100, 200, 100));
	const FTransform Last(FRotator(0, 120, 0), FVector(900, 700, 100));
	Checkpoint->RecordCheckpoint(OriginalWorld, First);
	Checkpoint->RecordCheckpoint(OriginalWorld, Last);
	for (int32 Restart = 0; Restart < 2; ++Restart)
	{
		TestTrue(TEXT("Reloaded world retains checkpoint on repeated restarts"), Checkpoint->TryGetCheckpoint(ReloadedWorld, Result));
		TestTrue(TEXT("Latest interaction determines position and direction"), Result.Equals(Last));
	}
	TestFalse(TEXT("Other maps do not inherit checkpoint"), Checkpoint->TryGetCheckpoint(OtherWorld, Result));
	Checkpoint->RecordCheckpoint(ReloadedWorld, First);
	TestTrue(TEXT("Returning to an older checkpoint updates last interaction"), Checkpoint->TryGetCheckpoint(ReloadedWorld, Result));
	TestTrue(TEXT("Resting again replaces previous checkpoint"), Result.Equals(First));
	Checkpoint->ResetCheckpoint();
	TestFalse(TEXT("New game clears checkpoint even in the same map"), Checkpoint->TryGetCheckpoint(ReloadedWorld, Result));
	return true;
}

#endif
