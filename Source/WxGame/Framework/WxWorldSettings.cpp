// Copyright Woogle. All Rights Reserved.

#include "Framework/WxWorldSettings.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AttributeSet.h"
#include "Character/WxCharacterBase.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"
#include "WxGame.h"

FPrimaryAssetId AWxWorldSettings::GetDefaultGameplayExperience() const
{
	if (GameplayExperience.IsNull())
	{
		return FPrimaryAssetId();
	}

	const FPrimaryAssetId Result = UAssetManager::Get().GetPrimaryAssetIdForPath(GameplayExperience.ToSoftObjectPath());
	if (!Result.IsValid())
	{
		UE_LOG(LogWxGame, Error, TEXT("GetDefaultGameplayExperience: '%s' 가 AssetManager 에 스캔돼 있지 않음. 스캔 폴더(/Game/Framework) 확인 필요."), *GameplayExperience.ToString());
	}

	return Result;
}

void AWxWorldSettings::OnSavePreparing()
{
	if (!HasAuthority())
	{
		return;
	}

	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AActor* PlayerActor = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (PlayerActor)
	{
		CapturePlayerState(PlayerActor);
	}
}

void AWxWorldSettings::OnSaveRestored(const TArray<FName>& RestoredPropertyNames)
{
	bPlayerRestorePending = RestoredPropertyNames.Contains(
		GET_MEMBER_NAME_CHECKED(AWxWorldSettings, PlayerPersistenceState))
		&& PlayerPersistenceState.bHasData;
}

void AWxWorldSettings::ApplyPendingPlayerState(AActor* PlayerActor)
{
	if (!HasAuthority() || !PlayerActor)
	{
		return;
	}

	if (!bPlayerRestorePending)
	{
		return;
	}

	ApplyPlayerState(PlayerActor);
	bPlayerRestorePending = false;
}

void AWxWorldSettings::CapturePlayerState(AActor* PlayerActor)
{
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!AbilitySystem)
	{
		return;
	}

	FWxPersistedPlayerState NewState;
	for (const UAttributeSet* AttributeSet : AbilitySystem->GetSpawnedAttributes())
	{
		if (!AttributeSet)
		{
			continue;
		}

		for (TFieldIterator<FStructProperty> It(AttributeSet->GetClass()); It; ++It)
		{
			if (It->Struct != FGameplayAttributeData::StaticStruct() || !It->HasAnyPropertyFlags(CPF_Net))
			{
				continue;
			}

			FWxPersistedGameplayAttribute& SavedAttribute = NewState.Attributes.AddDefaulted_GetRef();
			SavedAttribute.AttributeSetClass = AttributeSet->GetClass();
			SavedAttribute.AttributeName = It->GetFName();
			SavedAttribute.BaseValue = AbilitySystem->GetNumericAttributeBase(FGameplayAttribute(*It));
		}
	}

	if (const AWxCharacterBase* Character = Cast<AWxCharacterBase>(PlayerActor))
	{
		Character->CaptureAbilitySystemState(NewState.AbilitySystem);
	}

	NewState.bHasData = !NewState.Attributes.IsEmpty() || !NewState.AbilitySystem.GameplayEffects.IsEmpty();
	PlayerPersistenceState = MoveTemp(NewState);
}

void AWxWorldSettings::ApplyPlayerState(AActor* PlayerActor)
{
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(PlayerActor);
	if (!AbilitySystem)
	{
		return;
	}

	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const FWxPersistedGameplayAttribute& SavedAttribute : PlayerPersistenceState.Attributes)
		{
			const bool bMaxAttribute = SavedAttribute.AttributeName.ToString().StartsWith(TEXT("Max"));
			if ((Pass == 0) != bMaxAttribute)
			{
				continue;
			}

			UClass* AttributeSetClass = SavedAttribute.AttributeSetClass.LoadSynchronous();
			const UAttributeSet* MatchingSet = nullptr;
			for (const UAttributeSet* AttributeSet : AbilitySystem->GetSpawnedAttributes())
			{
				if (AttributeSet && AttributeSetClass && AttributeSet->IsA(AttributeSetClass))
				{
					MatchingSet = AttributeSet;
					break;
				}
			}

			FStructProperty* Property = MatchingSet
				? FindFProperty<FStructProperty>(MatchingSet->GetClass(), SavedAttribute.AttributeName)
				: nullptr;
			if (Property && Property->Struct == FGameplayAttributeData::StaticStruct())
			{
				AbilitySystem->SetNumericAttributeBase(FGameplayAttribute(Property), SavedAttribute.BaseValue);
			}
		}
	}

	if (AWxCharacterBase* Character = Cast<AWxCharacterBase>(PlayerActor))
	{
		Character->RestoreAbilitySystemState(PlayerPersistenceState.AbilitySystem);
	}
}
