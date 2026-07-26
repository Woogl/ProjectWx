// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UniversalObjectLocator.h"
#include "WxActorTarget.generated.h"

/**
 * StateTree 태스크 인스턴스 데이터용 단일 배치 액터 지정 래퍼.
 * UOL 을 인스턴스 데이터의 직속 멤버로 두면 ST 에디터가 값 위젯을 만들지 못하는 엔진(5.8) 제한이 있다 —
 * ST 의 ModifyRow 가 직속 자식 행에만 GetDefaultWidgets 를 호출하고, 이것이 타입 커스터마이제이션을 훼손한다.
 * 한 겹 감싸 UOL 을 손자 행으로 내리면 이 경로를 타지 않아 픽커가 정상 동작한다(배열 원소가 정상인 것과 같은 원리).
 * 여러 도메인 플러그인의 ST 노드가 공용으로 쓰므로 WxCore 에 둔다.
 */
USTRUCT()
struct FWxActorTarget
{
	GENERATED_BODY()

	/** 대상 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Locator;
};
