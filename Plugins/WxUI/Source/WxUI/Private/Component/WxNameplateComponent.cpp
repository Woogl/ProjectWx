// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"
#include "View/MVVMView.h"
#include "MVVM/WxViewModel_Character.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CoreMisc.h"
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
