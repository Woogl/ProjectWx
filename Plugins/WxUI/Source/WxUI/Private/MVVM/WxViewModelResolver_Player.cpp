// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Player.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/WxViewModelUtils.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelCollection.h"

UObject* UWxViewModelResolver_Player::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UMVVMViewModelCollectionObject* Collection = WxViewModel::GetGlobalCollection(UserWidget);
	return Collection && ExpectedType ? Collection->FindFirstViewModelInstanceOfType(const_cast<UClass*>(ExpectedType)) : nullptr;
}
