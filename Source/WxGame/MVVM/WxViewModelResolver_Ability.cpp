// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Ability.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"

UObject* UWxViewModelResolver_Ability::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	const AWxPlayerCharacter* PlayerCharacter = PC ? Cast<AWxPlayerCharacter>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;

	// 어빌리티 매칭과 인스턴스 소유는 ASC 의 어빌리티시스템 VM 이 맡는다 — 같은 어빌리티를 보는 슬롯끼리 하나를 나눠 쓴다.
	UWxViewModel_AbilitySystem* AbilitySystemViewModel = UWxViewModel_AbilitySystem::GetOrCreate(ASC);
	UWxViewModel_Ability* ViewModel = AbilitySystemViewModel ? AbilitySystemViewModel->GetOrCreateAbilityViewModel(AbilityTags) : nullptr;
	if (!ViewModel)
	{
		return nullptr;
	}

	// 아이콘은 WxUI 가 어빌리티에서 읽지 못해 여기서 채운다. 공유본이라 앞서 채워졌으면 다시 스트리밍하지 않는다.
	if (!ViewModel->GetIcon())
	{
		if (const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(ViewModel->GetBoundAbility()))
		{
			// 소프트 참조를 그대로 넘겨 VM이 비동기 스트리밍한다(동기 로드하지 않는다).
			ViewModel->SetIconSoft(Ability->GetIcon());
		}
	}

	return ViewModel;
}
