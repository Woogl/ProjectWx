// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameMode.h"

#include "Engine/GameInstance.h"
#include "WxSaveGameSubsystem.h"

AActor* AWxGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 활성 체크포인트가 있으면 그 PlayerStartTag 로 배치된 체크포인트(APlayerStart)를 찾아 부활 지점으로 쓴다.
	// 체크포인트 액터 자체가 부활 지점이라 임시 PlayerStart 스폰이 필요 없다.
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWxSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWxSaveGameSubsystem>())
		{
			const FName CheckpointTag = SaveSubsystem->GetActiveCheckpointTag();
			if (!CheckpointTag.IsNone())
			{
				if (AActor* CheckpointStart = FindPlayerStart(Player, CheckpointTag.ToString()))
				{
					return CheckpointStart;
				}
			}
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}
