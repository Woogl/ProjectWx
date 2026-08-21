// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameFeatureAction_AddComponents.h"

#include "AssetRegistry/AssetBundleData.h"
#include "Components/ControllerComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"
#include "Components/PawnComponent.h"
#include "Components/PlayerStateComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFeaturesSubsystemSettings.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "WxGame.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

/** 네 베이스는 UGameFrameworkComponent 아래 형제라 서로 겹치지 않으므로 검사 순서는 무관하다. */
static UClass* WxResolveReceiverClass(const UClass* ComponentClass)
{
	if (ComponentClass->IsChildOf(UPawnComponent::StaticClass()))
	{
		return APawn::StaticClass();
	}

	if (ComponentClass->IsChildOf(UControllerComponent::StaticClass()))
	{
		return AController::StaticClass();
	}

	if (ComponentClass->IsChildOf(UPlayerStateComponent::StaticClass()))
	{
		return APlayerState::StaticClass();
	}

	if (ComponentClass->IsChildOf(UGameStateComponent::StaticClass()))
	{
		return AGameStateBase::StaticClass();
	}

	return nullptr;
}

void UWxGameFeatureAction_AddComponents::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FContextHandles& Handles = ContextHandles.FindOrAdd(Context);

	Handles.GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(
		this, &UWxGameFeatureAction_AddComponents::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context));

	// 이미 초기화된 게임 인스턴스의 월드에는 지금 바로 요청한다.
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext, Handles);
		}
	}
}

void UWxGameFeatureAction_AddComponents::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FContextHandles* Handles = ContextHandles.Find(Context);
	if (!Handles)
	{
		return;
	}

	FWorldDelegates::OnStartGameInstance.Remove(Handles->GameInstanceStartHandle);

	ContextHandles.Remove(Context);
}

#if WITH_EDITORONLY_DATA
void UWxGameFeatureAction_AddComponents::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
	if (!UAssetManager::IsInitialized())
	{
		return;
	}

	// 사이드 플래그가 없으므로 전 엔트리를 클라·서버 번들 모두에 싣는다. 실제 생성 사이드는 런타임 규칙이 정한다.
	for (const FWxGameFeatureComponentEntry& Entry : ComponentList)
	{
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, Entry.ComponentClass.ToSoftObjectPath().GetAssetPath());
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, Entry.ComponentClass.ToSoftObjectPath().GetAssetPath());
	}
}
#endif

#if WITH_EDITOR
EDataValidationResult UWxGameFeatureAction_AddComponents::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	for (int32 Index = 0; Index < ComponentList.Num(); ++Index)
	{
		if (ComponentList[Index].ComponentClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::FromString(FString::Printf(TEXT("ComponentList[%d] 의 ComponentClass 가 비어 있습니다."), Index)));
		}
	}

	return Result;
}
#endif

void UWxGameFeatureAction_AddComponents::AddToWorld(const FWorldContext& WorldContext, FContextHandles& Handles)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	if (!GameInstance || !World || !World->IsGameWorld())
	{
		return;
	}

	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (!ComponentManager)
	{
		return;
	}

	for (const FWxGameFeatureComponentEntry& Entry : ComponentList)
	{
		if (Entry.ComponentClass.IsNull())
		{
			continue;
		}

		const TSubclassOf<UGameFrameworkComponent> ComponentClass = Entry.ComponentClass.LoadSynchronous();
		if (!ComponentClass)
		{
			UE_LOG(LogWxGame, Error, TEXT("AddToWorld: 컴포넌트 클래스 '%s' 로드 실패. 주입을 건너뜀."), *Entry.ComponentClass.ToString());
			continue;
		}

		UClass* ReceiverClass = WxResolveReceiverClass(ComponentClass);
		if (!ReceiverClass)
		{
			UE_LOG(LogWxGame, Error, TEXT("AddToWorld: 컴포넌트 클래스 '%s' 가 Pawn·Controller·PlayerState·GameState 컴포넌트 중 무엇도 아니라 대상 액터를 정할 수 없음. 주입을 건너뜀."), *ComponentClass->GetPathName());
			continue;
		}

		Handles.ComponentRequestHandles.Add(ComponentManager->AddComponentRequest(ReceiverClass, ComponentClass));
	}
}

void UWxGameFeatureAction_AddComponents::HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext)
{
	const FWorldContext* WorldContext = GameInstance->GetWorldContext();
	if (!WorldContext || !ChangeContext.ShouldApplyToWorldContext(*WorldContext))
	{
		return;
	}

	if (FContextHandles* Handles = ContextHandles.Find(ChangeContext))
	{
		AddToWorld(*WorldContext, *Handles);
	}
}
