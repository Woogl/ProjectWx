// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "WxGameplayTags.h"

UWxAbilityTask_LockOnTarget* UWxAbilityTask_LockOnTarget::CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed, float InPitchOffset, float InMaxDistance, TSubclassOf<UUserWidget> InReticleWidgetClass)
{
	UWxAbilityTask_LockOnTarget* Task = NewAbilityTask<UWxAbilityTask_LockOnTarget>(OwningAbility);
	Task->Target = InTarget;
	Task->InterpSpeed = InInterpSpeed;
	Task->PitchOffset = InPitchOffset;
	Task->MaxDistanceSquared = InMaxDistance * InMaxDistance;
	Task->ReticleWidgetClass = InReticleWidgetClass;
	Task->bTickingTask = true;
	return Task;
}

void UWxAbilityTask_LockOnTarget::Activate()
{
	Super::Activate();

	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.AddDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged);
		}

		CreateReticleWidget();
	}
}

void UWxAbilityTask_LockOnTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APawn* AvatarPawn = Cast<APawn>(GetAvatarActor());
	if (!AvatarPawn)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();

	// 거리 초과 시 락온 해제
	const float DistanceSquared = FVector::DistSquared(AvatarPawn->GetActorLocation(), TargetLocation);
	if (DistanceSquared > MaxDistanceSquared)
	{
		OnTargetLost.Broadcast();
		return;
	}

	APlayerController* PC = Cast<APlayerController>(AvatarPawn->GetController());
	if (!PC)
	{
		return;
	}

	FRotator DesiredRotation = (TargetLocation - AvatarPawn->GetActorLocation()).Rotation();
	DesiredRotation.Pitch += PitchOffset;

	const FRotator CurrentRotation = PC->GetControlRotation();
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, InterpSpeed);

	PC->SetControlRotation(NewRotation);
}

void UWxAbilityTask_LockOnTarget::OnDestroy(bool bInOwnerFinished)
{
	DestroyReticleWidget();

	if (AActor* TargetActor = Target.Get())
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &UWxAbilityTask_LockOnTarget::HandleTargetDestroyed);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			TargetASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UWxAbilityTask_LockOnTarget::HandleTargetDestroyed(AActor* DestroyedActor)
{
	OnTargetLost.Broadcast();
}

void UWxAbilityTask_LockOnTarget::HandleTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		OnTargetLost.Broadcast();
	}
}

void UWxAbilityTask_LockOnTarget::CreateReticleWidget()
{
	AActor* TargetActor = Target.Get();
	if (!TargetActor || !ReticleWidgetClass)
	{
		return;
	}

	ReticleWidgetComponent = NewObject<UWidgetComponent>(TargetActor);
	ReticleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ReticleWidgetComponent->SetWidgetClass(ReticleWidgetClass);
	ReticleWidgetComponent->SetDrawAtDesiredSize(true);
	ReticleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReticleWidgetComponent->RegisterComponent();
	ReticleWidgetComponent->AttachToComponent(TargetActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UWxAbilityTask_LockOnTarget::DestroyReticleWidget()
{
	if (ReticleWidgetComponent)
	{
		ReticleWidgetComponent->DestroyComponent();
		ReticleWidgetComponent = nullptr;
	}
}
