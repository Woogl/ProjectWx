// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Device/WxDevice.h"
#include "BrainComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FrontEnd/WxGameFlowSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "System/WxCheckpointSaveGame.h"
#include "UObject/Package.h"
#include "WxInteractable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxCheckpointFailureTest, "Wx.Checkpoint.FailurePaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxCheckpointFailureTest::RunTest(const FString& Parameters)
{
	const FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	if (FPaths::IsSamePath(SavedDir, FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Saved/"))))
	{
		AddError(TEXT("Run with -UserDir so the real checkpoint slot is untouched."));
		return false;
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString SlotPath = SavedDir / TEXT("SaveGames/WxCheckpoint.sav");
	const auto ClearSlot = [&]()
	{
		PlatformFile.SetReadOnly(*SlotPath, false);
		IFileManager::Get().Delete(*SlotPath, false, true, true);
	};
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	const FTransform First(FRotator(0, 45, 0), FVector(100, 200, 300));
	const FTransform Second(FRotator(0, 90, 0), FVector(400, 500, 600));
	FTransform Result;

	// 조회 불가: 슬롯 없음·다른 레벨·손상된 슬롯은 모두 체크포인트 없음(PlayerStart 부활)으로 판정한다.
	ClearSlot();
	TestFalse(TEXT("No slot -> no checkpoint"), UWxCheckpointSaveGame::TryGetCheckpoint(World, Result));
	TestTrue(TEXT("Save succeeds"), UWxCheckpointSaveGame::SaveCheckpoint(World, First));
	TestTrue(TEXT("Same level reads checkpoint"), UWxCheckpointSaveGame::TryGetCheckpoint(World, Result) && Result.Equals(First));
	TestFalse(TEXT("Other level -> no checkpoint"), UWxCheckpointSaveGame::TryGetCheckpoint(OtherWorld, Result));

	// 저장·삭제 실패: 읽기 전용 슬롯은 덮어쓰지도 지우지도 못한다.
	ClearSlot();
	TestTrue(TEXT("Save before lock"), UWxCheckpointSaveGame::SaveCheckpoint(World, First));
	PlatformFile.SetReadOnly(*SlotPath, true);
	TestFalse(TEXT("Save fails on read-only slot"), UWxCheckpointSaveGame::SaveCheckpoint(World, Second));
	TestTrue(TEXT("Failed save keeps previous checkpoint"), UWxCheckpointSaveGame::TryGetCheckpoint(World, Result) && Result.Equals(First));
	TestFalse(TEXT("Reset fails on read-only slot"), UWxCheckpointSaveGame::ResetCheckpoint(World));
	TestTrue(TEXT("Failed reset keeps slot"), PlatformFile.FileExists(*SlotPath));

	// 새 게임: 슬롯 삭제에 실패하면 이동하지 않고 안내 문구를 남긴다.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* FlowWorld = GameInstance->GetWorld();
	UWxGameFlowSubsystem* Flow = GameInstance->GetSubsystem<UWxGameFlowSubsystem>();
	if (TestNotNull(TEXT("Game flow subsystem"), Flow))
	{
		const TSoftObjectPtr<UWorld> Level(FSoftObjectPath(TEXT("/Game/Maps/LV_DevCombat.LV_DevCombat")));
		TestFalse(TEXT("New game aborts when reset fails"), Flow->RequestNewGame(TSoftClassPtr<APawn>(ACharacter::StaticClass()), Level));
		TestTrue(TEXT("Reset failure message"), Flow->GetStatusText().ToString().Contains(TEXT("초기화에 실패")));
		TestFalse(TEXT("No travel pending"), Flow->IsBusy());
		TestTrue(TEXT("Slot kept after aborted new game"), PlatformFile.FileExists(*SlotPath));
	}
	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(FlowWorld);
	FlowWorld->DestroyWorld(false);

	// 저장 태스크: 실제 BP_CheckPoint·ST_CheckPoint로 상호작용해 저장 실패 시 Failed 분기를 탄다.
	UClass* CheckpointClass = LoadClass<AWxDevice>(nullptr, TEXT("/Game/WorldObject/Gimmick/BP_CheckPoint.BP_CheckPoint_C"));
	if (TestNotNull(TEXT("BP_CheckPoint loads"), CheckpointClass))
	{
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			const bool bLocked = Pass == 1;
			ClearSlot();
			if (bLocked)
			{
				UWxCheckpointSaveGame::SaveCheckpoint(World, First);
				PlatformFile.SetReadOnly(*SlotPath, true);
				AddExpectedMessage(TEXT("체크포인트 저장에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
			}
			APlayerController* Controller = World->SpawnActor<APlayerController>();
			ACharacter* Character = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
			Controller->Possess(Character);
			AWxDevice* Device = World->SpawnActor<AWxDevice>(CheckpointClass, FTransform(FVector(1000, 0, 0)));
			UBrainComponent* Component = Device ? Device->FindComponentByClass<UBrainComponent>() : nullptr;
			if (!TestNotNull(TEXT("Checkpoint state tree component"), Component))
			{
				break;
			}
			if (!Component->IsRunning())
			{
				Component->StartLogic();
			}
			const auto Tick = [&]()
			{
				for (int32 Step = 0; Step < 10; ++Step)
				{
					Component->TickComponent(0.1f, LEVELTICK_All, &Component->PrimaryComponentTick);
				}
			};
			Tick();
			TArray<FWxInteractionOption> Options;
			Device->GetInteractionOptions(Character, Options);
			if (!TestTrue(TEXT("Checkpoint offers interaction"), Options.Num() > 0))
			{
				break;
			}
			Device->OnInteracted(Character, Options[0].Value);
			Tick();
			const bool bHasCheckpoint = UWxCheckpointSaveGame::TryGetCheckpoint(World, Result);
			if (bLocked)
			{
				TestTrue(TEXT("Locked slot keeps previous checkpoint"), bHasCheckpoint && Result.Equals(First));
			}
			else
			{
				TestTrue(TEXT("Interaction saves checkpoint"), bHasCheckpoint && !Result.Equals(First));
			}
			Component->StopLogic(TEXT("Test cleanup"));
		}
	}
	ClearSlot();
	World->DestroyWorld(false);
	OtherWorld->DestroyWorld(false);
	return true;
}

// 손상 슬롯은 종류마다 따로 돈다(엔진이 크래시하면 그 프로세스가 끝나므로).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxCheckpointCorruptSlotTest, "Wx.Checkpoint.CorruptSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxCheckpointCorruptSlotTest::RunTest(const FString& Parameters)
{
	FString Mode;
	FParse::Value(FCommandLine::Get(), TEXT("WxCorrupt="), Mode);
	const FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	if (FPaths::IsSamePath(SavedDir, FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Saved/"))))
	{
		AddError(TEXT("Run with -UserDir so the real checkpoint slot is untouched."));
		return false;
	}
	const FString SlotPath = SavedDir / TEXT("SaveGames/WxCheckpoint.sav");
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	IFileManager::Get().Delete(*SlotPath, false, true, true);
	TestTrue(TEXT("Valid save"), UWxCheckpointSaveGame::SaveCheckpoint(World, FTransform(FVector(1, 2, 3))));
	TArray<uint8> Bytes;
	FFileHelper::LoadFileToArray(Bytes, *SlotPath);
	AddInfo(FString::Printf(TEXT("Mode=%s, valid size=%d"), *Mode, Bytes.Num()));
	if (Mode == TEXT("Empty"))
	{
		Bytes.Reset();
	}
	else if (Mode == TEXT("Truncated"))
	{
		Bytes.SetNum(Bytes.Num() / 2);
	}
	else if (Mode == TEXT("Garbage"))
	{
		const FTCHARToUTF8 Text(TEXT("not a save game"));
		Bytes = TArray<uint8>(reinterpret_cast<const uint8*>(Text.Get()), Text.Length());
	}
	FFileHelper::SaveArrayToFile(Bytes, *SlotPath);
	FTransform Result;
	TestFalse(TEXT("Corrupt slot -> no checkpoint"), UWxCheckpointSaveGame::TryGetCheckpoint(World, Result));
	IFileManager::Get().Delete(*SlotPath, false, true, true);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxSpawnerAssetLoadTest, "Wx.Spawner.AssetLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxSpawnerAssetLoadTest::RunTest(const FString& Parameters)
{
	const TCHAR* Packages[] =
	{
		TEXT("/Game/WorldObject/Gimmick/ST_CheckPoint"),
		TEXT("/Game/WorldObject/Gimmick/BP_CheckPoint"),
		TEXT("/Game/Quest/Steps/ST_QuestStep_KillEnemies"),
		TEXT("/Game/Quest/ST_Quest_Main1"),
		TEXT("/Game/Quest/ST_Quest_Main2"),
		TEXT("/Game/__ExternalActors__/LevelDesign/LevelInstance/SiegeCannonEmplacement01/A/64/E5RT854NODVWD6343VJSF4"),
		TEXT("/Game/__ExternalActors__/LevelDesign/LevelInstance/SiegeCannonEmplacement01/A/GG/Y29X1QC157Q99ECP07Z05E"),
		TEXT("/Game/__ExternalActors__/LevelDesign/LevelInstance/SiegeCannonEmplacement01/C/YW/LUIO7D8I5LMS0AZ6TEKEA0"),
		TEXT("/Game/__ExternalActors__/LevelDesign/LevelInstance/SiegeCannonEmplacement01/E/VD/QMLAAG4Q12F9W3KM8GHHFV"),
		TEXT("/Game/__ExternalActors__/Maps/LV_DevCombat/0/H3/DCHT7MMMNNWXRDDUFVXTTO"),
		TEXT("/Game/__ExternalActors__/Maps/LV_DevCombat/2/2J/UA5O5O0OI5TF60G5ITO1EA"),
		TEXT("/Game/__ExternalActors__/Maps/LV_DevCombat/5/KC/J6DVE7DNQLN7FLLA9KCEF3"),
		TEXT("/Game/__ExternalActors__/Maps/LV_DevCombat/6/I3/8DMT31F8PIQ50TDMA0701R"),
		TEXT("/Game/__ExternalActors__/Maps/LV_DevCombat/C/NZ/9TD3LG9ZMG6AZNF70Z5B9R"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/3/9Z/X3JM0HK4CNVCEMLDUAQZGL"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/4/WD/DO20GLFT73YO7QI7GSTFRB"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/5/SD/4CXL0KJHA2BJIC6M4FV4IG"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/5/YR/MWSE2WV8C1J0EAPENOEU6J"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/6/6J/E739HF09XZVERPTLGOXTLC"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/6/D3/VY68JVTWARCMH96IB3W6PS"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/8/MP/2X2L249SMQSC8KSBTKX9V2"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/8/PN/XKBC9CLMB4H07UQUMIY0A9"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/8/S7/L3N9WJ1DB1LPEAIQNNXETT"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/A/DE/UNNMRUGNT7R6BWV349LXJG"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/D/E9/WDUZG6ELBZFFQ3860XIEAF"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/D/F3/GQ0JCZX3A0NXZH3KNV39D2"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/D/LO/XYNGUBX63O19GKBEOVLL97"),
		TEXT("/Game/__ExternalActors__/Maps/LV_OpenWorld/F/1I/YITNY81XA4LSVESO73RW2T"),
	};
	for (const TCHAR* Package : Packages)
	{
		TestNotNull(FString::Printf(TEXT("Loads %s"), Package), LoadPackage(nullptr, Package, LOAD_None));
	}
	return true;
}

#endif
