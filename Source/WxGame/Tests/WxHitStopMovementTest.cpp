// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Character/Component/WxCharacterMovementComponent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxHitStopComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxHitStopMovementTest, "Wx.Combat.HitStop.MovementContinuity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxHitStopMovementTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	ACharacter* Character = World->SpawnActor<ACharacter>();
	if (!TestNotNull(TEXT("Character"), Character))
	{
		World->DestroyWorld(false);
		return false;
	}
	Character->GetCharacterMovement()->Deactivate();
	Character->GetCharacterMovement()->SetComponentTickEnabled(true);
	UWxAbilitySystemComponent* ASC = NewObject<UWxAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Character, Character);
	UWxCharacterMovementComponent* Movement = NewObject<UWxCharacterMovementComponent>(Character);
	Movement->RegisterComponent();
	Movement->SetUpdatedComponent(Character->GetCapsuleComponent());
	Movement->bRunPhysicsWithNoController = true;
	Movement->SetMovementMode(MOVE_Falling);
	UWxHitStopComponent* HitStop = NewObject<UWxHitStopComponent>(Character);
	HitStop->RegisterComponent();
	HitStop->BeginPlay();

	const FVector InitialLocation = Character->GetActorLocation();
	const FVector LaunchVelocity(300.f, 0.f, 600.f);
	Movement->Velocity = LaunchVelocity;
	ASC->SetLooseGameplayTagCount(WxGameplayTags::Effect_HitStop, 2);
	TestTrue(TEXT("Hit stop keeps movement tick enabled"), Movement->IsComponentTickEnabled());
	TestTrue(TEXT("Hit stop does not disable actor movement tick"), Character->GetCharacterMovement()->IsComponentTickEnabled());
	// 방향키를 유지해도 정지 중 속도와 위치를 바꾸지 않는다.
	for (int32 Frame = 0; Frame < 12; ++Frame)
	{
		Character->AddMovementInput(FVector::ForwardVector);
		Movement->PerformMovement(1.f / 60.f);
	}
	TestEqual(TEXT("Frozen location"), Character->GetActorLocation(), InitialLocation);
	TestEqual(TEXT("Launch velocity preserved"), Movement->Velocity, LaunchVelocity);
	TestTrue(TEXT("Falling mode preserved"), Movement->IsFalling());
	ASC->SetLooseGameplayTagCount(WxGameplayTags::Effect_HitStop, 1);
	Movement->PerformMovement(1.f / 60.f);
	TestEqual(TEXT("Remaining hit keeps character frozen"), Character->GetActorLocation(), InitialLocation);
	ASC->SetLooseGameplayTagCount(WxGameplayTags::Effect_HitStop, 0);
	Movement->PerformMovement(1.f / 60.f);
	TestTrue(TEXT("Movement resumes after final hit expires"), Character->GetActorLocation().Z > InitialLocation.Z);
	TestTrue(TEXT("Movement tick remains enabled"), Movement->IsComponentTickEnabled());

	HitStop->EndPlay(EEndPlayReason::Destroyed);
	World->DestroyWorld(false);
	return true;
}

#endif
