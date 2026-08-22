// Copyright Woogle. All Rights Reserved.

#include "Framework/WxExperienceManagerComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Framework/WxExperienceActionSet.h"
#include "Framework/WxExperienceDefinition.h"
#include "Framework/WxExperienceManager.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "Net/UnrealNetwork.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxHUDLayout.h"
#include "WxGame.h"

// FGameFeatureDeactivatingContext 의 pauser 완료 콜백. 비동기 비활성은 지원하지 않으므로 할 일이 없다.
static void WxHandleDeactivationPauserCompleted(FStringView PauserTag)
{
}

/** 지급 목록과 같은 규칙으로 액션셋에서 찾는다. Experience 가 없거나 지정이 없으면 빈 값을 발행해 이전 세계의 지정을 지운다. */
static void WxPublishGameHUDClass(const UObject* WorldContextObject, const UWxExperienceDefinition* Experience)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWxUIManagerSubsystem* UIManager = GameInstance ? GameInstance->GetSubsystem<UWxUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	TSoftClassPtr<UWxHUDLayout> GameHUDClass;
	if (Experience)
	{
		for (const TObjectPtr<UWxExperienceActionSet>& ActionSet : Experience->ActionSets)
		{
			if (ActionSet && !ActionSet->GameHUDClass.IsNull())
			{
				GameHUDClass = ActionSet->GameHUDClass;
				break;
			}
		}
	}

	UIManager->SetGameHUDClass(GameHUDClass);
}

UWxExperienceManagerComponent::UWxExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UWxExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// PIE 다중 세션에선 마지막 요청자일 때만 실제로 내린다.
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		if (UWxExperienceManager::RequestToDeactivatePlugin(PluginURL))
		{
			UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
		}
	}

	WxPublishGameHUDClass(this, nullptr);

	if (LoadState == EWxExperienceLoadState::Loaded)
	{
		LoadState = EWxExperienceLoadState::Deactivating;

		// 액션 비활성화를 자기 월드로 한정한다 — PIE 다중 세션에서 남의 월드를 건드리지 않는다.
		FGameFeatureDeactivatingContext Context(TEXT(""), &WxHandleDeactivationPauserCompleted);
		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}

		TArray<UGameFeatureAction*> ActionsToDeactivate;
		CollectActions(ActionsToDeactivate);
		for (UGameFeatureAction* Action : ActionsToDeactivate)
		{
			Action->OnGameFeatureDeactivating(Context);
			Action->OnGameFeatureUnregistering();
		}

		if (Context.GetNumPausers() > 0)
		{
			UE_LOG(LogWxGame, Error, TEXT("EndPlay: 비동기 비활성(pauser)을 요구하는 액션은 지원하지 않는다. 정리가 완주되지 않았을 수 있다."));
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UWxExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxExperienceManagerComponent, CurrentExperience);
}

void UWxExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId ExperienceId)
{
	if (CurrentExperience)
	{
		return;
	}

	if (!ExperienceId.IsValid())
	{
		UE_LOG(LogWxGame, Warning, TEXT("SetCurrentExperience: Experience 미설정. 프레임워크 컴포넌트가 주입되지 않음."));
		return;
	}

	const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(ExperienceId);
	const UWxExperienceDefinition* Experience = Cast<UWxExperienceDefinition>(AssetPath.TryLoad());
	if (!Experience)
	{
		UE_LOG(LogWxGame, Error, TEXT("SetCurrentExperience: '%s' 를 Experience 에셋으로 해석하지 못함. AssetManager 스캔 설정과 에셋 경로 확인 필요."), *ExperienceId.ToString());
		return;
	}

	CurrentExperience = Experience;
	StartExperienceLoad();
}

void UWxExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FWxOnExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}

bool UWxExperienceManagerComponent::IsExperienceLoaded() const
{
	return LoadState == EWxExperienceLoadState::Loaded && CurrentExperience;
}

const UWxExperienceDefinition* UWxExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == EWxExperienceLoadState::Loaded && CurrentExperience);

	return CurrentExperience;
}

const UWxExperienceDefinition* UWxExperienceManagerComponent::GetCurrentExperience() const
{
	return CurrentExperience;
}

void UWxExperienceManagerComponent::OnRep_CurrentExperience()
{
	// async 패키지 로딩 설정에선 참조가 unmapped(null)로 먼저 도착할 수 있다. 매핑이 끝나면 RepNotify 가 다시 불려 회복된다.
	if (!CurrentExperience || LoadState != EWxExperienceLoadState::Unloaded)
	{
		return;
	}

	StartExperienceLoad();
}

void UWxExperienceManagerComponent::StartExperienceLoad()
{
	check(CurrentExperience);
	check(LoadState == EWxExperienceLoadState::Unloaded);

	LoadState = EWxExperienceLoadState::Loading;
	UE_LOG(LogWxGame, Log, TEXT("Experience '%s' 로드 시작."), *GetNameSafe(CurrentExperience));

	// 번들엔 액션의 소프트 참조(주입 컴포넌트 클래스 등)가 실려 있다.
	TSet<FPrimaryAssetId> BundleAssetList;
	BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());
	for (const TObjectPtr<UWxExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet)
		{
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}

	TArray<FName> BundlesToLoad;
	const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	const bool bLoadClient = GIsEditor || OwnerNetMode != NM_DedicatedServer;
	const bool bLoadServer = GIsEditor || OwnerNetMode != NM_Client;
	if (bLoadClient)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
	}
	if (bLoadServer)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
	}

	const TSharedPtr<FStreamableHandle> Handle = UAssetManager::Get().ChangeBundleStateForPrimaryAssets(
		BundleAssetList.Array(), BundlesToLoad, {}, false, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);

	FStreamableDelegate OnAssetsLoadedDelegate = FStreamableDelegate::CreateUObject(this, &UWxExperienceManagerComponent::HandleExperienceAssetsLoaded);
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// 지연 콜백 큐 규칙에 맞춰 즉시 실행한다.
		FStreamableHandle::ExecuteDelegate(MoveTemp(OnAssetsLoadedDelegate));
	}
	else
	{
		Handle->BindCompleteDelegate(OnAssetsLoadedDelegate);
		// 취소돼도 파이프라인은 계속 진행한다 — 빠진 에셋은 이후 단계가 개별 경고로 드러낸다.
		Handle->BindCancelDelegate(OnAssetsLoadedDelegate);
	}
}

void UWxExperienceManagerComponent::HandleExperienceAssetsLoaded()
{
	check(LoadState == EWxExperienceLoadState::Loading);

	GameFeaturePluginURLs.Reset();
	CollectGameFeaturePluginURLs(CurrentExperience->GameFeaturesToEnable);
	for (const TObjectPtr<UWxExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet)
		{
			CollectGameFeaturePluginURLs(ActionSet->GameFeaturesToEnable);
		}
	}

	NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();
	if (NumGameFeaturePluginsLoading == 0)
	{
		FinishExperienceLoad();
		return;
	}

	LoadState = EWxExperienceLoadState::LoadingGameFeatures;
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		UWxExperienceManager::NotifyOfPluginActivation(PluginURL);
		UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(
			PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &UWxExperienceManagerComponent::HandleGameFeaturePluginLoaded));
	}
}

void UWxExperienceManagerComponent::HandleGameFeaturePluginLoaded(const UE::GameFeatures::FResult& Result)
{
	if (Result.HasError())
	{
		UE_LOG(LogWxGame, Error, TEXT("HandleGameFeaturePluginLoaded: GameFeature 플러그인 활성 실패: %s"), *UE::GameFeatures::ToString(Result));
	}

	--NumGameFeaturePluginsLoading;
	if (NumGameFeaturePluginsLoading == 0)
	{
		FinishExperienceLoad();
	}
}

void UWxExperienceManagerComponent::FinishExperienceLoad()
{
	check(LoadState != EWxExperienceLoadState::Loaded);
	LoadState = EWxExperienceLoadState::ExecutingActions;

	// 액션 활성화를 자기 월드로 한정한다 — PIE 다중 세션에서 남의 월드에 새는 것을 막는다.
	FGameFeatureActivatingContext Context;
	const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	if (ExistingWorldContext)
	{
		Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}

	TArray<UGameFeatureAction*> ActionsToActivate;
	CollectActions(ActionsToActivate);
	for (UGameFeatureAction* Action : ActionsToActivate)
	{
		// Experience 소유 액션은 GameFeature 플러그인 수명 관리 밖이라 등록·로딩 단계를 여기서 직접 통과시킨다.
		Action->OnGameFeatureRegistering();
		Action->OnGameFeatureLoading();
		Action->OnGameFeatureActivating(Context);
	}

	// 화면을 띄우는 쪽은 Experience 를 알지 못하므로, 정해진 HUD 를 UI 매니저에 실어 준다(서버·클라 각자 주행한다).
	WxPublishGameHUDClass(this, CurrentExperience);

	LoadState = EWxExperienceLoadState::Loaded;
	UE_LOG(LogWxGame, Log, TEXT("Experience '%s' 로드 완료."), *GetNameSafe(CurrentExperience));

	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();
}

void UWxExperienceManagerComponent::CollectGameFeaturePluginURLs(const TArray<FString>& FeaturePluginList)
{
	for (const FString& PluginName : FeaturePluginList)
	{
		FString PluginURL;
		if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, PluginURL))
		{
			GameFeaturePluginURLs.AddUnique(PluginURL);
		}
		else
		{
			UE_LOG(LogWxGame, Error, TEXT("CollectGameFeaturePluginURLs: GameFeature 플러그인 '%s' 을 찾지 못함. 건너뜀."), *PluginName);
		}
	}
}

void UWxExperienceManagerComponent::CollectActions(TArray<UGameFeatureAction*>& OutActions) const
{
	for (const TObjectPtr<UGameFeatureAction>& Action : CurrentExperience->Actions)
	{
		if (Action)
		{
			OutActions.Add(Action);
		}
	}

	for (const TObjectPtr<UWxExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (!ActionSet)
		{
			continue;
		}

		for (const TObjectPtr<UGameFeatureAction>& Action : ActionSet->Actions)
		{
			if (Action)
			{
				OutActions.Add(Action);
			}
		}
	}
}
