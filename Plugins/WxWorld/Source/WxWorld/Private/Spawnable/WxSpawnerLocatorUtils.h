// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "StateTreeNodeBase.h"

struct FUniversalObjectLocator;
#endif

struct FWxSpawnerLocatorUtils
{
#if WITH_EDITOR
	/** 해석되는데 WxSpawner 가 아닌 지정을 컴파일 에러로 올린다. */
	static EDataValidationResult ValidateSpawners(UE::StateTree::ICompileNodeContext& CompileContext, const TArray<FUniversalObjectLocator>& Spawners);
#endif
};
