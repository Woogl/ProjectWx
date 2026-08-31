// Copyright Woogle. All Rights Reserved.

#include "WxSavable.h"

bool IWxSavable::ShouldPersistRuntimeActor() const
{
	return true;
}

void IWxSavable::OnSavePreparing()
{
}

void IWxSavable::OnSaveRestored(const TArray<FName>& RestoredPropertyNames)
{
}

void IWxSavable::OnPostRestoreLevel()
{
}
