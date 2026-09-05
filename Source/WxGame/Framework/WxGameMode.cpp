// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Framework/WxExperienceDefinition.h"
#include "Framework/WxExperienceManagerComponent.h"
#include "Framework/WxGameState.h"
#include "Framework/WxWorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "WxGame.h"

void AWxGameMode::InitGameState()
{
	Super::InitGameState();

	// Experience 확정은 모든 플레이어 로그인(SpawnPlayActor)보다 앞선다.
	AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	if (!WxGameState)
	{
		UE_LOG(LogWxGame, Warning, TEXT("InitGameState: GameState 가 AWxGameState 가 아님. Experience 를 적용할 수 없음."));
		return;
	}

	UWxExperienceManagerComponent* ExperienceManager = WxGameState->GetExperienceManagerComponent();
	ExperienceManager->SetCurrentExperience(ResolveExperienceId());
	ExperienceManager->CallOrRegister_OnExperienceLoaded(FWxOnExperienceLoaded::FDelegate::CreateUObject(this, &AWxGameMode::HandleExperienceLoaded));
}

UClass* AWxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// 폰 클래스는 정의 에셋 자체의 데이터라 로드 완료(주입·GF 활성)를 기다리지 않는다.
	// 엔진이 로그인 중 시작 지점을 고르며(ChoosePlayerStart) 폰 CDO 크기로 점유 여부를 재는 시점이 로드보다 이르기 때문이다.
	const AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	const UWxExperienceManagerComponent* ExperienceManager = WxGameState ? WxGameState->GetExperienceManagerComponent() : nullptr;
	const UWxExperienceDefinition* Experience = ExperienceManager ? ExperienceManager->GetCurrentExperience() : nullptr;
	if (!Experience)
	{
		// Experience 미확정은 InitGameState 가 이미 에러로 남긴 상태다.
		return nullptr;
	}

	// Super 를 부르지 않는다 — GameMode 의 DefaultPawnClass 로 폴백하면 폰 클래스의 출처가 둘이 된다.
	// 스펙테이터 폰으로 빙의시켜 HUD 가 빙의 경로 그대로 뜨게 한다.
	if (!Experience->DefaultPawnClass)
	{
		return SpectatorClass;
	}

	return Experience->DefaultPawnClass;
}

void AWxGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Experience 액션과 GameFeature 컴포넌트 적용이 끝나기 전의 스폰을 막는다.
	const AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	if (WxGameState && !WxGameState->GetExperienceManagerComponent()->IsExperienceLoaded())
	{
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

FPrimaryAssetId AWxGameMode::ResolveExperienceId() const
{
	// 같은 맵이라도 진입 URL 로 다른 구성을 실험할 수 있게 URL 옵션이 우선한다.
	const FString UrlExperienceName = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
	if (!UrlExperienceName.IsEmpty())
	{
		return FPrimaryAssetId(UWxExperienceDefinition::GetPrimaryAssetTypeStatic(), FName(*UrlExperienceName));
	}

	if (const AWxWorldSettings* WxWorldSettings = Cast<AWxWorldSettings>(GetWorldSettings()))
	{
		const FPrimaryAssetId WorldSettingsExperienceId = WxWorldSettings->GetDefaultGameplayExperience();
		if (WorldSettingsExperienceId.IsValid())
		{
			return WorldSettingsExperienceId;
		}
	}

	return FPrimaryAssetId();
}

void AWxGameMode::HandleExperienceLoaded(const UWxExperienceDefinition*)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		if (!PlayerController->GetPawn() && PlayerCanRestart(PlayerController))
		{
			RestartPlayer(PlayerController);
		}
	}
}
