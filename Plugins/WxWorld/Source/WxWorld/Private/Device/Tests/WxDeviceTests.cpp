// Copyright Woogle. All Rights Reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Device/Tests/WxDeviceTestTypes.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "Components/StateTreeComponentSchema.h"
#include "Conditions/StateTreeCommonConditions.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interaction/WxStateTreeTask_EnableInteraction.h"
#include "Misc/AutomationTest.h"
#include "Net/RepLayout.h"
#include "StateTree.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "WxGameplayTags.h"
#include "WxWorldModule.h"

struct FWxDeviceTestAccess
{
	static const FWxDeviceStateSnapshot& Snapshot(const UWxDeviceStateTreeComponent& Component);
	static void Receive(UWxDeviceStateTreeComponent& Component, const FWxDeviceStateSnapshot& Snapshot);
	static void SetInitialTarget(UWxDeviceStateTreeComponent& Component, FGameplayTag Tag);
	static uint32 LocalEntries(const UWxDeviceStateTreeComponent& Component);
	static FGameplayTag LocalTag(const UWxDeviceStateTreeComponent& Component);
	static bool HasFailed(const UWxDeviceStateTreeComponent& Component);
};

const FWxDeviceStateSnapshot& FWxDeviceTestAccess::Snapshot(const UWxDeviceStateTreeComponent& Component)
{
	return Component.StateSnapshot;
}

void FWxDeviceTestAccess::Receive(UWxDeviceStateTreeComponent& Component, const FWxDeviceStateSnapshot& Snapshot)
{
	const FWxDeviceStateSnapshot Previous = Component.StateSnapshot;
	Component.StateSnapshot = Snapshot;
	Component.OnRep_StateSnapshot(Previous);
}

void FWxDeviceTestAccess::SetInitialTarget(UWxDeviceStateTreeComponent& Component, FGameplayTag Tag)
{
	Component.InitialTarget = Tag;
}

uint32 FWxDeviceTestAccess::LocalEntries(const UWxDeviceStateTreeComponent& Component)
{
	return Component.LocalEntrySerial;
}

FGameplayTag FWxDeviceTestAccess::LocalTag(const UWxDeviceStateTreeComponent& Component)
{
	return Component.LastEnteredTag;
}

bool FWxDeviceTestAccess::HasFailed(const UWxDeviceStateTreeComponent& Component)
{
	return !Component.SyncFailure.IsEmpty();
}

namespace WxDeviceTests
{
	struct FWxFixture
	{
		FWxFixture();
		~FWxFixture();
		bool Start(FAutomationTestBase& Test, bool bClient = false, FGameplayTag InitialTag = FGameplayTag());
		void Tick(float DeltaTime = 0.1f);
		void AddDelegateTransition(bool bReject, bool bDelay);

		UWorld* World = nullptr;
		UStateTree* Tree = nullptr;
		UStateTreeEditorData* EditorData = nullptr;
		UStateTreeState* Root = nullptr;
		UStateTreeState* Idle = nullptr;
		UStateTreeState* Target = nullptr;
		AWxDeviceTestActor* Device = nullptr;
		UWxDeviceStateTreeComponent* Component = nullptr;
	};

	FWxFixture::FWxFixture()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).ShouldSimulatePhysics(false).SetTransactional(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
		Tree = NewObject<UStateTree>(World);
		EditorData = NewObject<UStateTreeEditorData>(Tree);
		Tree->EditorData = EditorData;
		EditorData->Schema = NewObject<UStateTreeComponentSchema>(EditorData);
		Root = &EditorData->AddSubTree(TEXT("Root"));
		Idle = &Root->AddChildState(TEXT("Idle"));
		Idle->Tag = WxGameplayTags::Device_Button_Idle;
		Idle->AddTask<FWxDeviceTestTask>();
		Target = &Root->AddChildState(TEXT("Target"));
		Target->Tag = WxGameplayTags::Device_Button_Pressed;
		Target->AddTask<FWxDeviceTestTask>();
	}

	FWxFixture::~FWxFixture()
	{
		if (Component)
		{
			Component->StopLogic(TEXT("Test cleanup"));
		}
		World->DestroyWorld(false);
	}

	bool FWxFixture::Start(FAutomationTestBase& Test, bool bClient, FGameplayTag InitialTag)
	{
		FStateTreeCompilerLog Log;
		FStateTreeCompiler Compiler(Log);
		if (!Test.TestTrue(TEXT("Runtime fixture compiles"), Compiler.Compile(*Tree)))
		{
			Log.DumpToLog(LogWxWorld);
			return false;
		}
		Device = World->SpawnActor<AWxDeviceTestActor>();
		if (!Test.TestNotNull(TEXT("Device spawned"), Device))
		{
			return false;
		}
		Component = Device->FindComponentByClass<UWxDeviceStateTreeComponent>();
		Component->SetStateTree(Tree);
		if (bClient)
		{
			Device->SetRole(ROLE_SimulatedProxy);
		}
		FWxDeviceTestAccess::SetInitialTarget(*Component, InitialTag);
		Component->StartLogic();
		return Test.TestTrue(TEXT("Fixture started"), Component->IsRunning());
	}

	void FWxFixture::Tick(float DeltaTime)
	{
		if (Component->IsComponentTickEnabled())
		{
			Component->TickComponent(DeltaTime, LEVELTICK_All, &Component->PrimaryComponentTick);
		}
	}

	void FWxFixture::AddDelegateTransition(bool bReject, bool bDelay)
	{
		auto& Enable = Idle->AddTask<FWxStateTreeTask_EnableInteraction>();
		Enable.GetInstanceData().bEnable = true;
		FStateTreeTransition& Transition = Idle->AddTransition(EStateTreeTransitionTrigger::OnDelegate,
			EStateTreeTransitionType::GotoState, Idle);
		Transition.bDelayTransition = bDelay;
		Transition.DelayDuration = 1.0f;
		Transition.DelayRandomVariance = 0.0f;
		if (bReject)
		{
			auto& Condition = Transition.AddConditionWithOuter<FStateTreeCompareIntCondition>(Idle);
			Condition.GetInstanceData().Left = 0;
			Condition.GetInstanceData().Right = 1;
			EditorData->AddPropertyBinding(
				FPropertyBindingPath(Idle->Tasks[0].ID, GET_MEMBER_NAME_CHECKED(FWxDeviceTestTaskInstanceData, Value)),
				FPropertyBindingPath(Condition.ID, GET_MEMBER_NAME_CHECKED(FStateTreeCompareIntConditionInstanceData, Left)));
		}
		EditorData->AddPropertyBinding(FPropertyBindingPath(Enable.ID,
			GET_MEMBER_NAME_CHECKED(FWxStateTreeTask_EnableInteractionInstanceData, OnInteracted)),
			FPropertyBindingPath(Transition.ID, GET_MEMBER_NAME_CHECKED(FStateTreeTransition, DelegateListener)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceDelegateTest, "Wx.Device.DelegateReentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceDelegateTest::RunTest(const FString& Parameters)
{
	for (int32 Scenario = 0; Scenario < 3; ++Scenario)
	{
		WxDeviceTests::FWxFixture Fixture;
		Fixture.AddDelegateTransition(Scenario == 0, Scenario == 1);
		if (!Fixture.Start(*this))
		{
			return false;
		}
		const uint32 Before = FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial;
		Fixture.Device->OnInteracted(nullptr);
		Fixture.Tick();
		TestEqual(TEXT("Only applied transitions advance the replicated entry"),
			FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial, Before + (Scenario == 2 ? 1u : 0u));
		if (Scenario == 1)
		{
			Fixture.Tick(1.1f);
			TestEqual(TEXT("Delayed transition eventually publishes exactly one reentry"),
				FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial, Before + 1u);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceEventTest, "Wx.Device.EventReentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceEventTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	Fixture.Idle->AddTransition(EStateTreeTransitionTrigger::OnEvent, WxGameplayTags::Event_Device_Triggered,
		EStateTreeTransitionType::GotoState, Fixture.Idle);
	if (!Fixture.Start(*this))
	{
		return false;
	}
	const uint32 Before = FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial;
	for (uint32 Index = 1; Index <= 2; ++Index)
	{
		Fixture.Device->NotifyDeviceInteracted(nullptr, WxGameplayTags::Event_Device_Triggered);
		Fixture.Tick();
		TestEqual(TEXT("Linked-device event publishes each applied self-transition"),
			FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial, Before + Index);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceInitialTest, "Wx.Device.UntaggedInitialAndLateJoin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceInitialTest::RunTest(const FString& Parameters)
{
	for (int32 Scenario = 0; Scenario < 2; ++Scenario)
	{
		WxDeviceTests::FWxFixture Fixture;
		Fixture.Idle->Tag = FGameplayTag();
		const FGameplayTag Target = Fixture.Target->Tag;
		if (!Fixture.Start(*this, Scenario == 1, Scenario == 0 ? Target : FGameplayTag()))
		{
			return false;
		}
		if (Scenario == 1)
		{
			FWxDeviceStateSnapshot Snapshot;
			Snapshot.StateTagName = Target.GetTagName();
			Snapshot.EntrySerial = 10;
			Snapshot.RunStatus = EStateTreeRunStatus::Running;
			FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
			Fixture.Tick();
		}
		Fixture.Tick();
		TestEqual(TEXT("Initial target reached from an untagged wait"),
			FWxDeviceTestAccess::LocalTag(*Fixture.Component), Target);
		TestFalse(TEXT("Initial convergence does not fail"), FWxDeviceTestAccess::HasFailed(*Fixture.Component));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceCompletionTest, "Wx.Device.SameTickCompletionAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceCompletionTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	Fixture.Target->Tasks.Reset();
	Fixture.Target->AddTask<FWxDeviceTestTask>().GetInstanceData().bCompleteOnEnter = true;
	Fixture.Target->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::Succeeded);
	Fixture.Idle->AddTransition(EStateTreeTransitionTrigger::OnEvent, WxGameplayTags::Event_Device_Triggered,
		EStateTreeTransitionType::GotoState, Fixture.Target);
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fixture.Device->NotifyDeviceInteracted(nullptr, WxGameplayTags::Event_Device_Triggered);
	Fixture.Tick();
	TestFalse(TEXT("Automatic completion is not running"), Fixture.Component->IsRunning());
	const FWxDeviceStateSnapshot Completed = FWxDeviceTestAccess::Snapshot(*Fixture.Component);
	TestEqual(TEXT("Last tag survives same-tick completion"), Completed.StateTagName, Fixture.Target->Tag.GetTagName());
	TestEqual(TEXT("Completion status is replicated"), Completed.RunStatus, EStateTreeRunStatus::Succeeded);

	Fixture.Device->SetRole(ROLE_SimulatedProxy);
	FWxDeviceStateSnapshot Running = Completed;
	Running.StateTagName = Fixture.Idle->Tag.GetTagName();
	++Running.EntrySerial;
	Running.RunStatus = EStateTreeRunStatus::Running;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Running);
	TestTrue(TEXT("Receiving a new target restarts the completed client before its next tick"), Fixture.Component->IsRunning());
	TestTrue(TEXT("The requested transition schedules the recovered client's tick"), Fixture.Component->IsComponentTickEnabled());
	Fixture.Tick();
	Fixture.Tick();
	TestTrue(TEXT("Completed client restarts and converges"), Fixture.Component->IsRunning());
	TestEqual(TEXT("Recovery reaches the authority's actual local state"),
		FWxDeviceTestAccess::LocalTag(*Fixture.Component), Fixture.Idle->Tag);
	TestFalse(TEXT("Recovery succeeds within budget"), FWxDeviceTestAccess::HasFailed(*Fixture.Component));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceSnapshotTest, "Wx.Device.SnapshotBaselineAndReentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceSnapshotTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	if (!Fixture.Start(*this, true))
	{
		return false;
	}
	FWxDeviceStateSnapshot Snapshot;
	Snapshot.StateTagName = Fixture.Idle->Tag.GetTagName();
	Snapshot.EntrySerial = 42;
	Snapshot.RunStatus = EStateTreeRunStatus::Running;
	const uint32 Before = FWxDeviceTestAccess::LocalEntries(*Fixture.Component);
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	TestEqual(TEXT("First snapshot is a baseline, not 42 replays"),
		FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before);
	++Snapshot.EntrySerial;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	Fixture.Tick();
	TestEqual(TEXT("New same-tag entry reselects once"),
		FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before + 1u);
	Fixture.Tick();
	TestEqual(TEXT("Applied entry is not replayed again"),
		FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before + 1u);
	++Snapshot.EntrySerial;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	Snapshot.RunStatus = EStateTreeRunStatus::Succeeded;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	TestEqual(TEXT("Completion arriving during an entry request does not duplicate it"),
		FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before + 2u);
	TestFalse(TEXT("Authority completion stops the local tree"), Fixture.Component->IsRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceRetryTest, "Wx.Device.RecoveryAttemptLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceRetryTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	Fixture.Target->Tasks.Reset();
	Fixture.Target->AddTask<FWxDeviceTestTask>().GetInstanceData().bCompleteOnEnter = true;
	Fixture.Target->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::Succeeded);
	if (!Fixture.Start(*this, true))
	{
		return false;
	}
	FWxDeviceStateSnapshot Snapshot;
	Snapshot.StateTagName = Fixture.Target->Tag.GetTagName();
	Snapshot.EntrySerial = 1;
	Snapshot.RunStatus = EStateTreeRunStatus::Running;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	AddExpectedError(TEXT("Device synchronization stopped:"), EAutomationExpectedErrorFlags::Contains, 1);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Fixture.Tick();
	}
	TestTrue(TEXT("An incompatible local completion is diagnosed once"), FWxDeviceTestAccess::HasFailed(*Fixture.Component));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceHierarchyTest, "Wx.Device.ChildTransitionPreservesParentEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceHierarchyTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	UStateTreeState& ChildA = Fixture.Idle->AddChildState(TEXT("ChildA"));
	UStateTreeState& ChildB = Fixture.Idle->AddChildState(TEXT("ChildB"));
	ChildA.AddTask<FWxDeviceTestTask>();
	ChildB.AddTask<FWxDeviceTestTask>();
	ChildA.AddTransition(EStateTreeTransitionTrigger::OnEvent, WxGameplayTags::Event_Device_Triggered,
		EStateTreeTransitionType::GotoState, &ChildB);
	if (!Fixture.Start(*this))
	{
		return false;
	}
	const uint32 Before = FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial;
	Fixture.Device->NotifyDeviceInteracted(nullptr, WxGameplayTags::Event_Device_Triggered);
	Fixture.Tick();
	TestEqual(TEXT("A child transition does not reenter the tagged parent"),
		FWxDeviceTestAccess::Snapshot(*Fixture.Component).EntrySerial, Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceInteractorTest, "Wx.Device.WaitForInteractor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceInteractorTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	if (!Fixture.Start(*this, true))
	{
		return false;
	}
	FWxDeviceStateSnapshot Snapshot;
	Snapshot.StateTagName = Fixture.Idle->Tag.GetTagName();
	Snapshot.EntrySerial = 1;
	Snapshot.RunStatus = EStateTreeRunStatus::Running;
	Snapshot.bHasInteractor = true;
	ACharacter* PreviousInteractor = Fixture.World->SpawnActor<ACharacter>();
	Snapshot.Interactor = PreviousInteractor;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	Snapshot.StateTagName = Fixture.Target->Tag.GetTagName();
	++Snapshot.EntrySerial;
	Snapshot.Interactor = nullptr;
	const uint32 Before = FWxDeviceTestAccess::LocalEntries(*Fixture.Component);
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	TestEqual(TEXT("RepNotify preserves the running state's interactor while waiting"),
		Fixture.Device->GetInteractingCharacter(), PreviousInteractor);
	Fixture.Tick();
	TestEqual(TEXT("Unresolved actor reference does not enter with the previous interactor"),
		FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before);
	Snapshot.Interactor = Fixture.World->SpawnActor<ACharacter>();
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	Fixture.Tick();
	Fixture.Tick();
	TestEqual(TEXT("Resolved actor allows the target entry"), FWxDeviceTestAccess::LocalEntries(*Fixture.Component), Before + 1u);
	TestEqual(TEXT("Interactor matches the snapshot"), Fixture.Device->GetInteractingCharacter(), Snapshot.Interactor.Get());
	Snapshot.bHasInteractor = false;
	FWxDeviceTestAccess::Receive(*Fixture.Component, Snapshot);
	TestNull(TEXT("A no-interactor state ignores any residual pointer"), Fixture.Device->GetInteractingCharacter());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWxDeviceNativeReplicationTest, "Wx.Device.NativeSnapshotSerialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWxDeviceNativeReplicationTest::RunTest(const FString& Parameters)
{
	WxDeviceTests::FWxFixture Fixture;
	UScriptStruct* Struct = FWxDeviceStateSnapshot::StaticStruct();
	TestFalse(TEXT("Snapshot uses native property replication, not a custom serializer"),
		EnumHasAnyFlags(Struct->StructFlags, STRUCT_NetSerializeNative));
	const TSharedPtr<FRepLayout> Layout = FRepLayout::CreateFromStruct(Struct);
	UWxDeviceTestPackageMap* Map = NewObject<UWxDeviceTestPackageMap>(Fixture.World);

	FWxDeviceStateSnapshot Source;
	Source.StateTagName = Fixture.Target->Tag.GetTagName();
	Source.EntrySerial = 17;
	Source.RunStatus = EStateTreeRunStatus::Running;
	Source.Interactor = Fixture.World->SpawnActor<ACharacter>();
	Source.bHasInteractor = true;
	FNetBitWriter Writer(Map, 4096);
	bool bHasUnmapped = false;
	Layout->SerializePropertiesForStruct(Struct, Writer, Map, FRepObjectDataBuffer(&Source), bHasUnmapped);
	TestFalse(TEXT("Engine property serialization succeeds"), Writer.IsError());

	FWxDeviceStateSnapshot Received;
	FNetBitReader Reader(Map, Writer.GetData(), Writer.GetNumBits());
	bHasUnmapped = false;
	Layout->SerializePropertiesForStruct(Struct, Reader, Map, FRepObjectDataBuffer(&Received), bHasUnmapped);
	TestFalse(TEXT("Engine property deserialization succeeds"), Reader.IsError());
	TestTrue(TEXT("Native object property reports an unresolved reference"), bHasUnmapped);
	TestEqual(TEXT("State tag is retained while the actor is unresolved"), Received.StateTagName, Source.StateTagName);
	TestEqual(TEXT("Entry is retained while the actor is unresolved"), Received.EntrySerial, Source.EntrySerial);
	TestEqual(TEXT("Run status round-trips"), Received.RunStatus, Source.RunStatus);
	TestTrue(TEXT("A missing pointer is distinguishable from a no-interactor state"), Received.bHasInteractor);
	TestNull(TEXT("Unresolved pointer remains null"), Received.Interactor.Get());

	// 기본 복제에서 객체 참조 재해소의 단위는 해당 객체 프로퍼티다.
	// 이전 참조를 해소해도 나중에 받은 상태 번호/완료 상태까지 다시 읽지 않는다.
	++Received.EntrySerial;
	Received.RunStatus = EStateTreeRunStatus::Succeeded;
	Received.StateTagName = Fixture.Idle->Tag.GetTagName();
	Map->ResolvedObject = Source.Interactor.Get();
	FProperty* InteractorProperty = FindFProperty<FProperty>(Struct, GET_MEMBER_NAME_CHECKED(FWxDeviceStateSnapshot, Interactor));
	if (!TestNotNull(TEXT("Interactor is a reflected property"), InteractorProperty))
	{
		return false;
	}
	FNetBitWriter ReferenceWriter(Map, 256);
	InteractorProperty->NetSerializeItem(ReferenceWriter, Map, InteractorProperty->ContainerPtrToValuePtr<void>(&Source));
	FNetBitReader ReferenceReader(Map, ReferenceWriter.GetData(), ReferenceWriter.GetNumBits());
	TestTrue(TEXT("Native actor property resolves"),
		InteractorProperty->NetSerializeItem(ReferenceReader, Map, InteractorProperty->ContainerPtrToValuePtr<void>(&Received)));
	TestEqual(TEXT("Actor reference is restored"), Received.Interactor.Get(), Source.Interactor.Get());
	TestEqual(TEXT("Reference resolution does not roll back entry"), Received.EntrySerial, Source.EntrySerial + 1u);
	TestEqual(TEXT("Reference resolution does not roll back completion"), Received.RunStatus, EStateTreeRunStatus::Succeeded);
	TestEqual(TEXT("Reference resolution does not roll back tag"), Received.StateTagName, Fixture.Idle->Tag.GetTagName());
	return true;
}

#endif




