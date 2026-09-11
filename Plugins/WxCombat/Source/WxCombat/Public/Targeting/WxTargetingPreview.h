// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;
class UTargetingPreset;
struct FAnimNotifyEvent;

/** 애님 에디터 뷰포트에 타겟팅 프리셋의 범위를 그려 보인다. */
namespace WxTargetingPreview
{
	/**
	 * 프리셋의 AOE 선택 태스크를 실제 쿼리와 같은 트랜스폼·형상으로 그린다. 각 노티파이의 DrawInEditor에서 그대로 호출한다.
	 *
	 * 재생 위치가 노티파이 구간 안일 때만 그린다. 단발 노티파이는 구간 길이가 0이라 트리거 직후 짧은 창을 준다.
	 * 뷰포트 Character 메뉴의 노티파이 시각화가 켜져 있어야 보인다.
	 */
	void DrawDebugTargetingPreset(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FAnimNotifyEvent& NotifyEvent, const UTargetingPreset* TargetingPreset, const FLinearColor& Color);
}

#endif
