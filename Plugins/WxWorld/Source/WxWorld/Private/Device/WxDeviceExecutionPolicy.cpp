// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceExecutionPolicy.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"

bool FWxDeviceExecutionPolicy::IsRestoring(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	return !Transition.SourceStateID.IsValid() || IsRestoringDevice(Context);
}

bool FWxDeviceExecutionPolicy::IsRestoringDevice(const FStateTreeExecutionContext& Context)
{
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxDeviceStateTreeComponent* Component = Owner ? Owner->FindComponentByClass<UWxDeviceStateTreeComponent>() : nullptr;
	return Component && Component->IsRestoringState();
}
