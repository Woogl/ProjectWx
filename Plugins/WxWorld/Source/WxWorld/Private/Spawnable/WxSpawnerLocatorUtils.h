// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "StateTreeNodeBase.h"
#endif

class AWxSpawner;
struct FUniversalObjectLocator;

/** 스포너 지정(TArray<FUniversalObjectLocator>)을 여러 태스크가 같은 규약으로 다루기 위한 헬퍼. */
struct FWxSpawnerLocatorUtils
{
	/**
	 * 지정이 가리키는 배치 스포너. 강제 로드는 하지 않으므로 스트리밍 아웃 상태면 nullptr 다.
	 *
	 * Context 는 게임 월드의 아무 오브젝트나 되며, 보통 태스크 오너를 넘긴다.
	 * 대상의 레벨을 주면 엔진의 1차 경로에서 바로 풀리고, 아니면 경로 직접 해석 폴백이 WP 런타임 셀 안의 대상까지 찾는다.
	 * 그래서 셀 안의 액터를 컨텍스트로 주려고 월드를 순회할 이유가 없다.
	 */
	static AWxSpawner* ResolveSpawner(const FUniversalObjectLocator& Locator, UObject* Context);

#if WITH_EDITOR
	/** 해석되는데 WxSpawner 가 아닌 지정을 컴파일 에러로 올린다. */
	static EDataValidationResult ValidateSpawners(UE::StateTree::ICompileNodeContext& CompileContext, const TArray<FUniversalObjectLocator>& Spawners);
#endif
};
