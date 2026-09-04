// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Character.h"
#include "Framework/Application/SlateApplication.h"
#include "GameplayTagAssetInterface.h"
#include "Misc/CoreMisc.h"
#include "WxGameplayTags.h"
#include "WxUIModule.h"
#include "Kismet/GameplayStatics.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	// 스크린 스페이스 위젯의 화면 부착·해제를 엔진이 이 틱에서만 처리하므로, 틱을 끄면 숨김이 화면에 반영되지 않는다.
	PrimaryComponentTick.bCanEverTick = true;
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);

	// 첫 가시성 판정 전에 한 프레임 노출되지 않도록 숨겨 둔다.
	SetVisibility(false);

	VisibilityRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);

	FGameplayTagContainer ShowAnyTags;
	ShowAnyTags.AddTag(WxGameplayTags::State_LockedOn);
	ShowAnyTags.AddTag(WxGameplayTags::State_Engaged);
	VisibilityRequirements.TagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(ShowAnyTags);
}

void UWxNameplateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		SetVisibility(false);
		return;
	}

	const float Distance = FVector::Dist(GetComponentLocation(), PlayerPawn->GetActorLocation());
	if (Distance > MaxVisibilityDistance)
	{
		SetVisibility(false);
		return;
	}

	FGameplayTagContainer OwnedTags;
	if (const IGameplayTagAssetInterface* TagOwner = Cast<IGameplayTagAssetInterface>(GetOwner()))
	{
		TagOwner->GetOwnedGameplayTags(OwnedTags);
	}

	if (!VisibilityRequirements.RequirementsMet(OwnedTags))
	{
		SetVisibility(false);
		return;
	}

	SetVisibility(true);

	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	const float Scale = FMath::Clamp(ReferenceDistance / FMath::Max(Distance, 1.f), MinScale, MaxScale);
	if (FMath::IsNearlyEqual(Scale, LastRenderScale, KINDA_SMALL_NUMBER))
	{
		return;
	}

	LastRenderScale = Scale;
	NameplateWidget->SetRenderScale(FVector2D(Scale, Scale));
}

void UWxNameplateComponent::InitializeViewModels(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait)
{
	// 호출 계약 위반은 빌드를 가리지 않고 알린다 — 그래서 데디 서버 판별보다 앞에 둔다.
	if (!InASC)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: ASC 없이 초기화를 요청받아 네임플레이트를 띄울 수 없다(%s)."), *GetNameSafe(GetOwner()));
		return;
	}

	// 엔진은 데디 서버이거나 Slate 가 없으면 위젯을 만들지 않는다(UWidgetComponent::InitWidget).
	// 둘 다 위젯이 없는 게 정상인 환경이라, 아래 위젯 부재 경고가 오경보로 울리지 않도록 그 앞에서 빠진다.
	if (IsRunningDedicatedServer() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>();
	if (!View)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯에 MVVM View 확장이 없어 뷰모델을 묶을 수 없다(%s)."), *GetNameSafe(NameplateWidget->GetClass()));
		return;
	}

	UWxViewModel_Character* CharacterViewModel = NewObject<UWxViewModel_Character>(this);
	CharacterViewModel->Initialize(InASC, InCharacterName, InPortrait);
	View->SetViewModelByClass(CharacterViewModel);
}
