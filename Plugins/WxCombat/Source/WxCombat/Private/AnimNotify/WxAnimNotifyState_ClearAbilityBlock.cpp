// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_ClearAbilityBlock.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxGameplayTags.h"

void UWxAnimNotifyState_ClearAbilityBlock::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->AddLooseGameplayTag(WxGameplayTags::ANS_CancelWindow);

			// 후딜 어빌리티가 걸어 둔 하드 차단을 해제해 다른 어빌리티가 진입할 수 있게 한다.
			// 스냅샷으로 복사 후 해제한다(UnBlock이 BlockedAbilityTags를 변경하므로).
			const FGameplayTagContainer Blocked = ASC->GetBlockedAbilityTags();
			ASC->UnBlockAbilitiesWithTags(Blocked);
		}
	}
}

void UWxAnimNotifyState_ClearAbilityBlock::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// 차단은 의도적으로 복구하지 않는다(헤더 주석 참고). 관찰용 신호 태그만 내린다.
	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_CancelWindow);
		}
	}
}
