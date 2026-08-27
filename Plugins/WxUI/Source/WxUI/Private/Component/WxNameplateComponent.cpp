// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Character.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CoreMisc.h"
#include "WxGameplayTags.h"
#include "WxUIModule.h"
#include "GameFramework/PlayerController.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	// 스크린 스페이스 위젯의 화면 부착·해제를 엔진이 이 틱에서만 처리하므로, 틱을 끄면 숨김이 화면에 반영되지 않는다.
	PrimaryComponentTick.bCanEverTick = true;
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);

	// 기본 숨김: 적의 존재를 미리 노출하지 않는다.
	SetVisibility(false);

	// 표시(둘 중 하나)는 OR 이라 TagQuery(MatchAny)로, 숨김은 IgnoreTags(HasAny면 숨김)로 둔다.
	VisibilityRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);

	FGameplayTagContainer ShowAnyTags;
	ShowAnyTags.AddTag(WxGameplayTags::State_InCombat);
	ShowAnyTags.AddTag(WxGameplayTags::State_LockedOn);
	VisibilityRequirements.TagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(ShowAnyTags);
}

void UWxNameplateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 화면 부착·해제를 맡은 Super 는 지나가되, 거리 스케일은 보일 때만 계산한다.
	// 판정이 갈리지 않도록 엔진의 화면 부착 조건과 같은 술어를 쓴다.
	if (!IsVisible())
	{
		return;
	}

	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const float Distance = FVector::Dist(GetComponentLocation(), CameraLocation);
	const float Scale = FMath::Clamp(ReferenceDistance / FMath::Max(Distance, 1.0f), MinScale, MaxScale);

	if (FMath::IsNearlyEqual(Scale, LastRenderScale, KINDA_SMALL_NUMBER))
	{
		return;
	}

	LastRenderScale = Scale;
	NameplateWidget->SetRenderScale(FVector2D(Scale, Scale));
}

void UWxNameplateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWatchingVisibilityTags();

	Super::EndPlay(EndPlayReason);
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
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯이 없어 네임플레이트가 영영 숨김에 머문다(%s). WidgetClass 지정을 확인한다."), *GetNameSafe(GetOwner()));
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

	// 조기 반환을 모두 지난 자리라, 초기화가 실패했을 때 이미 서 있던 표시를 끊지 않는다.
	StopWatchingVisibilityTags();

	CachedASC = InASC;

	// 표시 여부는 ASC 태그가 바뀔 때만 갱신하면 충분하다(매 틱 재계산 불필요).
	// 표시는 태그 유무로만 갈리므로 카운트 변화까지 볼 필요가 없다.
	FGameplayTagContainer WatchedTags;
	CollectVisibilityTags(WatchedTags);
	for (const FGameplayTag& WatchedTag : WatchedTags)
	{
		InASC->RegisterGameplayTagEvent(WatchedTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxNameplateComponent::HandleOwnedTagsChanged);
	}

	// 이벤트는 변화 시에만 발화하므로 초기 표시 상태는 여기서 한 번 확정한다.
	RefreshVisibility();
}

void UWxNameplateComponent::CollectVisibilityTags(FGameplayTagContainer& OutTags) const
{
	OutTags.AppendTags(VisibilityRequirements.RequireTags);
	OutTags.AppendTags(VisibilityRequirements.IgnoreTags);

	// 기본값처럼 조건이 TagQuery 에 들어 있어도 참조 태그를 전부 돌려준다.
	for (const FGameplayTag& QueryTag : VisibilityRequirements.TagQuery.GetGameplayTagArray())
	{
		OutTags.AddTag(QueryTag);
	}
}

void UWxNameplateComponent::StopWatchingVisibilityTags()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		FGameplayTagContainer WatchedTags;
		CollectVisibilityTags(WatchedTags);
		for (const FGameplayTag& WatchedTag : WatchedTags)
		{
			ASC->RegisterGameplayTagEvent(WatchedTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
		}
	}

	CachedASC.Reset();
}

void UWxNameplateComponent::RefreshVisibility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		return;
	}

	// 태그 변경 이벤트가 표시 결과와 무관하게 발화할 수 있으나, SetVisibility 는 값이 같으면 내부에서 no-op 이다.
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	// 숨김은 컴포넌트를 화면 위젯 레이어에서 빼내 매 프레임 투영 비용까지 없앤다.
	// 월드의 적 대부분이 기본 숨김이라 이 차이가 곧 전체 비용이다.
	SetVisibility(VisibilityRequirements.RequirementsMet(OwnedTags));
}

void UWxNameplateComponent::HandleOwnedTagsChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshVisibility();
}
