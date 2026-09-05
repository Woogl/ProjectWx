// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxGameFlowSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Framework/WxExperienceDefinition.h"
#include "Framework/WxExperienceManagerComponent.h"
#include "Framework/WxGameState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIManagerSubsystem.h"
#include "WxGameplayTags.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

#define LOCTEXT_NAMESPACE "WxGameFlow"

namespace
{
	constexpr double TravelTimeoutSeconds = 120.0;
}

void UWxGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UWxUIManagerSubsystem>();
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::HandleTick));
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
}

void UWxGameFlowSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	GEngine->OnTravelFailure().RemoveAll(this);
	ClearPending();
	Super::Deinitialize();
}

bool UWxGameFlowSubsystem::RequestNewGame(TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level)
{
	if (IsBusy())
	{
		return false;
	}
	if (!GetWorld() || !GetWorld()->IsNetMode(NM_Standalone))
	{
		StatusText = LOCTEXT("StandaloneOnly", "새 게임 진입은 싱글플레이에서 사용할 수 있습니다.");
		return false;
	}
	ClearPending();
	if (PawnClass.IsNull() || Level.IsNull()
		|| !FPackageName::DoesPackageExist(Level.ToSoftObjectPath().GetLongPackageName())
		|| IsWorldPackage(GetWorld(), Level))
	{
		Fail(LOCTEXT("InvalidSelection", "캐릭터 또는 레벨을 확인해주세요."));
		return false;
	}
	PendingPawnClass = PawnClass;
	PendingLevel = Level;
	ReturnLevelPackage = FName(*UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()));
	ActiveRequestId = FGuid::NewGuid();
	State = EWxTravelState::Preparing;
	Deadline = FPlatformTime::Seconds() + TravelTimeoutSeconds;
	StatusText = FText::GetEmpty();
	TArray<FSoftObjectPath> Paths;
	Paths.Add(PendingPawnClass.ToSoftObjectPath());
	AssetHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(Paths,
		FStreamableDelegate::CreateUObject(this, &ThisClass::HandleAssetsLoaded, ActiveRequestId));
	if (!AssetHandle)
	{
		Fail(LOCTEXT("LoadRequestFailed", "입장 데이터 로드를 시작하지 못했습니다."));
		return false;
	}
	return true;
}

void UWxGameFlowSubsystem::HandleAssetsLoaded(FGuid RequestId)
{
	if (State != EWxTravelState::Preparing || RequestId != ActiveRequestId)
	{
		return;
	}
	SelectedPawnClass = PendingPawnClass.Get();
	if (!SelectedPawnClass || !SelectedPawnClass->IsChildOf(APawn::StaticClass())
		|| SelectedPawnClass->HasAnyClassFlags(CLASS_Abstract))
	{
		Fail(LOCTEXT("CharacterUnavailable", "선택한 캐릭터를 불러올 수 없습니다."));
		return;
	}
	State = EWxTravelState::Traveling;
	UGameplayStatics::OpenLevel(this, FName(*PendingLevel.ToSoftObjectPath().GetLongPackageName()), true);
}

void UWxGameFlowSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	if (State == EWxTravelState::Traveling)
	{
		if (!IsDestinationWorld(World))
		{
			Fail(LOCTEXT("WrongWorld", "입장한 레벨이 요청한 목적지와 다릅니다."));
			return;
		}
		State = EWxTravelState::AwaitingReady;
	}
}

void UWxGameFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (World && World->GetGameInstance() == GetGameInstance() && IsBusy())
	{
		Fail(FText::Format(LOCTEXT("TravelFailure", "레벨 전환에 실패했습니다: {0}"), FText::FromString(Error)));
	}
}

bool UWxGameFlowSubsystem::HandleTick(float DeltaSeconds)
{
	if (bRecoveryRequired)
	{
		bRecoveryRequired = false;
		BeginRecovery();
		return true;
	}
	if (!IsBusy())
	{
		return true;
	}
	UWorld* World = GetWorld();
	if (State == EWxTravelState::AwaitingReady || State == EWxTravelState::Recovering)
	{
		const AWxGameState* GameState = World ? World->GetGameState<AWxGameState>() : nullptr;
		const UWxExperienceManagerComponent* Experience = GameState ? GameState->GetExperienceManagerComponent() : nullptr;
		if (Experience && Experience->HasLoadFailed())
		{
			Fail(LOCTEXT("ExperienceFailed", "게임 구성을 불러오지 못했습니다."));
			return true;
		}
		APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
		const UWxUIManagerSubsystem* UI = GetGameInstance()->GetSubsystem<UWxUIManagerSubsystem>();
		const UWxPrimaryGameLayout* Layout = UI ? UI->GetPrimaryGameLayout() : nullptr;
		const UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game) : nullptr;
		const bool bHUDReady = UI && (UI->GetGameHUDClass().IsNull() || (Stack && Stack->GetNumWidgets() > 0));
		UWorldPartitionSubsystem* Partition = World ? World->GetSubsystem<UWorldPartitionSubsystem>() : nullptr;
		const bool bWorldReady = !World || !World->GetWorldPartition() || (Partition && Partition->IsAllStreamingCompleted());
		if (Experience && Experience->IsExperienceLoaded() && PC && PC->GetPawn() && bHUDReady && bWorldReady)
		{
			if (State == EWxTravelState::Recovering
				&& UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == ReturnLevelPackage.ToString())
			{
				State = EWxTravelState::Idle;
			}
			else if (State == EWxTravelState::AwaitingReady && ValidateArrival(World, Experience->GetCurrentExperience()))
			{
				if (!PC->GetPawn()->IsA(SelectedPawnClass))
				{
					Fail(LOCTEXT("WrongPawn", "선택한 캐릭터를 생성하지 못했습니다."));
					return true;
				}
				RunState.PawnClass = PendingPawnClass;
				RunState.Level = PendingLevel;
				State = EWxTravelState::Idle;
				StatusText = FText::GetEmpty();
				ReleaseArrivalPawn();
				// 재스폰도 같은 선택을 사용하므로 현재 맵 참조와 클래스는 유지한다.
				AssetHandle.Reset();
			}
		}
	}
	if (IsBusy() && FPlatformTime::Seconds() > Deadline)
	{
		Fail(LOCTEXT("Timeout", "입장 준비 시간이 초과되었습니다. 다시 시도해주세요."));
	}
	return true;
}

bool UWxGameFlowSubsystem::ValidateArrival(const UWorld* World, const UWxExperienceDefinition* Experience)
{
	if (State == EWxTravelState::Idle && !PendingLevel.IsNull() && !IsDestinationWorld(World))
	{
		// 일반 메뉴의 FrontEnd 복귀도 이전 월드의 선택 검증을 이어받지 않는다.
		ClearPending();
	}
	if (PendingLevel.IsNull() || State == EWxTravelState::Preparing || State == EWxTravelState::Recovering)
	{
		return true;
	}
	if (State == EWxTravelState::Failed)
	{
		return false;
	}
	if (!IsDestinationWorld(World) || !Experience || !SelectedPawnClass)
	{
		Fail(LOCTEXT("ArrivalMismatch", "목적지의 게임 구성이 요청과 일치하지 않습니다."));
		return false;
	}
	return true;
}

void UWxGameFlowSubsystem::HoldArrivalPawn(APlayerController* Controller)
{
	if (!Controller || !IsBusy() || !IsDestinationWorld(Controller->GetWorld()) || HeldController.IsValid())
	{
		return;
	}
	HeldController = Controller;
	Controller->SetIgnoreMoveInput(true);
	Controller->SetIgnoreLookInput(true);
	HeldPawn = Controller->GetPawn();
	if (APawn* Pawn = HeldPawn.Get())
	{
		bPawnInputWasEnabled = Pawn->InputEnabled();
		Pawn->DisableInput(Controller);
	}
	if (ACharacter* Character = Cast<ACharacter>(Controller->GetPawn()))
	{
		HeldMovement = Character->GetCharacterMovement();
		bMovementTickWasEnabled = HeldMovement->IsComponentTickEnabled();
		// Pawn은 스트리밍 소스 위치를 제공하되 지형 준비 전에 중력으로 떨어지지 않는다.
		HeldMovement->SetComponentTickEnabled(false);
	}
}

void UWxGameFlowSubsystem::ReleaseArrivalPawn()
{
	if (APlayerController* Controller = HeldController.Get())
	{
		if (HeldPawn.IsValid() && bPawnInputWasEnabled)
		{
			HeldPawn->EnableInput(Controller);
		}
		Controller->SetIgnoreMoveInput(false);
		Controller->SetIgnoreLookInput(false);
	}
	if (UCharacterMovementComponent* Movement = HeldMovement.Get())
	{
		Movement->SetComponentTickEnabled(bMovementTickWasEnabled);
	}
	HeldController.Reset();
	HeldPawn.Reset();
	HeldMovement.Reset();
}

void UWxGameFlowSubsystem::Fail(const FText& Reason)
{
	bRecoveryRequired = State == EWxTravelState::Traveling || State == EWxTravelState::AwaitingReady;
	State = EWxTravelState::Failed;
	StatusText = Reason;
	if (AssetHandle)
	{
		AssetHandle->CancelHandle();
		AssetHandle.Reset();
	}
}

void UWxGameFlowSubsystem::BeginRecovery()
{
	ClearPending();
	if (ReturnLevelPackage.IsNone() || !FPackageName::DoesPackageExist(ReturnLevelPackage.ToString()))
	{
		State = EWxTravelState::Failed;
		StatusText = LOCTEXT("RecoveryFailed", "출발 레벨로 돌아갈 수 없습니다. 게임을 다시 실행해주세요.");
		return;
	}
	State = EWxTravelState::Recovering;
	Deadline = FPlatformTime::Seconds() + TravelTimeoutSeconds;
	UGameplayStatics::OpenLevel(this, ReturnLevelPackage);
}

void UWxGameFlowSubsystem::CancelPreparation()
{
	if (State == EWxTravelState::Preparing)
	{
		State = EWxTravelState::Idle;
		ClearPending();
		StatusText = FText::GetEmpty();
	}
}

void UWxGameFlowSubsystem::ClearPending()
{
	ReleaseArrivalPawn();
	ActiveRequestId.Invalidate();
	if (AssetHandle)
	{
		AssetHandle->CancelHandle();
		AssetHandle.Reset();
	}
	PendingPawnClass.Reset();
	PendingLevel.Reset();
	SelectedPawnClass = nullptr;
}

bool UWxGameFlowSubsystem::IsWorldPackage(const UWorld* World, const TSoftObjectPtr<UWorld>& Map) const
{
	return World && UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == Map.ToSoftObjectPath().GetLongPackageName();
}

bool UWxGameFlowSubsystem::IsDestinationWorld(const UWorld* World) const
{
	return !PendingLevel.IsNull() && IsWorldPackage(World, PendingLevel);
}

UClass* UWxGameFlowSubsystem::GetSelectedPawnClass(const UWorld* World) const
{
	return IsDestinationWorld(World) ? SelectedPawnClass.Get() : nullptr;
}

bool UWxGameFlowSubsystem::IsBusy() const
{
	return bRecoveryRequired || (State != EWxTravelState::Idle && State != EWxTravelState::Failed);
}

EWxTravelState UWxGameFlowSubsystem::GetTravelState() const
{
	return State;
}

const FText& UWxGameFlowSubsystem::GetStatusText() const
{
	return StatusText;
}

const FWxRunState& UWxGameFlowSubsystem::GetRunState() const
{
	return RunState;
}

#undef LOCTEXT_NAMESPACE
