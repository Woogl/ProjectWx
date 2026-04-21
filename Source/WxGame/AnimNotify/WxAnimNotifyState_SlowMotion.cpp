// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_SlowMotion.h"
#include "Framework/WxGameState.h"
#include "Engine/World.h"

void UWxAnimNotifyState_SlowMotion::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// ANS는 서버/클라이언트 양쪽에서 발동하지만, Global TimeDilation은 서버 권위로만 갱신.
	// 클라이언트는 AWxGameState의 OnRep으로 동기화된다.
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (AWxGameState* GameState = Owner->GetWorld()->GetGameState<AWxGameState>())
	{
		GameState->SetGlobalTimeDilationAuthoritative(TargetDilation);
	}
}

void UWxAnimNotifyState_SlowMotion::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (AWxGameState* GameState = Owner->GetWorld()->GetGameState<AWxGameState>())
	{
		GameState->SetGlobalTimeDilationAuthoritative(1.f);
	}
}
