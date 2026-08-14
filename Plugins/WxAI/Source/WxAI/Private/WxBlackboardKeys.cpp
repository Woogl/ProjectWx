// Copyright Woogle. All Rights Reserved.

#include "WxBlackboardKeys.h"

#include "WxAIModule.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Actor.h"

namespace
{
	// 매 accessor 호출마다 도는 진단이라 Shipping 빌드에서는 검사를 통째로 비운다.
	void VerifyBlackboardKey(const UBlackboardComponent* Blackboard, const FName& KeyName, TSubclassOf<UBlackboardKeyType> ExpectedType)
	{
#if !UE_BUILD_SHIPPING
		const FBlackboard::FKey KeyID = Blackboard->GetKeyID(KeyName);
		if (KeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogWxAI, Warning, TEXT("Blackboard 키 '%s' 를 에셋 '%s' 에서 찾을 수 없습니다. 키 이름 오타나 에셋 등록 여부를 확인하세요."),
				*KeyName.ToString(), *GetNameSafe(Blackboard->GetBlackboardAsset()));
			return;
		}

		const TSubclassOf<UBlackboardKeyType> ActualType = Blackboard->GetKeyType(KeyID);
		if (ActualType != ExpectedType)
		{
			UE_LOG(LogWxAI, Warning, TEXT("Blackboard 키 '%s' 의 타입이 기대한 %s 가 아니라 %s 입니다."),
				*KeyName.ToString(), *GetNameSafe(ExpectedType), *GetNameSafe(ActualType));
		}
#endif
	}
}

namespace WxBlackboardKeys
{
	const FName SelfActor = TEXT("SelfActor");

	const FName TargetActor = TEXT("TargetActor");

	const FName HomeLocation = TEXT("HomeLocation");

	const FName PatrolTargetLocation = TEXT("PatrolTargetLocation");

	const FName TargetDistance = TEXT("TargetDistance");

	AActor* GetTargetActor(const UBlackboardComponent* Blackboard)
	{
		VerifyBlackboardKey(Blackboard, TargetActor, UBlackboardKeyType_Object::StaticClass());
		return Cast<AActor>(Blackboard->GetValueAsObject(TargetActor));
	}

	void SetTargetActor(UBlackboardComponent* Blackboard, AActor* Value)
	{
		VerifyBlackboardKey(Blackboard, TargetActor, UBlackboardKeyType_Object::StaticClass());
		Blackboard->SetValueAsObject(TargetActor, Value);
	}

	FVector GetHomeLocation(const UBlackboardComponent* Blackboard)
	{
		VerifyBlackboardKey(Blackboard, HomeLocation, UBlackboardKeyType_Vector::StaticClass());
		return Blackboard->GetValueAsVector(HomeLocation);
	}

	void SetHomeLocation(UBlackboardComponent* Blackboard, const FVector& Value)
	{
		VerifyBlackboardKey(Blackboard, HomeLocation, UBlackboardKeyType_Vector::StaticClass());
		Blackboard->SetValueAsVector(HomeLocation, Value);
	}

	AActor* GetSelfActor(const UBlackboardComponent* Blackboard)
	{
		VerifyBlackboardKey(Blackboard, SelfActor, UBlackboardKeyType_Object::StaticClass());
		return Cast<AActor>(Blackboard->GetValueAsObject(SelfActor));
	}

	void SetSelfActor(UBlackboardComponent* Blackboard, AActor* Value)
	{
		VerifyBlackboardKey(Blackboard, SelfActor, UBlackboardKeyType_Object::StaticClass());
		Blackboard->SetValueAsObject(SelfActor, Value);
	}

	void SetPatrolTargetLocation(UBlackboardComponent* Blackboard, const FVector& Value)
	{
		VerifyBlackboardKey(Blackboard, PatrolTargetLocation, UBlackboardKeyType_Vector::StaticClass());
		Blackboard->SetValueAsVector(PatrolTargetLocation, Value);
	}

	void SetTargetDistance(UBlackboardComponent* Blackboard, float Value)
	{
		VerifyBlackboardKey(Blackboard, TargetDistance, UBlackboardKeyType_Float::StaticClass());
		Blackboard->SetValueAsFloat(TargetDistance, Value);
	}

	void ClearTargetDistance(UBlackboardComponent* Blackboard)
	{
		VerifyBlackboardKey(Blackboard, TargetDistance, UBlackboardKeyType_Float::StaticClass());
		Blackboard->ClearValue(TargetDistance);
	}
}
