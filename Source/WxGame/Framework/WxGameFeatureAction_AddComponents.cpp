// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameFeatureAction_AddComponents.h"

#include "AssetRegistry/AssetBundleData.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFeaturesSubsystemSettings.h"
#include "WxGame.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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

	// 컨텍스트 제거로 요청 핸들이 해제되면 매니저가 등록된 액터들에서 컴포넌트를 회수한다.
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
		if (ComponentList[Index].ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::FromString(FString::Printf(TEXT("ComponentList[%d] 의 ActorClass 가 비어 있습니다."), Index)));
		}

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

	// 넷모드 분기 없이 전 엔트리를 요청한다. 사이드 제한은 매니저의 authority 규칙과 컴포넌트 자기 가드가 담당한다.
	for (const FWxGameFeatureComponentEntry& Entry : ComponentList)
	{
		if (Entry.ActorClass.IsNull() || Entry.ComponentClass.IsNull())
		{
			continue;
		}

		const TSubclassOf<UActorComponent> ComponentClass = Entry.ComponentClass.LoadSynchronous();
		if (!ComponentClass)
		{
			UE_LOG(LogWxGame, Error, TEXT("AddToWorld: 컴포넌트 클래스 '%s' 로드 실패. 주입을 건너뜀."), *Entry.ComponentClass.ToString());
			continue;
		}

		Handles.ComponentRequestHandles.Add(ComponentManager->AddComponentRequest(Entry.ActorClass, ComponentClass));
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
