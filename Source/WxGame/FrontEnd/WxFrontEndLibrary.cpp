// Copyright Woogle. All Rights Reserved.

#include "FrontEnd/WxFrontEndLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FrontEnd/WxGameFlowSubsystem.h"

void UWxFrontEndLibrary::GetTravelStatus(const UObject* WorldContextObject, bool& bBusy, FText& Message)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UWxGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UWxGameFlowSubsystem>() : nullptr;
	bBusy = !Flow || Flow->IsBusy();
	Message = Flow ? Flow->GetStatusText() : FText::GetEmpty();
}

bool UWxFrontEndLibrary::RequestNewGame(const UObject* WorldContextObject, TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UWxGameFlowSubsystem* Flow = GI ? GI->GetSubsystem<UWxGameFlowSubsystem>() : nullptr;
	return Flow && Flow->RequestNewGame(PawnClass, Level);
}
