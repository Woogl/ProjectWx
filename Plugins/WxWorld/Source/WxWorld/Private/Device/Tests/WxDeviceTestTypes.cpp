// Copyright Woogle. All Rights Reserved.

#include "Device/Tests/WxDeviceTestTypes.h"
#include "StateTreeExecutionContext.h"

FWxDeviceTestTask::FWxDeviceTestTask()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxDeviceTestTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return Context.GetInstanceData(*this).bCompleteOnEnter ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

bool UWxDeviceTestPackageMap::SerializeObject(FArchive& Ar, UClass* InClass, UObject*& Object, FNetworkGUID* OutNetGUID)
{
	bool bPresent = Object != nullptr;
	Ar.SerializeBits(&bPresent, 1);
	if (Ar.IsLoading())
	{
		Object = bPresent ? ResolvedObject.Get() : nullptr;
	}
	return !bPresent || (Object && Object->IsA(InClass));
}
