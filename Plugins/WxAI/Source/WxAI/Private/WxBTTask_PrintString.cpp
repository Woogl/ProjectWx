// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_PrintString.h"
#include "WxAIModule.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/Engine.h"

UWxBTTask_PrintString::UWxBTTask_PrintString()
{
	NodeName = TEXT("Print String");
}

EBTNodeResult::Type UWxBTTask_PrintString::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FString Output = Message;
	if (bPrependPawnName)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
		const FString PawnName = Pawn ? Pawn->GetName() : TEXT("None");
		Output = FString::Printf(TEXT("[%s] %s"), *PawnName, *Message);
	}

	UE_LOG(LogWxAI, Log, TEXT("%s"), *Output);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, ScreenDuration, ScreenColor, Output);
	}

	return EBTNodeResult::Succeeded;
}

FString UWxBTTask_PrintString::GetStaticDescription() const
{
	return Message;
}
