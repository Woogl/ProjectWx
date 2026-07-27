// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UniversalObjectLocator.h"
#include "WxActorTarget.generated.h"

/**
 * StateTree 태스크 인스턴스 데이터용 단일 배치 액터 지정 래퍼.
 * UOL 을 인스턴스 데이터의 직속 멤버로 두면 ST 에디터가 값 위젯을 만들지 못하는 엔진(5.8) 버그가 있다.
 * ST 의 ModifyRow 는 직속 자식 행마다 GetDefaultWidgets 를 부르는데, 이것이 타입 커스터마이제이션의 CustomizeHeader 를 한 번 더 실행시킨다.
 * UOL 커스터마이제이션은 CustomizeHeader 끝에서 리빌드를 예약하면서 요청 플래그를 세우고 곧 버려질 위젯에 액티브 타이머를 걸어 두므로(그 위젯은 트리에 없어 절대 발화하지 않는다),
 * 실제 행을 만들 때의 두 번째 CustomizeHeader 는 그 플래그에 막혀 리빌드를 예약하지 못하고 값 영역이 빈 채로 남는다.
 * 한 겹 감싸 UOL 을 손자 행으로 내리면 ModifyRow 대상이 아니게 되어 CustomizeHeader 가 한 번만 불린다(배열 원소가 정상인 것과 같은 원리).
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
