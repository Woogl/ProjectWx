// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateManagerComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Component/WxNameplateSourceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMView.h"
#include "WxGameplayTags.h"
#include "WxUIModule.h"

UWxNameplateManagerComponent::UWxNameplateManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 기획: 적이 인식하면 보이고 추적이 끝나면 즉시 숨는다. 락온은 LockOnTargetQuery로 따로 들어온다.
	VisibilityRequirements.RequireTags.AddTag(WxGameplayTags::State_Engaged);
	VisibilityRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);
}

void UWxNameplateManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AController* OwningController = Cast<AController>(GetOwner());
	const APawn* ViewerPawn = OwningController ? OwningController->GetPawn() : nullptr;
	USceneComponent* LockOnTarget = ViewerPawn && LockOnTargetQuery.IsBound() ? LockOnTargetQuery.Execute() : nullptr;

	// 레티클은 락온·대상 교체 순간에 바로 떠야 하므로, 목록 판정과 함께 매 프레임 따라간다.
	UpdateReticle(LockOnTarget);
	UpdateNameplates(ViewerPawn, LockOnTarget ? LockOnTarget->GetOwner() : nullptr);
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
	for (const TPair<TWeakObjectPtr<UWxNameplateSourceComponent>, TWeakObjectPtr<UWidgetComponent>>& Pair : Nameplates)
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

void UWxNameplateManagerComponent::UpdateNameplates(const APawn* ViewerPawn, const AActor* LockOnActor)
{
	// 파괴된 대상의 Nameplate는 대상과 함께 이미 사라졌으므로 목록에서만 뺀다.
	for (auto It = Nameplates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	const UWorld* World = GetWorld();
	for (const TWeakObjectPtr<UWxNameplateSourceComponent>& WeakSource : UWxNameplateSourceComponent::GetRegisteredComponents())
	{
		UWxNameplateSourceComponent* Source = WeakSource.Get();
		AActor* Target = Source ? Source->GetOwner() : nullptr;
		if (!Target || Source->GetWorld() != World)
		{
			continue;
		}

		const TWeakObjectPtr<UWidgetComponent>* Entry = Nameplates.Find(Source);
		UWidgetComponent* Nameplate = Entry ? Entry->Get() : nullptr;

		UAbilitySystemComponent* TargetASC = nullptr;
		const double Distance = ViewerPawn ? FVector::Dist(Target->GetActorLocation(), ViewerPawn->GetActorLocation()) : 0.0;
		// 새로 붙일 때만 여유만큼 안쪽을 요구해, 경계에서 붙였다 떼기를 반복하지 않는다.
		const double VisibleDistance = Nameplate ? MaxVisibilityDistance : MaxVisibilityDistance - VisibilityDistanceHysteresis;
		// 락온 가능 거리는 락온 쪽이 정하므로 LockOn 대상에는 거리 조건을 두지 않는다.
		const bool bLockedOn = Target == LockOnActor;

		bool bVisible = false;
		if (ViewerPawn && (bLockedOn || Distance <= VisibleDistance))
		{
			TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
			const FGameplayTagContainer& OwnedTags = TargetASC ? TargetASC->GetOwnedGameplayTags() : FGameplayTagContainer::EmptyContainer;
			bVisible = bLockedOn ? !OwnedTags.HasAny(VisibilityRequirements.IgnoreTags) : VisibilityRequirements.RequirementsMet(OwnedTags);
		}

		if (!bVisible)
		{
			if (Entry)
			{
				if (Nameplate)
				{
					Nameplate->DestroyComponent();
				}
				Nameplates.Remove(Source);
			}
			continue;
		}

		if (!Nameplate)
		{
			USceneComponent* Root = Target->GetRootComponent();
			Nameplate = AttachWidget(Root, NameplateWidgetClass);
			if (!Nameplate)
			{
				continue;
			}
			Nameplates.Add(Source, Nameplate);

			// 기본 포즈 메시의 윗면을 쓴다. 애니메이션 바운드를 따르면 모션마다 Nameplate가 흔들린다.
			double Height = HeadClearance;
			const ACharacter* Character = Cast<ACharacter>(Target);
			const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
			if (const USkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr)
			{
				const FTransform MeshToRoot = Mesh->GetComponentTransform().GetRelativeTransform(Root->GetComponentTransform());
				Height += MeshAsset->GetImportedBounds().GetBox().TransformBy(MeshToRoot).Max.Z;
			}
			Nameplate->SetRelativeLocation(FVector(0.0, 0.0, Height));

			// 공유본의 수명은 이를 참조하는 MVVM View가 유지한다. 여기서 직접 초기화·해제하지 않는다.
			UUserWidget* Widget = Nameplate->GetWidget();
			UMVVMView* View = Widget ? Widget->GetExtension<UMVVMView>() : nullptr;
			if (!View || !TargetASC || !View->SetViewModelByClass(UWxViewModel_Character::GetOrCreate(TargetASC, Target)))
			{
				UE_LOG(LogWxUI, Warning, TEXT("Nameplate: Character 뷰모델을 연결하지 못했다. 위젯의 MVVM View·Manual 소스와 대상 ASC를 확인한다. Widget=%s Target=%s"), *GetNameSafe(Widget), *GetNameSafe(Target));
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
