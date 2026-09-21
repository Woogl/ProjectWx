// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxFrontEndLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FrontEnd/WxGameFlowSubsystem.h"
#include "FrontEnd/WxFrontEndDeveloperSettings.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Misc/PackageName.h"
#include "Widget/WxButtonBase.h"

TArray<FWxFrontEndOption> UWxFrontEndLibrary::GetCharacterOptions()
{
	TArray<FWxFrontEndOption> Options;
	for (const TSoftClassPtr<APawn>& Character : GetDefault<UWxFrontEndDeveloperSettings>()->CharacterOptions)
	{
		FWxFrontEndOption& Option = Options.AddDefaulted_GetRef();
		Option.PawnClass = Character;
		const FSoftObjectPath& ClassPath = Character.ToSoftObjectPath();
		const FString PackageName = ClassPath.GetLongPackageName();
		const FString DisplayName = FPackageName::IsScriptPackage(PackageName)
			? ClassPath.GetAssetName()
			: FPackageName::GetShortName(PackageName);
		Option.DisplayName = FText::FromString(DisplayName);
	}
	return Options;
}

TArray<FWxFrontEndOption> UWxFrontEndLibrary::GetLevelOptions()
{
	TArray<FWxFrontEndOption> Options;
	for (const TSoftObjectPtr<UWorld>& Level : GetDefault<UWxFrontEndDeveloperSettings>()->LevelOptions)
	{
		FWxFrontEndOption& Option = Options.AddDefaulted_GetRef();
		Option.Level = Level;
		Option.DisplayName = FText::FromString(Level.ToSoftObjectPath().GetAssetName());
	}
	return Options;
}

bool UWxFrontEndLibrary::HasSelectableOptions()
{
	return GetDefault<UWxFrontEndDeveloperSettings>()->HasSelectableOptions();
}

UWidget* UWxFrontEndLibrary::BuildOptionButtons(UUserWidget* Owner, UVerticalBox* Container, TSubclassOf<UWxButtonBase> ButtonClass,
	const TArray<FWxFrontEndOption>& Options, const FWxFrontEndOptionSelected& OnSelected, float ButtonSpacing)
{
	if (!Owner || !Container || !ButtonClass)
	{
		return nullptr;
	}
	Container->ClearChildren();
	UWidget* FocusTarget = nullptr;
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		UWxButtonBase* Button = CreateWidget<UWxButtonBase>(Owner, ButtonClass);
		if (!Button)
		{
			continue;
		}
		const FWxFrontEndOption& Option = Options[Index];
		Button->SetButtonText(Option.DisplayName);
		const bool bSelectable = !Option.PawnClass.IsNull() || !Option.Level.IsNull();
		Button->SetIsEnabled(bSelectable);
		Button->OnClicked().AddWeakLambda(Owner, [OnSelected, Index]() { OnSelected.ExecuteIfBound(Index); });
		Container->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, ButtonSpacing));
		if (!FocusTarget && bSelectable)
		{
			FocusTarget = Button;
		}
	}
	return FocusTarget;
}

void UWxFrontEndLibrary::GetTravelStatus(const UObject* WorldContextObject, bool& bBusy, FText& Message)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UWxGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UWxGameFlowSubsystem>() : nullptr;
	bBusy = !Flow || Flow->IsBusy();
	Message = Flow ? Flow->GetStatusText() : FText::GetEmpty();
}

bool UWxFrontEndLibrary::RequestNewGame(const UObject* WorldContextObject, TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UWxGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UWxGameFlowSubsystem>() : nullptr;
	return Flow && Flow->RequestNewGame(PawnClass, Level);
}
