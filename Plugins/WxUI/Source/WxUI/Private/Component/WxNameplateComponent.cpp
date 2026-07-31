// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "AbilitySystemComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Character.h"
#include "WxGameplayTags.h"
#include "GameFramework/PlayerController.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 거리 스케일은 보일 때만 의미가 있다. 기본 숨김에 맞춰 틱도 꺼진 채로 출발하고, RefreshVisibility 가 표시와 함께 켠다.
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);

	// 기본 숨김: 적의 존재를 미리 노출하지 않는다.
	// 인식/락온 시 RefreshVisibility 가 노출한다.
	SetVisibility(false);

	// 기본 표시 정책: 죽지 않았고 (인식되거나 락온됨).
	// 보스·샌드백 등 특수 엔티티는 BP 에서 오버라이드한다.
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

	// SetRenderScale 은 값이 같아도 위젯을 무효화하므로, 눈에 띄지 않는 미세 변화로 Slate 무효화를 쌓지 않는다.
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

	// 캐릭터 Composite VM 하나로 어트리뷰트/이펙트(자식 AbilitySystem VM)와 표시 데이터(이름/초상화/설명)를 함께 노출한다.
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

	const bool bShouldShow = VisibilityRequirements.RequirementsMet(OwnedTags);
	SetVisibility(bShouldShow);

	// 숨겨진 네임플레이트는 거리 스케일을 갱신할 이유가 없다. 월드의 적 대부분이 기본 숨김이라 이 차이가 곧 전체 비용이다.
	SetComponentTickEnabled(bShouldShow);
}

void UWxNameplateComponent::HandleOwnedTagsChanged(const FGameplayTag Tag, int32 NewCount)
{
	RefreshVisibility();
}
