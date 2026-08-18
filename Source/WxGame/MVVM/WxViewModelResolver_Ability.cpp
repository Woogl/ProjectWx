// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const AWxPlayerCharacter* PlayerCharacter = PC ? Cast<AWxPlayerCharacter>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;

	// 빈 컨테이너는 HasAll 이 항상 true 라 아무 어빌리티나 매칭되므로 거부한다.
	if (!ASC || AbilityTags.IsEmpty())
	{
		return nullptr;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
		if (!Ability || !Ability->GetAssetTags().HasAll(AbilityTags))
		{
			continue;
		}

		// 위젯이 아닌 데이터 소스(ASC)를 Outer 로 생성한다.
		// 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
		UWxViewModel_Ability* ViewModel = NewObject<UWxViewModel_Ability>(ASC);
		ViewModel->Initialize(ASC, Ability);

		// 소프트 참조를 그대로 넘겨 VM이 비동기 스트리밍한다(동기 로드하지 않는다).
		ViewModel->SetIconSoft(Ability->GetIcon());
		return ViewModel;
	}

	return nullptr;
}
