// Copyright Woogle. All Rights Reserved.

#include "Spawner/WxSpawnerLocatorUtils.h"

#include "Spawner/WxSpawner.h"
#include "UniversalObjectLocator.h"

#if WITH_EDITOR
#include "System/WxLocatorUtils.h"
#endif

AWxSpawner* FWxSpawnerLocatorUtils::ResolveSpawner(const FUniversalObjectLocator& Locator, UObject* Context)
{
	return Cast<AWxSpawner>(Locator.SyncFind(Context));
}

#if WITH_EDITOR
EDataValidationResult FWxSpawnerLocatorUtils::ValidateSpawners(UE::StateTree::ICompileNodeContext& CompileContext, const TArray<FUniversalObjectLocator>& Spawners)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	// 픽커가 후보를 좁혀도 복사·붙여넣기처럼 픽커를 거치지 않은 값이 있어 컴파일에서 한 번 더 잡는다.
	// 미해석(빈 로케이터·WP 언로드)은 타입을 알 수 없으므로 통과시킨다 — 에디터의 로드 상태에 따라 컴파일 결과가 갈리면 안 된다.
	for (const FUniversalObjectLocator& Locator : Spawners)
	{
		const UObject* Object = Locator.SyncFind();
		if (Object && !Object->IsA<AWxSpawner>())
		{
			CompileContext.AddValidationError(FText::Format(INVTEXT("Spawners: '{0}' is not a WxSpawner."), FWxLocatorUtils::GetDisplayName(Locator)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
