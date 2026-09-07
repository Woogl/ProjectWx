// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/SlateWrapperTypes.h"
#include "MVVMEditorSubsystem.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewConversionFunction.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Editor.h"
#include "UObject/StructOnScope.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxAttackSlotVisibilityTest, "Wx.MVVM.Ability.SlotVisibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxAttackSlotVisibilityTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Widget/WBP_PlayerSkills.WBP_PlayerSkills"));
	if (!TestNotNull(TEXT("Player skills"), BP)) return false;
	TestNull(TEXT("Removed custom attack input wrapper"), BP->WidgetTree->FindWidget(TEXT("AttackSlot")));
	if (!TestNotNull(TEXT("Original attack icon overlay"), BP->WidgetTree->FindWidget(TEXT("AttackVariants")))) return false;
	UMVVMBlueprintView* View = GEditor->GetEditorSubsystem<UMVVMEditorSubsystem>()->GetView(BP);
	if (!TestNotNull(TEXT("Actual HUD view bindings"), View)) return false;
	for (const FMVVMBlueprintViewBinding& Binding : View->GetBindings())
	{
		TestTrue(TEXT("No binding targets the removed input"), Binding.DestinationPath.GetWidgetName() != TEXT("AttackSlot"));
	}
	UObject* Widget = NewObject<UObject>(GetTransientPackage(), BP->GeneratedClass);
	UWxViewModel_Character* Character = NewObject<UWxViewModel_Character>();
	Character->AbilitySystem = NewObject<UWxViewModel_AbilitySystem>();
	FObjectPropertyBase* CharacterProperty = FindFProperty<FObjectPropertyBase>(BP->GeneratedClass, TEXT("VM_PlayerCharacter"));
	if (!TestNotNull(TEXT("Runtime character field"), CharacterProperty)) return false;
	CharacterProperty->SetObjectPropertyValue_InContainer(Widget, Character);
	TArray<UWxViewModel_Ability*> Abilities;
	for (const FName Name : { FName("WxViewModel_Ability_AttackLight"), FName("WxViewModel_Ability_AttackAir"), FName("WxViewModel_Ability_AttackHeavy") })
	{
		FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(BP->GeneratedClass, Name);
		if (!TestNotNull(TEXT("Runtime ability field"), Property)) return false;
		UWxViewModel_Ability* Ability = NewObject<UWxViewModel_Ability>();
		Abilities.Add(Ability);
		Property->SetObjectPropertyValue_InContainer(Widget, Ability);
	}
	for (const bool bInAir : { false, true, false })
	{
		Character->AbilitySystem->OwnedTags.Reset();
		if (bInAir) Character->AbilitySystem->OwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Movement.InAir")));
		for (const bool bCanActivate : { true, false })
		{
			for (UWxViewModel_Ability* Ability : Abilities) Ability->SetCanActivate(bCanActivate);
			int32 Displayed = 0;
			int32 Evaluated = 0;
			for (const FMVVMBlueprintViewBinding& Binding : View->GetBindings())
			{
				const FName Name = Binding.DestinationPath.GetWidgetName();
				if (Name != TEXT("Attack_Light") && Name != TEXT("Attack_Air") && Name != TEXT("Attack_Heavy")) continue;
				if (!Binding.Conversion.SourceToDestinationConversion) continue;
				UFunction* Function = const_cast<UFunction*>(Binding.Conversion.SourceToDestinationConversion->GetCompiledFunction(BP->GeneratedClass));
				if (!TestNotNull(TEXT("Actual compiled conversion"), Function)) return false;
				if (!TestNotNull(TEXT("Visibility return value"), Function->GetReturnProperty())) return false;
				if (!TestEqual(TEXT("Wrapper reads its sources from the actual widget"), static_cast<int32>(Function->NumParms), 1)) return false;
				FStructOnScope Params(Function);
				Widget->ProcessEvent(Function, Params.GetStructMemory());
				const ESlateVisibility Visibility = *Function->GetReturnProperty()->ContainerPtrToValuePtr<ESlateVisibility>(Params.GetStructMemory());
				const ESlateVisibility Expected = ((Name == TEXT("Attack_Air")) == bInAir) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
				TestEqual(FString::Printf(TEXT("%s InAir=%d CanActivate=%d"), *Name.ToString(), bInAir, bCanActivate), Visibility, Expected);
				Displayed += Visibility != ESlateVisibility::Collapsed;
				++Evaluated;
			}
			TestEqual(TEXT("Evaluated all actual attack bindings"), Evaluated, 3);
			TestTrue(TEXT("Slot always has a displayed attack, including when activation is blocked"), Displayed > 0);
		}
	}
	return true;
}

#endif

