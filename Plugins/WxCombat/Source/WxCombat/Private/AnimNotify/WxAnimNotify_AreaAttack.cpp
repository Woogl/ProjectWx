// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_AreaAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"

void UWxAnimNotify_AreaAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !TargetingPreset)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(Owner->GetWorld());
	if (!TargetingSubsystem)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!SourceASC)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = Owner;
	SourceContext.InstigatorActor = Owner;
	if (!SourceSocketName.IsNone())
	{
		SourceContext.SourceLocation = MeshComp->GetSocketLocation(SourceSocketName);
	}

	FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);

	TArray<AActor*> TargetActors;
	TargetingSubsystem->GetTargetingResultsActors(Handle, TargetActors);
	TargetingSubsystem->ReleaseTargetRequestHandle(Handle);

	if (TargetActors.IsEmpty())
	{
		return;
	}
	
	APawn* OwnerPawn = Cast<APawn>(Owner);

	for (AActor* TargetActor : TargetActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
		{
			continue;
		}

		// HitReact 위치 등을 위해 ImpactPoint 를 타겟 위치로 채운다.
		// 각 타겟마다 컨텍스트가 달라야 하므로 루프 안에서 컨텍스트와 Spec 을 새로 생성한다.
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(Owner);
		Context.AddInstigator(OwnerPawn ? static_cast<AActor*>(OwnerPawn) : Owner, Owner);
		Context.SetAbility(SourceASC->GetAnimatingAbility());

		FHitResult HitResult;
		HitResult.ImpactPoint = TargetActor->GetActorLocation();
		HitResult.Location = TargetActor->GetActorLocation();
		Context.AddHitResult(HitResult, true);

		FWxDamageInfo DamageInfo = FWxDamageInfo::FromDataRow(DamageDataRow);
		const TArray<FGameplayEffectSpecHandle> Specs = DamageInfo.MakeSpecs(SourceASC, Context);
		for (const FGameplayEffectSpecHandle& SpecHandle : Specs)
		{
			if (SpecHandle.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}
