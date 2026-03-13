// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"

UInputMappingContext* AWxPlayerController::GetDefaultMappingContext() const
{
	return DefaultMappingContext;
}

const UInputAction* AWxPlayerController::GetMoveAction() const
{
	return MoveAction;
}

const UInputAction* AWxPlayerController::GetLookAction() const
{
	return LookAction;
}

const UWxInputConfig* AWxPlayerController::GetInputConfig() const
{
	return InputConfig;
}
