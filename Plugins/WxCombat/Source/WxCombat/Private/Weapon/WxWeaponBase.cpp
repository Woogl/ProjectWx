// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Animation/AnimInstance.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

AWxWeaponBase::AWxWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
	SetRootComponent(GripPoint);

#if WITH_EDITORONLY_DATA
	GripArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("GripArrow"));
	if (GripArrow)
	{
		GripArrow->SetupAttachment(GripPoint);
		GripArrow->SetArrowColor(FLinearColor::Red);
	}
#endif

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GripPoint);

	HitCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(GripPoint);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWxWeaponBase::HandleHitCollisionOverlap);
}

void AWxWeaponBase::AttachToCharacter(ACharacter* Character, FName SocketName)
{
	if (!Character) return;

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	SetOwner(Character);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWxWeaponBase::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

void AWxWeaponBase::SetWeaponCollisionEnabled(bool bEnabled)
{
	HitActorsThisSwing.Empty();
	HitCollision->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	BindMontageEndedCallback(bEnabled);
}

void AWxWeaponBase::BindMontageEndedCallback(bool bBind)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	AnimInstance->OnMontageEnded.RemoveDynamic(this, &AWxWeaponBase::HandleOwnerMontageEnded);
	if (bBind)
	{
		AnimInstance->OnMontageEnded.AddDynamic(this, &AWxWeaponBase::HandleOwnerMontageEnded);
	}
}

void AWxWeaponBase::HandleOwnerMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	SetWeaponCollisionEnabled(false);
}

void AWxWeaponBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner) return;
	if (HitActorsThisSwing.Contains(OtherActor)) return;

	HitActorsThisSwing.Add(OtherActor);

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);

	if (TargetASC && SourceASC && DamageEffectClass)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}
}
