// Copyright Woogle. All Rights Reserved.

#include "Controller/WxNameplateManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModelResolver_AbilitySystem.h"
#include "Targeting/WxLockOnComponent.h"
#include "View/MVVMView.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

UWxNameplateManagerComponent::UWxNameplateManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWxNameplateManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AController* OwningController = Cast<AController>(GetOwner());
	const AWxCharacterBase* Viewer = OwningController ? OwningController->GetPawn<AWxCharacterBase>() : nullptr;
	USceneComponent* LockOnTarget = Viewer ? Viewer->GetLockOnComponent()->GetLockOnTarget() : nullptr;

	// 레티클은 락온·대상 교체 순간에 바로 떠야 하므로, 목록 판정과 함께 매 프레임 따라간다.
	UpdateReticle(LockOnTarget);
	UpdateNameplates(Viewer, LockOnTarget ? LockOnTarget->GetOwner() : nullptr);
}

void UWxNameplateManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	const AController* OwningController = Cast<AController>(GetOwner());
	SetComponentTickEnabled(OwningController && OwningController->IsLocalController());
}

void UWxNameplateManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 대상은 컨트롤러보다 오래 살 수 있으므로 붙여 둔 표시를 직접 뗀다.
	for (const TPair<TWeakObjectPtr<AWxEnemyCharacter>, TWeakObjectPtr<UWidgetComponent>>& Pair : Nameplates)
	{
		if (UWidgetComponent* Nameplate = Pair.Value.Get())
		{
			Nameplate->DestroyComponent();
		}
	}
	Nameplates.Reset();

	if (UWidgetComponent* ReticleComponent = Reticle.Get())
	{
		ReticleComponent->DestroyComponent();
	}
	Reticle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxNameplateManagerComponent::UpdateNameplates(const AActor* Viewer, const AActor* LockOnActor)
{
	// 파괴된 대상의 Nameplate는 대상과 함께 이미 사라졌으므로 목록에서만 뺀다.
	for (auto It = Nameplates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<AWxEnemyCharacter> It(GetWorld()); It; ++It)
	{
		// 스트리밍 직후 BeginPlay 전인 적은 ASC가 아직 준비되지 않았다.
		AWxEnemyCharacter* Target = *It;
		if (!Target->HasActorBegunPlay())
		{
			continue;
		}

		const TWeakObjectPtr<UWidgetComponent>* Entry = Nameplates.Find(Target);
		UWidgetComponent* Nameplate = Entry ? Entry->Get() : nullptr;

		const double Distance = Viewer ? FVector::Dist(Target->GetActorLocation(), Viewer->GetActorLocation()) : 0.0;
		// 새로 붙일 때만 여유만큼 안쪽을 요구해, 경계에서 붙였다 떼기를 반복하지 않는다.
		const double VisibleDistance = Nameplate ? MaxVisibilityDistance : MaxVisibilityDistance - VisibilityDistanceHysteresis;
		// 교전하지 않은 적도 락온하면 보인다. 락온 가능 거리는 락온 쪽이 정하므로 거리 조건도 두지 않는다.
		const bool bLockedOn = Target == LockOnActor;
		const bool bInRange = Distance <= VisibleDistance;
		const bool bEngaged = Target->GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::State_Engaged);
		const bool bVisible = Viewer && Target->IsAlive() && (bLockedOn || (bInRange && bEngaged));

		if (!bVisible)
		{
			if (Entry)
			{
				if (Nameplate)
				{
					Nameplate->DestroyComponent();
				}
				Nameplates.Remove(Target);
			}
			continue;
		}

		if (!Nameplate)
		{
			UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
			Nameplate = AttachWidget(Capsule, NameplateWidgetClass);
			if (!Nameplate)
			{
				continue;
			}
			Nameplates.Add(Target, Nameplate);
			Nameplate->SetRelativeLocation(FVector(0.0, 0.0, Capsule->GetUnscaledCapsuleHalfHeight() + HeadClearance));

			// 공유본의 수명은 이를 참조하는 MVVM View가 유지한다. 여기서 직접 초기화·해제하지 않는다.
			UUserWidget* Widget = Nameplate->GetWidget();
			UMVVMView* View = Widget ? Widget->GetExtension<UMVVMView>() : nullptr;
			if (!View || !View->SetViewModelByClass(UWxViewModel_Character::GetOrCreate(
				UWxViewModelResolver_AbilitySystem::GetOrCreate(Target->GetAbilitySystemComponent()), Target->GetTitle(), Target->GetIcon())))
			{
				UE_LOG(LogWxGame, Warning, TEXT("Nameplate: Character 뷰모델을 연결하지 못했다. 위젯의 MVVM View·Manual 소스를 확인한다. Widget=%s Target=%s"), *GetNameSafe(Widget), *GetNameSafe(Target));
			}
		}

		if (UUserWidget* Widget = Nameplate->GetWidget())
		{
			const double Scale = FMath::Clamp(ReferenceDistance / FMath::Max(Distance, 1.0), static_cast<double>(MinScale), static_cast<double>(MaxScale));
			const FVector2D RenderScale(Scale, Scale);
			if (!Widget->GetRenderTransform().Scale.Equals(RenderScale))
			{
				Widget->SetRenderScale(RenderScale);
			}
		}
	}
}

void UWxNameplateManagerComponent::UpdateReticle(USceneComponent* LockOnTarget)
{
	UWidgetComponent* Current = Reticle.Get();
	if (Current && Current->GetAttachParent() == LockOnTarget)
	{
		return;
	}

	if (Current)
	{
		Current->DestroyComponent();
	}
	Reticle = AttachWidget(LockOnTarget, ReticleWidgetClass);
}

UWidgetComponent* UWxNameplateManagerComponent::AttachWidget(USceneComponent* Parent, TSubclassOf<UUserWidget> WidgetClass) const
{
	AActor* TargetActor = Parent ? Parent->GetOwner() : nullptr;
	if (!TargetActor || !WidgetClass)
	{
		return nullptr;
	}

	const APlayerController* OwningPlayer = Cast<APlayerController>(GetOwner());

	UWidgetComponent* WidgetComponent = NewObject<UWidgetComponent>(TargetActor);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetWidgetClass(WidgetClass);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetOwnerPlayer(OwningPlayer ? OwningPlayer->GetLocalPlayer() : nullptr);
	WidgetComponent->RegisterComponent();
	WidgetComponent->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	return WidgetComponent;
}
