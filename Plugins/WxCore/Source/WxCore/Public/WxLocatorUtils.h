// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FUniversalObjectLocator;

/** 저작 도구에 로케이터를 보여주기 위한 헬퍼. */
struct WXCORE_API FWxLocatorUtils
{
#if WITH_EDITOR
	/** 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	static FText GetDisplayName(const FUniversalObjectLocator& Locator);

	/** 표시명 3개까지 나열하고 초과분은 +N 으로 줄인다. */
	static FText GetDisplayNames(const TArray<FUniversalObjectLocator>& Locators);
#endif
};
