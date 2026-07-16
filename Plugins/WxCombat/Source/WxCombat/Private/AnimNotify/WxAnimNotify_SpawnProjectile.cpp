// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SpawnProjectile.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxDamageTableRow.h"

void UWxAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	// 몽타주를 재생 중인 어빌리티(서버 인스턴스)가 확정 스폰한다. 클라에선 authority 게이트로 무동작.
	if (UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ASC->GetAnimatingAbility()))
	{
		Ability->SpawnProjectile(ProjectileClass, SpawnSocketName, ResolveDamageInfo());
	}
}

#if WITH_EDITOR
bool UWxAnimNotify_SpawnProjectile::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	if (DamageDataRow.DataTable != nullptr && InProperty->GetOwnerStruct() == FWxDamageInfo::StaticStruct())
	{
		return false;
	}

	return true;
}
#endif

FWxDamageInfo UWxAnimNotify_SpawnProjectile::ResolveDamageInfo() const
{
	if (const FWxDamageTableRow* Row = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("WxAnimNotify_SpawnProjectile")))
	{
		FWxDamageInfo Resolved;
		Resolved.ApplyTableRow(*Row);
		return Resolved;
	}

	return DamageInfo;
}
