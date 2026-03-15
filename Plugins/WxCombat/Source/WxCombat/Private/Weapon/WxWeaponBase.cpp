// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

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

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GripPoint);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	HitCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(GripPoint);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitCollision->SetCollisionObjectType(WxCollision::Attack);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitCollision->OnComponentBeginOverlap.AddDynamic(this, &AWxWeaponBase::HandleHitCollisionOverlap);
}

void AWxWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DetachFromCharacter();
	Super::EndPlay(EndPlayReason);
}

void AWxWeaponBase::AttachToCharacter(ACharacter* Character, FName SocketName)
{
	if (!Character)
	{
		return;
	}

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	SetOwner(Character);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character))
	{
		ASC->RegisterGameplayTagEvent(WxGameplayTags::ANS_WeaponCollision, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AWxWeaponBase::HandleWeaponCollisionTagChanged);
	}
}

void AWxWeaponBase::DetachFromCharacter()
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
		{
			ASC->RegisterGameplayTagEvent(WxGameplayTags::ANS_WeaponCollision, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);
		}
	}

	SetWeaponCollisionEnabled(false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

void AWxWeaponBase::SetWeaponCollisionEnabled(bool bEnabled)
{
	HitActorsThisSwing.Empty();
	HitCollision->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AWxWeaponBase::HandleWeaponCollisionTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	SetWeaponCollisionEnabled(NewCount > 0);
}

void AWxWeaponBase::HandleHitCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AActor* WeaponOwner = GetOwner();
	if (!OtherActor || OtherActor == WeaponOwner)
	{
		return;
	}
	if (HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

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
