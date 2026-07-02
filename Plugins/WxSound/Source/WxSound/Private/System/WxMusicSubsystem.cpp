// Copyright Woogle. All Rights Reserved.

#include "System/WxMusicSubsystem.h"
#include "System/WxMusicSettings.h"
#include "WxBGMData.h"

#include "ChooserFunctionLibrary.h"
#include "IObjectChooser.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "Components/ActorComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

	UGameInstance* GameInstance = InWorld.GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	// 이미 있는 로컬 플레이어 + 앞으로 추가될 로컬 플레이어 모두에 컨트롤러 교체 델리게이트를 건다.
	// (늦게 스폰되는 PC/폰을 폴링 없이 잡기 위한 부트스트랩.)
	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		HandleLocalPlayerAdded(LocalPlayer);
	}
	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &UWxMusicSubsystem::HandleLocalPlayerAdded);
}

void UWxMusicSubsystem::Deinitialize()
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				if (LocalPlayer)
				{
					LocalPlayer->OnPlayerControllerChanged().RemoveAll(this);
				}
			}
		}
	}

	if (APlayerController* PC = BoundController.Get())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
	}
	BoundController = nullptr;

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}
	BoundASC = nullptr;

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
	Reevaluate();
}

void UWxMusicSubsystem::StopBGM()
{
	bSuspended = true;
	ApplyBGM(nullptr);
}

void UWxMusicSubsystem::RegisterBGMSource(UObject* Source, const FGameplayTag& InMusicTag, int32 InPriority)
{
	if (!Source)
	{
		return;
	}

	// 같은 소스의 기존 등록을 먼저 제거해 갱신 겸 최근 순서로 재삽입한다(RemoveAt 로 순서 보존).
	for (int32 Index = ActiveSources.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveSources[Index].Source.Get() == Source)
		{
			ActiveSources.RemoveAt(Index);
		}
	}

	FWxBGMSourceRequest& Request = ActiveSources.AddDefaulted_GetRef();
	Request.Source = Source;
	// 소스는 컴포넌트이므로 그 소유자 액터를 미리 해석해 둔다(Chooser 가 클래스로 필터).
	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(Source))
	{
		Request.Owner = SourceComponent->GetOwner();
	}
	Request.MusicTag = InMusicTag;
	Request.Priority = InPriority;

	Reevaluate();
}

void UWxMusicSubsystem::UnregisterBGMSource(UObject* Source)
{
	if (!Source)
	{
		return;
	}

	bool bRemoved = false;
	for (int32 Index = ActiveSources.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveSources[Index].Source.Get() == Source)
		{
			ActiveSources.RemoveAt(Index);
			bRemoved = true;
		}
	}

	if (bRemoved)
	{
		Reevaluate();
	}
}

void UWxMusicSubsystem::Reevaluate()
{
	if (bSuspended)
	{
		return;
	}

	ApplyBGM(EvaluateBGM());
}

void UWxMusicSubsystem::HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}

	// 이미 컨트롤러가 있으면 즉시 반영하고, 이후 교체는 델리게이트로 잡는다.
	if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
	{
		HandlePlayerControllerChanged(PC);
	}
	LocalPlayer->OnPlayerControllerChanged().AddUObject(this, &UWxMusicSubsystem::HandlePlayerControllerChanged);
}

void UWxMusicSubsystem::HandlePlayerControllerChanged(APlayerController* PC)
{
	APlayerController* Old = BoundController.Get();
	if (Old != PC)
	{
		if (Old)
		{
			Old->OnPossessedPawnChanged.RemoveDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
		}
		BoundController = PC;
		if (PC)
		{
			PC->OnPossessedPawnChanged.AddDynamic(this, &UWxMusicSubsystem::HandlePawnChanged);
		}
	}

	// 컨트롤러가 이미 폰을 물고 있을 수 있으므로 ASC 를 지금 재바인딩한다(이후 교체는 HandlePawnChanged 가 처리).
	RebindAbilitySystem(PC ? PC->GetPawn() : nullptr);
	Reevaluate();
}

void UWxMusicSubsystem::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	RebindAbilitySystem(NewPawn);
	Reevaluate();
}

void UWxMusicSubsystem::HandleOwnedTagsChanged(const FGameplayTag Tag, int32 NewCount)
{
	Reevaluate();
}

void UWxMusicSubsystem::RebindAbilitySystem(APawn* NewPawn)
{
	UAbilitySystemComponent* NewASC = NewPawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn) : nullptr;
	UAbilitySystemComponent* OldASC = BoundASC.Get();
	if (NewASC == OldASC)
	{
		return;
	}

	if (OldASC)
	{
		OldASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}
	if (NewASC)
	{
		NewASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxMusicSubsystem::HandleOwnedTagsChanged);
	}
	BoundASC = NewASC;
}

UWxBGMData* UWxMusicSubsystem::EvaluateBGM()
{
	if (!Chooser)
	{
		return nullptr;
	}

	// 컨텍스트 채우기. 멤버 ChooserContext 는 AddStructParam 이 참조만 잡으므로 평가 동안 살아있어야 한다.
	ChooserContext.PlayerStateTags.Reset();
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GetOwnedGameplayTags(ChooserContext.PlayerStateTags);
	}

	// 승자 소스가 있으면 그 MusicTag/소유자를, 없으면 베이스라인 BGMTag/null 을 컨텍스트에 채운다.
	const FWxBGMSourceRequest* TopSource = GetTopSource();
	ChooserContext.BGMTag = TopSource ? TopSource->MusicTag : BGMTag;
	ChooserContext.SourceOwner = TopSource ? TopSource->Owner.Get() : nullptr;

	FChooserEvaluationContext Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddStructParam(ChooserContext);

	UObject* Result = UChooserFunctionLibrary::EvaluateObjectChooserBase(
		Context,
		UChooserFunctionLibrary::MakeEvaluateChooser(Chooser),
		UWxBGMData::StaticClass(),
		false);

	return Cast<UWxBGMData>(Result);
}

const FWxBGMSourceRequest* UWxMusicSubsystem::GetTopSource() const
{
	const FWxBGMSourceRequest* Best = nullptr;
	for (const FWxBGMSourceRequest& Request : ActiveSources)
	{
		// 파괴된 소스, 또는 유효하지 않은 MusicTag 는 건너뛴다(빈 태그가 베이스라인 폴백을 덮지 않도록).
		if (!Request.Source.IsValid() || !Request.MusicTag.IsValid())
		{
			continue;
		}

		// 동률은 배열 뒤(최근 등록)가 이기도록 >= 로 갱신한다.
		if (!Best || Request.Priority >= Best->Priority)
		{
			Best = &Request;
		}
	}

	return Best;
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
