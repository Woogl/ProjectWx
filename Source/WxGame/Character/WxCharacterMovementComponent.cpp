// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "GameFramework/Character.h"

UWxCharacterMovementComponent::UWxCharacterMovementComponent()
{
	// 상승 높이 = 캐릭터 키(180cm), 상승 0.55s 기준으로 역산한 점프 초속.
	// 중력 스케일(Rise/Fall)과 함께 총 체공 약 1.0초(상승 0.55 / 하강 0.45)를 만든다.
	JumpZVelocity = 655.f;
}

float UWxCharacterMovementComponent::GetGravityZ() const
{
	// Super::GetGravityZ() == 월드 중력 * GravityScale(=1.0 유지) == 월드 중력값.
	// 여기에 상승/하강 스케일만 곱해 비대칭 중력을 적용한다.
	const float BaseGravityZ = Super::GetGravityZ();
	const float DirectionalScale = (Velocity.Z < 0.f) ? FallGravityScale : RiseGravityScale;
	return BaseGravityZ * DirectionalScale;
}

bool UWxCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	// 1단(지상 또는 코요테 첫 점프)은 엔진 기본 처리로 위임해 풀 JumpZVelocity를 유지한다.
	// JumpCurrentCountPreJump는 CheckJumpInput의 코요테 보정 증가 이전 값이라 1단 판별에 안전하다.
	if (!CharacterOwner || CharacterOwner->JumpCurrentCountPreJump == 0)
	{
		return Super::DoJump(bReplayingMoves, DeltaTime);
	}

	// 2단 이상(공중) 점프: 1단의 절반 높이가 나오는 Z속도를 새로 부여한다(현재 낙하 속도 무시).
	if (CharacterOwner->CanJump() && (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f))
	{
		Velocity.Z = SecondJumpZVelocity;
		SetMovementMode(MOVE_Falling);
		return true;
	}

	return false;
}
