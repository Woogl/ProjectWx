// Copyright Woogle. All Rights Reserved.

#include "WxPlayerSpawnComponent.h"

#include "Engine/PlayerStartPIE.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "WxSaveGameSubsystem.h"
#include "WxSaveModule.h"

void UWxPlayerSpawnComponent::OnRegister()
{
	Super::OnRegister();

	// BeginPlay 가 아니라 OnRegister 에서 구독한다: SpawnPlayActor(Login -> PostLogin)가 World BeginPlay 보다 먼저라 BeginPlay 시점엔 PostLogin 을 이미 놓친다.
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !GetWorld() || !GetWorld()->IsGameWorld() || PostLoginHandle.IsValid())
	{
		return;
	}

	PostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UWxPlayerSpawnComponent::HandleGameModePostLogin);

	// 지연 스폰의 RestartPlayer 는 로드 완료 브로드캐스트 뒤라 항상 이 캐치업보다 늦다.
	// 로그인 경과는 PlayerState 유무로 가른다(컨트롤러 초기화 중 부착이면 아직 없고, 로그인 완료 뒤 부착이면 있다).
	APlayerController* OwnerPC = GetController<APlayerController>();
	if (OwnerPC && OwnerPC->PlayerState)
	{
		HandleGameModePostLogin(nullptr, OwnerPC);
	}
}

void UWxPlayerSpawnComponent::OnUnregister()
{
	FGameModeEvents::OnGameModePostLoginEvent().Remove(PostLoginHandle);
	PostLoginHandle.Reset();

	Super::OnUnregister();
}

void UWxPlayerSpawnComponent::HandleGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	// GameModePostLoginEvent 는 static 이라 모든 PC 의 로그인이 들어온다.
	if (!NewPlayer || NewPlayer != GetController<APlayerController>())
	{
		return;
	}

	// 스탯 복원 구독은 조건 없이 건다 — 저장 스탯은 맵과 무관하고 PIE "여기서 플레이" 로 시작해도 복원 대상이다.
	NewPlayer->OnPossessedPawnChanged.AddDynamic(this, &UWxPlayerSpawnComponent::HandlePossessedPawnChanged);

	// PIE "여기서 플레이"(APlayerStartPIE)가 있으면 저장 위치로 덮지 않는다 — 개발 중 지정 위치 우선.
	// StartSpot 을 비워두면 엔진 ChoosePlayerStart 가 APlayerStartPIE 를 최우선으로 고른다.
	for (TActorIterator<APlayerStartPIE> It(GetWorld()); It; ++It)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UWxSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	FTransform SavedTransform;
	if (!SaveSubsystem || !SaveSubsystem->TryGetPlayerTransform(GetWorld(), SavedTransform))
	{
		return;
	}

	// 폰을 pitch/roll 없이 스폰하는 엔진 SpawnDefaultPawnFor 정책에 맞춰 Yaw 만 남긴다.
	const FVector SpawnLocation = SavedTransform.GetLocation();
	const FRotator SpawnRotation(0.0f, SavedTransform.GetRotation().Rotator().Yaw, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* Marker = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
	if (!Marker)
	{
		UE_LOG(LogWxSave, Warning, TEXT("HandleGameModePostLogin: 재개 지점 마커 스폰 실패 — 엔진 기본 시작 지점으로 폴백한다."));
		return;
	}

	NewPlayer->StartSpot = Marker;

	// PC 는 그 자체로 월드파티션 스트리밍 소스이고 빙의 전 ViewTarget 이 자기 자신이라, 폰이 스폰되기 전에 그 셀의 로딩이 시작된다.
	NewPlayer->SetInitialLocationAndRotation(SpawnLocation, SpawnRotation);
}

void UWxPlayerSpawnComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// UnPossess 는 NewPawn 이 null 이라 ApplySavedPlayerStats 의 액터 가드에 걸려 noop 이다.
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UWxSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr)
	{
		SaveSubsystem->ApplySavedPlayerStats(NewPawn);
	}
}
