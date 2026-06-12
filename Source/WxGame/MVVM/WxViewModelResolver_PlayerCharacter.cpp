// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_PlayerCharacter.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/WxPlayerCharacter.h"
#include "Component/WxAbilityComponent_UIData.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Character.h"

UObject* UWxViewModelResolver_PlayerCharacter::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	AWxPlayerCharacter* PlayerCharacter = PC ? Cast<AWxPlayerCharacter>(PC->GetPawn()) : nullptr;
	UAbilitySystemComponent* ASC = PlayerCharacter ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	// 위젯이 아닌 데이터 소스(빙의 캐릭터)를 Outer 로 생성한다. 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Character* ViewModel = NewObject<UWxViewModel_Character>(PlayerCharacter);
	ViewModel->Initialize(ASC, PlayerCharacter->GetCharacterUIData());

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = ViewModel->AbilitySystem;
	if (!AbilitySystemViewModel)
	{
		return ViewModel;
	}

	// WxUI는 WxCombat(어빌리티)에 의존할 수 없어 어빌리티를 직접 순회할 수 없으므로,
	// 어빌리티에 부착된 UIData 컴포넌트(WxUI) 의 아이콘은 양쪽에 의존하는 본 리졸버가 주입한다.
	// 어빌리티 VM 은 지연 생성되므로, 아이콘이 있는 어빌리티는 여기서 GetOrCreate 로 만들어 주입한다. 바인딩이 먼저 만들었다면 그 VM 을 재사용한다.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Spec.Ability);
		const UWxAbilityComponent_UIData* AbilityUIData = WxAbility ? WxAbility->FindComponent<UWxAbilityComponent_UIData>() : nullptr;
		if (!AbilityUIData || AbilityUIData->Icon.IsNull())
		{
			continue;
		}

		const FGameplayTagContainer& AssetTags = WxAbility->GetAssetTags();
		if (AssetTags.IsEmpty())
		{
			continue;
		}

		UWxViewModel_Ability* AbilityVM = AbilitySystemViewModel->GetOrCreateAbilityViewModel(AssetTags);
		if (!AbilityVM)
		{
			continue;
		}

		AbilityVM->SetIcon(AbilityUIData->Icon.LoadSynchronous());
	}

	return ViewModel;
}
