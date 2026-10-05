// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AActor;
class UBlackboardComponent;
class UBlackboardKeyType;

/**
 * SelfActor 는 엔진이 채운다(모든 Blackboard 에셋의 고정 키이고, AAIController 가 블랙보드 초기화·빙의 때 폰으로 갱신한다).
 * 나머지 키 SET/CLEAR 는 AIController(HomeLocation·Master·빙의 전환 시 TargetActor), BTTask/BTService(TargetActor·PatrolTargetLocation·TargetDistance) 가 나눠 담당한다.
 * Blackboard 에셋에 같은 이름의 키가 등록돼 있어야 한다.
 *
 * 키별 accessor 는 키 이름과 값 타입을 한 곳에 묶어 GetValueAs / SetValueAs 계열의 타입 오용을 막는다.
 * accessor 는 Blackboard 가 유효(non-null)하다는 전제로 호출한다(호출부가 이미 가드함).
 * 키가 에셋에 없거나 타입이 어긋나면 엔진이 조용히 기본값을 돌려주므로, accessor 는 그런 접근을 경고 로그로 드러낸다(Shipping 빌드 제외).
 */
namespace WxBlackboardKeys
{
	WXGAME_API extern const FName SelfActor;
	WXGAME_API extern const FName TargetActor;
	/** 소환자다. 주인 없이 태어난 폰에서는 비어 있다. */
	WXGAME_API extern const FName Master;
	WXGAME_API extern const FName HomeLocation;
	WXGAME_API extern const FName PatrolTargetLocation;
	WXGAME_API extern const FName TargetDistance;

	/** accessor 전용 진단이라 export 하지 않는다. */
	void VerifyBlackboardKey(const UBlackboardComponent* Blackboard, const FName& KeyName, TSubclassOf<UBlackboardKeyType> ExpectedType);

	// Object 키: null = 미설정이라 setter 에 nullptr 을 넘기면 Clear 와 동일하게 동작 → 별도 Clear 불필요.

	WXGAME_API AActor* GetTargetActor(const UBlackboardComponent* Blackboard);
	WXGAME_API void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value);

	WXGAME_API AActor* GetSelfActor(const UBlackboardComponent* Blackboard);

	WXGAME_API void SetMaster(UBlackboardComponent* Blackboard, AActor* Value);

	WXGAME_API void SetHomeLocation(UBlackboardComponent* Blackboard, const FVector& Value);

	WXGAME_API void SetPatrolTargetLocation(UBlackboardComponent* Blackboard, const FVector& Value);

	// Float 키: Clear 가 0(=코앞)을 써 근거리 비교를 통과시키므로, 타겟이 없을 때는 대신 이 값을 기록해 "무한히 멀다"로 읽히게 한다.
	WXGAME_API extern const float NoTargetDistance;

	WXGAME_API void SetTargetDistance(UBlackboardComponent* Blackboard, float Value);
}
