// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxGameFlowSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "System/WxCheckpointSubsystem.h"

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
		StatusText = LOCTEXT("StandaloneOnly", "새 게임 진입은 싱글플레이에서 사용할 수 있습니다.");
		FrontEndChanged.Broadcast();
		return false;
	}
	if (PawnClass.IsNull() || Level.IsNull()
		|| !FPackageName::DoesPackageExist(Level.ToSoftObjectPath().GetLongPackageName())
		|| IsWorldPackage(World, Level))
	{
		StatusText = LOCTEXT("InvalidSelection", "캐릭터 또는 레벨을 확인해주세요.");
		FrontEndChanged.Broadcast();
		return false;
	}
	PendingPawnClass = PawnClass;
	PendingLevel = Level;
	StatusText = FText::GetEmpty();
	GetGameInstance()->GetSubsystem<UWxCheckpointSubsystem>()->ResetCheckpoint();
	FrontEndChanged.Broadcast();
	UGameplayStatics::OpenLevel(this, FName(*Level.ToSoftObjectPath().GetLongPackageName()), true);
	return true;
}

bool UWxGameFlowSubsystem::IsBusy() const
{
	return !PendingLevel.IsNull() && !IsDestinationWorld(GetWorld());
}

const FText& UWxGameFlowSubsystem::GetStatusText() const
{
	return StatusText;
}

UClass* UWxGameFlowSubsystem::GetSelectedPawnClass(const UWorld* World) const
{
	return IsDestinationWorld(World) ? PendingPawnClass.LoadSynchronous() : nullptr;
}

void UWxGameFlowSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || PendingLevel.IsNull())
	{
		return;
	}
	if (!IsDestinationWorld(World))
	{
		// 전환이 어긋났든 이후의 일반 이동이든, 목적지가 아닌 맵이 열리면 선택은 여기서 끝난다.
		PendingPawnClass.Reset();
		PendingLevel.Reset();
		FrontEndChanged.Broadcast();
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
	FrontEndChanged.Broadcast();
}

void UWxGameFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || !IsBusy())
	{
		return;
	}
	// 출발 맵에 그대로 있으므로 선택만 버리면 메뉴가 문구를 띄우고 버튼을 다시 연다.
	PendingPawnClass.Reset();
	PendingLevel.Reset();
	StatusText = FText::Format(LOCTEXT("TravelFailure", "레벨 전환에 실패했습니다: {0}"), FText::FromString(Error));
	FrontEndChanged.Broadcast();
}

UWxGameFlowSubsystem::FWxFrontEndChanged& UWxGameFlowSubsystem::OnFrontEndChanged()
{
	return FrontEndChanged;
}

void UWxGameFlowSubsystem::ResetFrontEnd()
{
	SelectedCharacter = FWxFrontEndOption();
	SelectedDestination = FWxFrontEndOption();
	FrontEndStep = EWxFrontEndStep::Main;
	FrontEndChanged.Broadcast();
}

void UWxGameFlowSubsystem::BeginCharacterSelection()
{
	if (FrontEndStep != EWxFrontEndStep::Main || IsFrontEndInputBlocked())
	{
		return;
	}
	SelectedCharacter = FWxFrontEndOption();
	SelectedDestination = FWxFrontEndOption();
	FrontEndStep = EWxFrontEndStep::Character;
	FrontEndChanged.Broadcast();
}

bool UWxGameFlowSubsystem::SelectCharacter(const FWxFrontEndOption& Option)
{
	if (FrontEndStep != EWxFrontEndStep::Character || IsFrontEndInputBlocked() || Option.PawnClass.IsNull())
	{
		return false;
	}
	SelectedCharacter = Option;
	FrontEndStep = EWxFrontEndStep::Destination;
	FrontEndChanged.Broadcast();
	return true;
}

bool UWxGameFlowSubsystem::SelectDestination(const FWxFrontEndOption& Option)
{
	if (FrontEndStep != EWxFrontEndStep::Destination || IsFrontEndInputBlocked() || Option.Level.IsNull())
	{
		return false;
	}
	SelectedDestination = Option;
	FrontEndStep = EWxFrontEndStep::Confirmation;
	FrontEndChanged.Broadcast();
	return true;
}

void UWxGameFlowSubsystem::ResolveStartConfirmation(bool bConfirmed)
{
	if (FrontEndStep != EWxFrontEndStep::Confirmation)
	{
		return;
	}
	if (!bConfirmed)
	{
		ResetFrontEnd();
		return;
	}
	FrontEndStep = EWxFrontEndStep::Destination;
	RequestNewGame(SelectedCharacter.PawnClass, SelectedDestination.Level);
	FrontEndChanged.Broadcast();
}

EWxFrontEndStep UWxGameFlowSubsystem::GetFrontEndStep() const
{
	return FrontEndStep;
}

bool UWxGameFlowSubsystem::IsFrontEndInputBlocked() const
{
	return IsBusy() || FrontEndStep == EWxFrontEndStep::Confirmation;
}

FText UWxGameFlowSubsystem::GetStartConfirmationText() const
{
	return FText::Format(LOCTEXT("StartConfirmation", "Character: {0}\nLevel: {1}\n\nStart the game with this selection?"),
		SelectedCharacter.Title, SelectedDestination.Title);
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
