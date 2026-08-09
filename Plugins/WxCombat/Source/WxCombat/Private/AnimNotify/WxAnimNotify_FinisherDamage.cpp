// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_FinisherDamage.h"
#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

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

	// 클라에도 인스턴스가 있어 이 캐스팅은 성공한다.
	// 다만 처형은 ServerInitiated라 예측 키가 서버 발행 키여서, 엔진 권위 검사에 걸러져 GE 적용은 서버에서만 성립한다.
	if (UWxAbility_Finisher* Finisher = Cast<UWxAbility_Finisher>(ASC->GetAnimatingAbility()))
	{
		Finisher->ApplyFinisherDamage(FWxDamageInfo::FromDataRow(DamageDataRow));
	}
}
