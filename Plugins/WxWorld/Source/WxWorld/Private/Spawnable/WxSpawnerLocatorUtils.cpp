// Copyright Woogle. All Rights Reserved.

#include "WxSpawnerLocatorUtils.h"

#if WITH_EDITOR
#include "Spawnable/WxSpawner.h"
#include "UniversalObjectLocator.h"
#include "WxLocatorUtils.h"

EDataValidationResult FWxSpawnerLocatorUtils::ValidateSpawners(UE::StateTree::ICompileNodeContext& CompileContext, const TArray<FUniversalObjectLocator>& Spawners)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	// UOL 픽커에는 액터 클래스를 좁히는 엔진 확장점이 없으므로 컴파일에서 잡는다 — 드래그드롭으로 넣은 값도 같이 걸린다.
	// 미해석(빈 로케이터·WP 언로드)은 타입을 알 수 없으므로 통과시킨다 — 에디터의 로드 상태에 따라 컴파일 결과가 갈리면 안 된다.
	for (const FUniversalObjectLocator& Locator : Spawners)
	{
		const UObject* Object = Locator.SyncFind();
		if (Object && !Object->IsA<AWxSpawner>())
		{
			CompileContext.AddValidationError(FText::Format(INVTEXT("Spawners: '{0}' 은(는) WxSpawner 가 아니다."), FText::FromString(FWxLocatorUtils::GetDisplayName(Locator))));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
