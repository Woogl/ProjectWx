// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Character.h"
#include "WxGameplayTags.h"
#include "GameFramework/PlayerController.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	// 틱은 늘 켜 둔다. 스크린 스페이스 위젯의 화면 부착·해제를 엔진이 이 틱에서만 처리하므로, 틱을 끄면 숨김이 화면에 반영되지 않는다.
	PrimaryComponentTick.bCanEverTick = true;
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);

	// 기본 숨김: 적의 존재를 미리 노출하지 않는다.
	// 인식/락온 시 RefreshVisibility 가 노출한다.
	SetVisibility(false);

	// 표시(둘 중 하나)는 OR 이라 TagQuery(MatchAny)로, 숨김은 IgnoreTags(HasAny면 숨김)로 둔다.
	VisibilityRequirements.IgnoreTags.AddTag(WxGameplayTags::State_Dead);

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
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UWxNameplateComponent::InitializeViewModels(UAbilitySystemComponent* InASC, const FWxCharacterUIData& InUIData)
{
	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget)
	{
		return;
	}

	UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>();
	if (!View)
	{
		return;
	}

	UWxViewModel_Character* CharacterViewModel = NewObject<UWxViewModel_Character>(this);
	CharacterViewModel->Initialize(InASC, InUIData);
	View->SetViewModelByClass(CharacterViewModel);

	CachedASC = InASC;

	// 표시 여부는 ASC 태그가 바뀔 때만 갱신하면 충분하다(매 틱 재계산 불필요).
	InASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxNameplateComponent::HandleOwnedTagsChanged);

	// 이벤트는 변화 시에만 발화하므로 초기 표시 상태는 여기서 한 번 확정한다.
	RefreshVisibility();
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

	// 숨김은 컴포넌트를 화면 위젯 레이어에서 빼내므로, 거리 스케일 계산뿐 아니라 레이어의 매 프레임 투영 비용까지 함께 사라진다.
	// 월드의 적 대부분이 기본 숨김이라 이 차이가 곧 전체 비용이다.
	SetVisibility(VisibilityRequirements.RequirementsMet(OwnedTags));
}

void UWxNameplateComponent::HandleOwnedTagsChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshVisibility();
}
