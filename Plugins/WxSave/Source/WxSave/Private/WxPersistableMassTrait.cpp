// Copyright Woogle. All Rights Reserved.

#include "WxPersistableMassTrait.h"

#include "MassEntityTemplateRegistry.h"

void UWxPersistableEntityConfigTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FWxPersistableEntityConfigFragment>();
}
