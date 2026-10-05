// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestVolume.h"

#include "Character/WxPlayerCharacter.h"
#include "Quest/WxQuestLibrary.h"
#include "StateTree.h"

void AWxQuestVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!HasAuthority() || !Cast<AWxPlayerCharacter>(OtherActor))
	{
		return;
	}

	UWxQuestLibrary::StartQuest(this, QuestAsset);
	SetActorEnableCollision(false);
}
