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

	// 몽타주를 재생 중인 처형 어빌리티(서버 인스턴스)가 확정 대상에 적용한다. 클라에선 캐스팅 실패로 무동작.
	// 앞잡·뒤잡 모두 이 행의 계수 피해를 쓰므로 어빌리티 쪽에 변형 분기가 없다.
	if (UWxAbility_Finisher* Finisher = Cast<UWxAbility_Finisher>(ASC->GetAnimatingAbility()))
	{
		Finisher->ApplyFinisherDamage(FWxDamageInfo::FromDataRow(DamageDataRow));
	}
}
