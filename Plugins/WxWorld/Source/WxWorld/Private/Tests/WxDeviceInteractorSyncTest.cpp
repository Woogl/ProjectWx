// Copyright Woogle. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Device/WxDevice.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceInteractorSyncTest, "Wx.World.Device.OptionalInteractor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceInteractorSyncTest::RunTest(const FString& Parameters)
{
	UClass* DoorClass = LoadClass<AWxDevice>(nullptr, TEXT("/Game/WorldObject/Gimmick/BP_Door.BP_Door_C"));
	if (!TestNotNull(TEXT("실제 문 BP 로드"), DoorClass))
	{
		return false;
	}

	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AWxDevice* Door = World->SpawnActor<AWxDevice>(DoorClass);
	UWxDeviceStateTreeComponent* Component = Door ? Door->FindComponentByClass<UWxDeviceStateTreeComponent>() : nullptr;
	if (!TestNotNull(TEXT("문 상태 컴포넌트"), Component))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Door->SetRole(ROLE_SimulatedProxy);
	Component->StartLogic();
	const FGameplayTag OpenTag = FGameplayTag::RequestGameplayTag(TEXT("Device.Door.Open"));
	const FGameplayTag CloseTag = FGameplayTag::RequestGameplayTag(TEXT("Device.Door.Close"));
	TestTrue(TEXT("문 열림 상태 존재"), Component->HasState(OpenTag));
	TestTrue(TEXT("문 닫힘 상태 존재"), Component->HasState(CloseTag));

	ACharacter* PreviousInteractor = World->SpawnActor<ACharacter>();
	Door->InteractingCharacter = PreviousInteractor;
	FWxDeviceStateSnapshot Previous = Component->StateSnapshot;
	Component->StateSnapshot.EntrySerial = 1;
	Component->StateSnapshot.StateTagName = OpenTag.GetTagName();
	Component->StateSnapshot.RunStatus = EStateTreeRunStatus::Running;
	Component->StateSnapshot.Interactor = nullptr;
	Component->OnRep_StateSnapshot(Previous);
	TestNull(TEXT("미해소 참조 수신 시 이전 당사자 제거"), Door->GetInteractingCharacter());
	for (int32 Tick = 0; Tick < 8 && Component->AppliedEntrySerial != 1; ++Tick)
	{
		Component->TickComponent(0.1f, LEVELTICK_All, nullptr);
	}
	TestEqual(TEXT("당사자 없이 최초 스냅샷 적용"), Component->AppliedEntrySerial, 1u);
	TestEqual(TEXT("실제 트리가 열린 상태에 수렴"), Component->LastEnteredTag, OpenTag);
	TestTrue(TEXT("동기화 오류 없음"), Component->SyncFailure.IsEmpty());

	const uint32 AppliedLocalEntry = Component->LocalEntrySerial;
	const uint8 AppliedAttempts = Component->SyncAttempts;
	Previous = Component->StateSnapshot;
	Component->StateSnapshot.Interactor = PreviousInteractor;
	Component->OnRep_StateSnapshot(Previous);
	TestEqual(TEXT("늦게 해소된 당사자 반영"), Door->GetInteractingCharacter(), PreviousInteractor);
	TestEqual(TEXT("참조 해소가 상태 재진입을 만들지 않음"), Component->LocalEntrySerial, AppliedLocalEntry);
	TestEqual(TEXT("참조 해소가 재요청을 만들지 않음"), Component->SyncAttempts, AppliedAttempts);
	TestFalse(TEXT("참조 해소 후 대기 중 전이 없음"), Component->bRequestPending);

	PreviousInteractor->Destroy();
	TestFalse(TEXT("파괴 후 GC 전 당사자 무효"), IsValid(PreviousInteractor));
	Previous = Component->StateSnapshot;
	Component->StateSnapshot.EntrySerial = 2;
	Component->StateSnapshot.StateTagName = CloseTag.GetTagName();
	Component->OnRep_StateSnapshot(Previous);
	TestNull(TEXT("파괴된 당사자 적용 차단"), Door->GetInteractingCharacter());
	for (int32 Tick = 0; Tick < 8 && Component->AppliedEntrySerial != 2; ++Tick)
	{
		Component->TickComponent(0.1f, LEVELTICK_All, nullptr);
	}
	TestEqual(TEXT("파괴된 참조가 있어도 다음 상태 적용"), Component->AppliedEntrySerial, 2u);
	TestEqual(TEXT("실제 트리가 닫힌 상태에 수렴"), Component->LastEnteredTag, CloseTag);

	Previous = Component->StateSnapshot;
	Component->StateSnapshot.Interactor = nullptr;
	Component->StateSnapshot.RunStatus = EStateTreeRunStatus::Succeeded;
	Component->OnRep_StateSnapshot(Previous);
	TestFalse(TEXT("당사자 없이 완료 스냅샷 적용"), Component->IsRunning());
	const uint32 CompletedLocalEntry = Component->LocalEntrySerial;
	ACharacter* LateInteractor = World->SpawnActor<ACharacter>();
	Previous = Component->StateSnapshot;
	Component->StateSnapshot.Interactor = LateInteractor;
	Component->OnRep_StateSnapshot(Previous);
	TestEqual(TEXT("완료 후에도 늦은 참조 반영"), Door->GetInteractingCharacter(), LateInteractor);
	TestFalse(TEXT("참조 해소가 완료한 트리를 재시작하지 않음"), Component->IsRunning());
	TestEqual(TEXT("완료 후 중복 진입 없음"), Component->LocalEntrySerial, CompletedLocalEntry);

	Door->SetRole(ROLE_Authority);
	Door->InteractingCharacter = PreviousInteractor;
	++Component->LocalEntrySerial;
	Component->PublishAuthorityState();
	TestNull(TEXT("서버는 파괴된 당사자를 null로 발행"), Component->StateSnapshot.Interactor.Get());
	Door->InteractingCharacter = LateInteractor;
	++Component->LocalEntrySerial;
	Component->PublishAuthorityState();
	TestEqual(TEXT("서버는 유효 당사자를 유지"), Component->StateSnapshot.Interactor.Get(), LateInteractor);

	Component->StopLogic(TEXT("Automation cleanup"));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
