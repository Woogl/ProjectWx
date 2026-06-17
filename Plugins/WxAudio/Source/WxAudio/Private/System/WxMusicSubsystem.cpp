// Copyright Woogle. All Rights Reserved.

#include "System/WxMusicSubsystem.h"
#include "System/WxMusicSettings.h"
#include "WxBGMData.h"

#include "ChooserFunctionLibrary.h"
#include "IObjectChooser.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UWxMusicSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// BGM 은 로컬 전용. 데디케이티드 서버에서는 아무 것도 하지 않는다.
	if (InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const UWxMusicSettings* Settings = GetDefault<UWxMusicSettings>();
	if (Settings && !Settings->DefaultBGMChooser.IsNull())
	{
		Chooser = Settings->DefaultBGMChooser.LoadSynchronous();
	}

	const float Interval = Settings ? Settings->ReevaluateInterval : 0.5f;
	if (Interval > 0.f)
	{
		InWorld.GetTimerManager().SetTimer(ReevaluateTimerHandle, this, &UWxMusicSubsystem::HandleReevaluate, Interval, true);
	}

	HandleReevaluate();
}

void UWxMusicSubsystem::Deinitialize()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReevaluateTimerHandle);
	}

	if (APlayerController* PC = BoundController.Get())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
	}
	BoundController = nullptr;

	if (CurrentComponent)
	{
		CurrentComponent->Stop();
		CurrentComponent = nullptr;
	}
	if (PreviousComponent)
	{
		PreviousComponent->Stop();
		PreviousComponent = nullptr;
	}

	Super::Deinitialize();
}

void UWxMusicSubsystem::StartBGM(const FGameplayTag& InBGMTag)
{
	bSuspended = false;
	BGMTag = InBGMTag;
	HandleReevaluate();
}

void UWxMusicSubsystem::StopBGM()
{
	bSuspended = true;
	ApplyBGM(nullptr);
}

void UWxMusicSubsystem::HandleReevaluate()
{
	BindLocalController();

	if (bSuspended)
	{
		return;
	}

	ApplyBGM(EvaluateBGM());
}

void UWxMusicSubsystem::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	HandleReevaluate();
}

void UWxMusicSubsystem::BindLocalController()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC || PC == BoundController.Get())
	{
		return;
	}

	if (APlayerController* Old = BoundController.Get())
	{
		Old->OnPossessedPawnChanged.RemoveDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
	}

	PC->OnPossessedPawnChanged.AddDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
	BoundController = PC;
}

UWxBGMData* UWxMusicSubsystem::EvaluateBGM()
{
	if (!Chooser)
	{
		return nullptr;
	}

	// 컨텍스트 채우기. 멤버 ChooserContext 는 AddStructParam 이 참조만 잡으므로 평가 동안 살아있어야 한다.
	ChooserContext.PlayerStateTags.Reset();
	if (APawn* Pawn = GetLocalPlayerPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			ASC->GetOwnedGameplayTags(ChooserContext.PlayerStateTags);
		}
	}

	ChooserContext.BGMTag = BGMTag;

	FChooserEvaluationContext Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddStructParam(ChooserContext);

	UObject* Result = UChooserFunctionLibrary::EvaluateObjectChooserBase(
		Context,
		UChooserFunctionLibrary::MakeEvaluateChooser(Chooser),
		UWxBGMData::StaticClass(),
		false);

	return Cast<UWxBGMData>(Result);
}

void UWxMusicSubsystem::ApplyBGM(UWxBGMData* NewBGM)
{
	if (NewBGM == CurrentBGM)
	{
		return;
	}

	// 직전 전환에서 아직 페이드 아웃 중이던 컴포넌트가 있으면 즉시 정리(스택 방지).
	if (PreviousComponent)
	{
		PreviousComponent->Stop();
		PreviousComponent = nullptr;
	}

	if (CurrentComponent)
	{
		const float OutFade = CurrentBGM ? CurrentBGM->FadeOutTime : 0.f;
		CurrentComponent->FadeOut(OutFade, 0.f);
		PreviousComponent = CurrentComponent;
		CurrentComponent = nullptr;
	}

	CurrentBGM = NewBGM;

	if (NewBGM && NewBGM->Sound)
	{
		UAudioComponent* NewComponent = UGameplayStatics::SpawnSound2D(this, NewBGM->Sound);
		if (NewComponent)
		{
			NewComponent->FadeIn(NewBGM->FadeInTime);
			CurrentComponent = NewComponent;
		}
	}
}

APawn* UWxMusicSubsystem::GetLocalPlayerPawn() const
{
	return UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}
