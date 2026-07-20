// Copyright Woogle. All Rights Reserved.

#include "Character/WxCharacterMovementComponent.h"
#include "GameFramework/Character.h"

UWxCharacterMovementComponent::UWxCharacterMovementComponent()
{
	JumpZVelocity = 640.f;
}

float UWxCharacterMovementComponent::GetGravityZ() const
{
	// Super::GetGravityZ() == 월드 중력 * GravityScale(=1.0 유지) == 월드 중력값.
	// 여기에 상승/하강 스케일만 곱해 비대칭 중력을 적용한다.
	const float BaseGravityZ = Super::GetGravityZ();
	const float DirectionalScale = (Velocity.Z < 0.f) ? FallGravityScale : RiseGravityScale;
	return BaseGravityZ * DirectionalScale;
}
