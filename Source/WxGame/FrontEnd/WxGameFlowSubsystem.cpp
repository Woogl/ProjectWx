// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxGameFlowSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FrontEnd/WxFrontEndDeveloperSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Save/WxCheckpointSaveGame.h"

#define LOCTEXT_NAMESPACE "WxGameFlow"

void UWxGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
}

void UWxGameFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	GEngine->OnTravelFailure().RemoveAll(this);
	Super::Deinitialize();
}

bool UWxGameFlowSubsystem::RequestNewGame(TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level)
{
	if (IsBusy())
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		StatusText = LOCTEXT("StandaloneOnly", "Starting a new game is only available in single player.");
		return false;
	}
	if (PawnClass.IsNull() || Level.IsNull()
		|| !FPackageName::DoesPackageExist(Level.ToSoftObjectPath().GetLongPackageName())
		|| IsWorldPackage(World, Level))
	{
		StatusText = LOCTEXT("InvalidSelection", "Check the selected character and level.");
		return false;
	}
	UClass* SelectedPawnClass = PawnClass.LoadSynchronous();
	if (!SelectedPawnClass || !SelectedPawnClass->IsChildOf(APawn::StaticClass())
		|| SelectedPawnClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		StatusText = LOCTEXT("InvalidPawnClass", "The selected character cannot be spawned. Choose another character.");
		return false;
	}
	if (!UWxCheckpointSaveGame::ResetCheckpoint(World))
	{
		StatusText = LOCTEXT("CheckpointResetFailed", "Failed to reset the checkpoint save. Please try again.");
		return false;
	}
	// 검증한 클래스를 맵 이동 중에도 유지해 목적지에서 다시 로드하거나 기본 Pawn으로 대체하지 않는다.
	PendingPawnClass = SelectedPawnClass;
	PendingLevel = Level;
	StatusText = FText::GetEmpty();
	UGameplayStatics::OpenLevel(this, FName(*Level.ToSoftObjectPath().GetLongPackageName()), true);
	return true;
}

bool UWxGameFlowSubsystem::IsBusy() const
{
	return !PendingLevel.IsNull() && !IsDestinationWorld(GetWorld());
}

const FText& UWxGameFlowSubsystem::GetStatusText() const
{
	if (StatusText.IsEmpty() && !GetDefault<UWxFrontEndDeveloperSettings>()->HasSelectableOptions())
	{
		static const FText NoOptions = LOCTEXT("NoOptions", "No selectable characters or levels.");
		return NoOptions;
	}
	return StatusText;
}

UClass* UWxGameFlowSubsystem::GetSelectedPawnClass(const UWorld* World) const
{
	return IsDestinationWorld(World) ? PendingPawnClass.Get() : nullptr;
}

void UWxGameFlowSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || PendingLevel.IsNull())
	{
		return;
	}
	if (!IsDestinationWorld(World))
	{
		PendingPawnClass = nullptr;
		PendingLevel.Reset();
		return;
	}
	// 폰은 스폰됐지만 아직 한 틱도 돌지 않았다. 지금 지형을 올려 두면 중력으로 떨어지지 않는다.
	APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController();
	if (Controller && Controller->PlayerCameraManager)
	{
		// 월드파티션이 볼 스트리밍 소스 위치를 폰 시점으로 확정한다.
		Controller->PlayerCameraManager->UpdateCamera(0.f);
	}
	World->BlockTillLevelStreamingCompleted();
}

void UWxGameFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	// 엔진이 기본 맵(프론트엔드)으로 되돌린다. 맵 로드 단계 실패는 폴백 맵 로드가 이 방송보다 먼저 와 선택을 이미 비우므로 진행 중인지 묻지 않는다.
	// 문구는 GameInstance 수명이라 다시 뜬 메뉴가 읽는다.
	PendingPawnClass = nullptr;
	PendingLevel.Reset();
	StatusText = FText::Format(LOCTEXT("TravelFailure", "Level travel failed: {0}"), FText::FromString(Error));
}

bool UWxGameFlowSubsystem::IsDestinationWorld(const UWorld* World) const
{
	return !PendingLevel.IsNull() && IsWorldPackage(World, PendingLevel);
}

bool UWxGameFlowSubsystem::IsWorldPackage(const UWorld* World, const TSoftObjectPtr<UWorld>& Map) const
{
	return World && UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == Map.ToSoftObjectPath().GetLongPackageName();
}

#undef LOCTEXT_NAMESPACE
