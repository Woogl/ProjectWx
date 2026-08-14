// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UBlackboardComponent;

/**
 * 키 SET/CLEAR 는 AIController(SelfActor·HomeLocation), UWxAIPerceptionComponent(TargetActor), BTTask/BTService(PatrolTargetLocation·TargetDistance) 가 나눠 담당한다.
 * Blackboard 에셋에 같은 이름의 키가 등록돼 있어야 한다.
 *
 * 키별 accessor 는 키 이름과 값 타입을 한 곳에 묶어 GetValueAs / SetValueAs 계열의 타입 오용을 막는다.
 * accessor 는 Blackboard 가 유효(non-null)하다는 전제로 호출한다(호출부가 이미 가드함).
 * 키가 에셋에 없거나 타입이 어긋나면 엔진이 조용히 기본값을 돌려주므로, accessor 는 그런 접근을 경고 로그로 드러낸다(Shipping 빌드 제외).
 */
namespace WxBlackboardKeys
{
	WXAI_API extern const FName SelfActor;
	WXAI_API extern const FName TargetActor;
	WXAI_API extern const FName HomeLocation;
	WXAI_API extern const FName PatrolTargetLocation;
	WXAI_API extern const FName TargetDistance;

	// Object 키: null = 미설정이라 setter 에 nullptr 을 넘기면 Clear 와 동일하게 동작 → 별도 Clear 불필요.

	WXAI_API AActor* GetTargetActor(const UBlackboardComponent* Blackboard);
	WXAI_API void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value);

	WXAI_API AActor* GetSelfActor(const UBlackboardComponent* Blackboard);
	WXAI_API void SetSelfActor(UBlackboardComponent* Blackboard, AActor* Value);

	WXAI_API FVector GetHomeLocation(const UBlackboardComponent* Blackboard);
	WXAI_API void SetHomeLocation(UBlackboardComponent* Blackboard, const FVector& Value);

	WXAI_API void SetPatrolTargetLocation(UBlackboardComponent* Blackboard, const FVector& Value);

	// Float 키: 모든 float 가 유효값이라 "값 없음"을 Set 으로 표현할 수 없어, 타겟이 없을 때를 위한 Clear 를 별도로 둔다.
	WXAI_API void SetTargetDistance(UBlackboardComponent* Blackboard, float Value);
	WXAI_API void ClearTargetDistance(UBlackboardComponent* Blackboard);
}
