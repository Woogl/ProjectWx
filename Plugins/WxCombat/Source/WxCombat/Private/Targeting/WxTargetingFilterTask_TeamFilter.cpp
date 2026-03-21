// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_TeamFilter.h"
#include "GenericTeamAgentInterface.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_TeamFilter::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle))
	{
		const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(SourceContext->SourceActor);
		const AActor* TargetActor = TargetData.HitResult.GetActor();
		if (!SourceTeamAgent || !TargetActor)
		{
			return false;
		}

		const ETeamAttitude::Type Attitude = SourceTeamAgent->GetTeamAttitudeTowards(*TargetActor);
		switch (Attitude)
		{
		case ETeamAttitude::Friendly:
			return !bIncludeFriendly;
		case ETeamAttitude::Hostile:
			return !bIncludeHostile;
		case ETeamAttitude::Neutral:
			return !bIncludeNeutral;
		}
	}

	return false;
}
