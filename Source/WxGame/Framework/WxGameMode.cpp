// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Framework/WxExperienceActionSet.h"
#include "Framework/WxExperienceDefinition.h"
#include "Framework/WxExperienceManagerComponent.h"
#include "Framework/WxGameState.h"
#include "Framework/WxPawnData.h"
#include "Framework/WxWorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "WxGame.h"

void AWxGameMode::InitGameState()
{
	Super::InitGameState();

	// Experience 확정은 모든 플레이어 로그인(SpawnPlayActor)보다 앞선다. 다만 적용(비동기 로드)은 로그인보다 늦을 수 있어,
	// 폰 스폰과 시작 지급이 로드 완료(HandleExperienceLoaded)를 기다린다.
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
	// 폰 스폰은 로드 완료 뒤로 게이트되므로 정상 경로에선 항상 Experience 의 PawnData 를 본다.
	const AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	const UWxExperienceManagerComponent* ExperienceManager = WxGameState ? WxGameState->GetExperienceManagerComponent() : nullptr;
	if (ExperienceManager && ExperienceManager->IsExperienceLoaded())
	{
		const UWxPawnData* PawnData = ExperienceManager->GetCurrentExperienceChecked()->DefaultPawnData;
		if (PawnData && PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}

		UE_LOG(LogWxGame, Warning, TEXT("GetDefaultPawnClassForController: Experience 에 PawnData 미설정. GameMode DefaultPawnClass 로 폴백."));
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AWxGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 로드 완료 전에는 스폰을 미룬다 — 재개 지점·스탯 복원 등 프레임워크 컴포넌트가 붙기 전의 스폰을 막는다.
	// 밀린 접속자는 HandleExperienceLoaded 가 일괄 스폰한다.
	const AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	if (WxGameState && !WxGameState->GetExperienceManagerComponent()->IsExperienceLoaded())
	{
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AWxGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 로드 완료 후의 접속자만 여기서 지급한다 — 완료 전 접속자는 HandleExperienceLoaded 가 일괄 지급하므로 이중 지급이 없다.
	const AWxGameState* WxGameState = Cast<AWxGameState>(GameState);
	if (WxGameState && WxGameState->GetExperienceManagerComponent()->IsExperienceLoaded())
	{
		GrantDefaultInventory(NewPlayer, WxGameState->GetExperienceManagerComponent()->GetCurrentExperienceChecked());
	}
}

FPrimaryAssetId AWxGameMode::ResolveExperienceId() const
{
	// 같은 맵이라도 진입 URL 로 다른 구성을 실험할 수 있게 URL 옵션이 우선한다.
	const FString UrlExperienceName = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
	if (!UrlExperienceName.IsEmpty())
	{
		return FPrimaryAssetId(UWxExperienceDefinition::GetPrimaryAssetTypeStatic(), FName(*UrlExperienceName));
	}

	// 맵이 자기 기본 Experience 를 지정할 수 있다(월드 세팅).
	if (const AWxWorldSettings* WxWorldSettings = Cast<AWxWorldSettings>(GetWorldSettings()))
	{
		const FPrimaryAssetId WorldSettingsExperienceId = WxWorldSettings->GetDefaultGameplayExperience();
		if (WorldSettingsExperienceId.IsValid())
		{
			return WorldSettingsExperienceId;
		}
	}

	if (DefaultExperience)
	{
		return DefaultExperience->GetPrimaryAssetId();
	}

	return FPrimaryAssetId();
}

void AWxGameMode::HandleExperienceLoaded(const UWxExperienceDefinition* Experience)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		GrantDefaultInventory(PlayerController, Experience);

		// 로드 대기로 스폰이 밀렸던 접속자를 스폰한다.
		if (!PlayerController->GetPawn() && PlayerCanRestart(PlayerController))
		{
			RestartPlayer(PlayerController);
		}
	}
}

void AWxGameMode::GrantDefaultInventory(APlayerController* PlayerController, const UWxExperienceDefinition* Experience) const
{
	// 지급 목록은 Experience 가 정의한다 — 본체와 액션셋의 목록을 이어붙인다(매니저의 GameFeature 목록 합성과 같은 규칙).
	TArray<FWxItemRewardEntry> Items = Experience->DefaultInventoryItems;
	for (const TObjectPtr<UWxExperienceActionSet>& ActionSet : Experience->ActionSets)
	{
		if (ActionSet)
		{
			Items.Append(ActionSet->DefaultInventoryItems);
		}
	}

	if (Items.IsEmpty())
	{
		return;
	}

	// 인벤토리 주입은 로드 완료 시점에 이미 붙어 있다(로드 후 접속자는 컨트롤러 초기화에서 동기 부착).
	// 복제 등록 이전이어도 ReadyForReplication 이 기존 엔트리를 back-fill 하므로 지급이 누락되지 않는다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(PlayerController))
	{
		Inventory->GrantItems(Items);
	}
}
