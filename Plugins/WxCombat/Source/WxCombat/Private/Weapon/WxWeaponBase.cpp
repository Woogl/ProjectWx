// Copyright Woogle. All Rights Reserved.

#include "Weapon/WxWeaponBase.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxWeaponBase::AWxWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

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

AWxWeaponBase* AWxWeaponBase::FindWeapon(const AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		if (AWxWeaponBase* Weapon = Cast<AWxWeaponBase>(Attached))
		{
			return Weapon;
		}
	}

	return nullptr;
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
	
	const IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(WeaponOwner);
	if (OwnerTeam)
	{
		const ETeamAttitude::Type Attitude = OwnerTeam->GetTeamAttitudeTowards(*OtherActor);
		if (Attitude != ETeamAttitude::Hostile)
		{
			return;
		}
	}

	HitActorsThisSwing.Add(OtherActor);

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(WeaponOwner);

	if (TargetASC && SourceASC)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);
		Context.AddInstigator(WeaponOwner, WeaponOwner);
		Context.SetAbility(SourceASC->GetAnimatingAbility());

		FHitResult HitResult;
		if (bFromSweep)
		{
			HitResult = SweepResult;
		}
		else
		{
			FVector ClosestPoint;
			if (OtherComp->GetClosestPointOnCollision(HitCollision->GetComponentLocation(), ClosestPoint) >= 0.f)
			{
				HitResult.ImpactPoint = ClosestPoint;
				HitResult.Location = ClosestPoint;
			}
			else
			{
				HitResult.ImpactPoint = OtherComp->GetComponentLocation();
				HitResult.Location = OtherComp->GetComponentLocation();
			}
		}
		Context.AddHitResult(HitResult);

		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
		{
			if (!EffectClass)
			{
				continue;
			}

			const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
			if (Spec.IsValid())
			{
				Spec.Data->AppendDynamicAssetTags(AttackTags);
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}

		// 역경직: 공격자의 몽타주를 잠시 일시 정지
		if (HitStopDuration > 0.f)
		{
			FGameplayCueParameters HitStopParams;
			HitStopParams.RawMagnitude = HitStopDuration;
			SourceASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_HitStop, HitStopParams);
		}
	}
}
