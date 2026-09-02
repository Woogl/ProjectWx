// Copyright Woogle. All Rights Reserved.

#include "Finisher/WxFinisherDamageComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimNotify/WxAnimNotify_FinisherDamage.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"

void UWxFinisherDamageComponent::BeginFinisherDamage(const AActor* Target, const FDataTableRowHandle& DamageDataRow)
{
	PendingTarget = Target;
	PendingDamageDataRow = DamageDataRow;
}

void UWxFinisherDamageComponent::EndFinisherDamage(const AActor* Target)
{
	if (PendingTarget.Get() == Target)
	{
		PendingTarget.Reset();
		PendingDamageDataRow = FDataTableRowHandle();
	}
}

void UWxFinisherDamageComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	AbilitySystemComponent = ASC;
	FinisherDamageEventHandle = ASC->GenericGameplayEventCallbacks
		.FindOrAdd(WxGameplayTags::Event_ApplyFinisherDamage)
		.AddUObject(this, &UWxFinisherDamageComponent::HandleFinisherDamageEvent);
}

void UWxFinisherDamageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(WxGameplayTags::Event_ApplyFinisherDamage))
		{
			EventDelegate->Remove(FinisherDamageEventHandle);
		}
	}

	PendingTarget.Reset();
	PendingDamageDataRow = FDataTableRowHandle();
	AbilitySystemComponent.Reset();
	FinisherDamageEventHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxFinisherDamageComponent::HandleFinisherDamageEvent(const FGameplayEventData* Payload)
{
	AActor* Owner = GetOwner();
	const AActor* Target = PendingTarget.Get();
	if (!Owner || !Owner->HasAuthority() || !Target || !Payload)
	{
		return;
	}

	const UWxAnimNotify_FinisherDamage* FinisherDamageNotify = Cast<UWxAnimNotify_FinisherDamage>(Payload->OptionalObject.Get());
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Payload->OptionalObject2.Get());
	if (!FinisherDamageNotify || !Mesh || Mesh->GetOwner() != Owner)
	{
		return;
	}

	const FDataTableRowHandle DamageDataRow = PendingDamageDataRow;
	PendingTarget.Reset();
	PendingDamageDataRow = FDataTableRowHandle();

	FHitResult HitResult;
	HitResult.ImpactPoint = Target->GetActorLocation();
	HitResult.Location = Target->GetActorLocation();
	UWxCombatLibrary::ApplyDamage(Owner, Target, DamageDataRow, HitResult);
}
