// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxProjectileSubsystem.generated.h"

class AWxProjectileBase;

/**
 * 투사체를 서버 권위로 생성하는 월드 단위 진입점. 클래스와 위치는 호출자(몽타주 노티파이)가 정한다.
 */
UCLASS()
class WXCOMBAT_API UWxProjectileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Owner를 소유자이자 Instigator로 심어 투사체가 팀과 적중 판정의 출처로 쓴다. 생성하지 않으면 null. */
	AWxProjectileBase* SpawnProjectile(AActor& Owner, TSubclassOf<AWxProjectileBase> ProjectileClass, const FTransform& SpawnTransform);

protected:
	/** 에디터 월드에서는 시퀀서 프리뷰의 노티파이가 권위를 통과해 레벨에 스폰해 버리므로 게임 월드에만 만든다. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
