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
	// 뒤잡 즉사/앞잡 계수 분기는 어빌리티가 현재 재생 몽타주로 판단한다.
	if (UWxAbility_Finisher* Finisher = Cast<UWxAbility_Finisher>(ASC->GetAnimatingAbility()))
	{
		Finisher->ApplyFinisherDamage(ResolveDamageInfo());
	}
}

FWxDamageInfo UWxAnimNotify_FinisherDamage::ResolveDamageInfo() const
{
	FWxDamageInfo Resolved;
	if (const FWxDamageTableRow* Row = DamageDataRow.GetRow<FWxDamageTableRow>(TEXT("WxAnimNotify_FinisherDamage")))
	{
		Resolved.ApplyTableRow(*Row);
	}
	return Resolved;
}
