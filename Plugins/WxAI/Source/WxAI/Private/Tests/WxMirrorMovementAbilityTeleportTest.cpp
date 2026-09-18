// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WxBTService_MirrorMovement.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxMirrorMovementAbilityTeleportTest, "Wx.AI.MirrorMovement.AbilityTeleport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxMirrorMovementAbilityTeleportTest::RunTest(const FString& Parameters)
{
	UClass* SkillClass = LoadClass<UGameplayAbility>(nullptr, TEXT("/Game/Character/HGTest/Abilities/Skill_3/GA_HGTest_Skill_3.GA_HGTest_Skill_3_C"));
	if (!TestNotNull(TEXT("Skill_3 asset"), SkillClass)) { return false; }
	UGameplayAbility* Skill = SkillClass->GetDefaultObject<UGameplayAbility>();
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	ACharacter* Master = World->SpawnActor<ACharacter>(FVector(1000.f, 2000.f, 1000.f), FRotator(0.f, 90.f, 0.f));
	ACharacter* Follower = World->SpawnActor<ACharacter>(FVector(0.f, 0.f, 1000.f), FRotator::ZeroRotator);
	AAIController* Controller = World->SpawnActor<AAIController>();
	Controller->Possess(Follower);
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Follower);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Follower, Follower);
	UBlackboardData* Blackboard = NewObject<UBlackboardData>();
	FBlackboardEntry Entry;
	Entry.EntryName = TEXT("Master");
	Entry.KeyType = NewObject<UBlackboardKeyType_Object>(Blackboard);
	Blackboard->Keys.Add(Entry);
	UBlackboardComponent* BB = nullptr;
	Controller->UseBlackboard(Blackboard, BB);
	BB->SetValueAsObject(TEXT("Master"), Master);
	UBehaviorTreeComponent* BT = NewObject<UBehaviorTreeComponent>(Controller);
	BT->RegisterComponent();
	UWxBTService_MirrorMovement* Service = NewObject<UWxBTService_MirrorMovement>(BT);
	TArray<uint8> Memory;
	Memory.SetNumZeroed(Service->GetSpecialMemorySize() + Service->GetInstanceMemorySize() + 16);
	uint8* NodeMemory = Memory.GetData() + Service->GetSpecialMemorySize();
	Service->Master = Master;
	Service->Follower = Follower;
	Service->FollowerAbilitySystem = ASC;
	const FVector Before = Follower->GetActorLocation();
	Service->HandleAbilityActivated(Skill);
	TestTrue(TEXT("Unconfigured service does not teleport"), Follower->GetActorLocation().Equals(Before));
	Service->FaceMasterAbilityTags.AddTag(WxGameplayTags::Ability_Skill_3);
	Follower->GetCharacterMovement()->Velocity = FVector(100.f, 0.f, 0.f);
	Service->HandleAbilityActivated(Skill);
	const FVector Expected = Master->GetActorLocation() + FVector(0.f, 500.f, 0.f);
	TestTrue(TEXT("Teleport 500cm along master yaw"), Follower->GetActorLocation().Equals(Expected, 0.1f));
	TestTrue(TEXT("Face master before skill body"), Follower->GetActorForwardVector().Equals(FVector(0.f, -1.f, 0.f), 0.001f));
	TestTrue(TEXT("Clear preceding movement velocity"), Follower->GetVelocity().IsNearlyZero());
	ASC->AddLooseGameplayTag(WxGameplayTags::Ability_Skill_3);
	Service->TickNode(*BT, NodeMemory, 0.1f);
	TestTrue(TEXT("Active skill retains position"), Follower->GetActorLocation().Equals(Expected, 0.1f));
	TestTrue(TEXT("Active skill retains facing"), Follower->GetActorForwardVector().Equals(FVector(0.f, -1.f, 0.f), 0.001f));
	TestTrue(TEXT("Active skill has no follow input"), Follower->GetPendingMovementInputVector().IsNearlyZero());
	ASC->RemoveLooseGameplayTag(WxGameplayTags::Ability_Skill_3);
	Service->HandleAbilityEnded(FAbilityEndedData());
	Service->TickNode(*BT, NodeMemory, 0.1f);
	const FVector FollowDestination = Master->GetActorLocation() + Master->GetActorRotation().RotateVector(Service->LocalOffset);
	TestTrue(TEXT("Ended skill returns to follow offset"), Follower->GetActorLocation().Equals(FollowDestination, 0.1f));
	TestTrue(TEXT("Ended skill follows master rotation"), Follower->GetActorForwardVector().Equals(Master->GetActorForwardVector(), 0.001f));
	Service->Release(*BT);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
