// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_FinisherDamage.h"
#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "WxDamageTableRow.h"

void UWxAnimNotify_FinisherDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

	// 몽타주를 재생 중인 처형 어빌리티(서버 인스턴스)가 확정 대상에 적용한다. 클라에선 캐스팅 실패로 무동작.
	if (UWxAbility_Finisher* Finisher = Cast<UWxAbility_Finisher>(ASC->GetAnimatingAbility()))
	{
		Finisher->ApplyFinisherDamage(ResolveDamageInfo());
	}
}

#if WITH_EDITOR
bool UWxAnimNotify_FinisherDamage::CanEditChange(const FProperty* InProperty) const
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

FWxDamageInfo UWxAnimNotify_FinisherDamage::ResolveDamageInfo() const
{
	if (const FWxDamageTableRow* Row = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("WxAnimNotify_FinisherDamage")))
	{
		FWxDamageInfo Resolved;
		Resolved.ApplyTableRow(*Row);
		return Resolved;
	}

	return DamageInfo;
}
