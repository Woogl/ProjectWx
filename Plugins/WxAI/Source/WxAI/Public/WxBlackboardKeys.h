// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UBlackboardComponent;

/**
 * BehaviorTree 가 사용하는 Blackboard 키 이름과, 각 키를 알맞은 타입으로 읽고 쓰는 accessor 모음.
 *
 * 키 SET/CLEAR 는 AIController 또는 UWxAIPerceptionComponent 가 담당하고, 본 namespace 는 BTTask/BTService/관찰자가 키를 이름으로 참조할 때 사용한다.
 * Blackboard 에셋에 같은 이름의 키가 등록돼 있어야 한다.
 *
 * 키별 accessor 는 키 이름과 값 타입을 한 곳에 묶어 GetValueAs / SetValueAs 계열의 타입 오용을 막는다.
 * accessor 는 Blackboard 가 유효(non-null)하다는 전제로 호출한다(호출부가 이미 가드함).
 */
namespace WxBlackboardKeys
{
	WXAI_API extern const FName SelfActor;
	WXAI_API extern const FName TargetActor;
	WXAI_API extern const FName HomeLocation;
	WXAI_API extern const FName TargetLastKnownLocation;
	WXAI_API extern const FName Phase;
	WXAI_API extern const FName PatrolTargetLocation;

	// 타입드 accessor (키 이름 ↔ 값 타입을 묶는다)
	// Object 키: null = 미설정이라 setter 에 nullptr 을 넘기면 Clear 와 동일하게 동작 → 별도 Clear 불필요.
	// Vector 키: 모든 FVector 가 유효값이라 "값 없음"을 Set 으로 표현할 수 없어 Clear 를 별도로 둔다(정찰 게이트가 이 unset 에 의존).

	WXAI_API AActor* GetTargetActor(const UBlackboardComponent* Blackboard);
	WXAI_API void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value);

	WXAI_API void SetSelfActor(UBlackboardComponent* Blackboard, AActor* Value);

	WXAI_API FVector GetHomeLocation(const UBlackboardComponent* Blackboard);
	WXAI_API void SetHomeLocation(UBlackboardComponent* Blackboard, const FVector& Value);

	WXAI_API void SetTargetLastKnownLocation(UBlackboardComponent* Blackboard, const FVector& Value);
	WXAI_API void ClearTargetLastKnownLocation(UBlackboardComponent* Blackboard);

	WXAI_API void SetPatrolTargetLocation(UBlackboardComponent* Blackboard, const FVector& Value);
	WXAI_API void ClearPatrolTargetLocation(UBlackboardComponent* Blackboard);
}
