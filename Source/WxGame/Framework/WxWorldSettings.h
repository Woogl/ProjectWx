// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/WxPersistedAbilitySystemState.h"
#include "GameFramework/WorldSettings.h"
#include "WxSavable.h"
#include "WxWorldSettings.generated.h"

class AActor;
class UAttributeSet;
class UWxExperienceDefinition;

USTRUCT()
struct FWxPersistedGameplayAttribute
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftClassPtr<UAttributeSet> AttributeSetClass;

	UPROPERTY()
	FName AttributeName;

	UPROPERTY()
	float BaseValue = 0.f;
};

/** 플레이어 폰을 LSP 런타임 재스폰 대상에 넣지 않고 맵 액터가 운반하는 플레이어 상태. */
USTRUCT()
struct FWxPersistedPlayerState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasData = false;

	UPROPERTY()
	TArray<FWxPersistedGameplayAttribute> Attributes;

	UPROPERTY()
	FWxPersistedAbilitySystemState AbilitySystem;
};

/**
 * 맵이 자기 기본 Experience 를 지정하는 자리다 — GameMode 는 진입 URL 다음으로 이 값을 보며, 확정의 마지막 단계다.
 */
UCLASS()
class AWxWorldSettings : public AWorldSettings, public IWxSavable
{
	GENERATED_BODY()

public:
	/** 미지정·미스캔이면 무효 ID 를 반환한다. 진입 URL 도 비어 있었다면 그대로 Experience 미확정이 된다. */
	FPrimaryAssetId GetDefaultGameplayExperience() const;

	//~ Begin IWxSavable
	virtual void OnSavePreparing() override;
	virtual void OnSaveRestored(const TArray<FName>& RestoredPropertyNames) override;
	//~ End IWxSavable

	/** 빙의와 AbilitySet 초기화가 끝난 권위 플레이어에게 복원 대기 중인 상태를 한 번 적용한다. */
	void ApplyPendingPlayerState(AActor* PlayerActor);

protected:
	UPROPERTY(EditAnywhere, Category = "GameMode")
	TSoftObjectPtr<UWxExperienceDefinition> GameplayExperience;

private:
	void CapturePlayerState(AActor* PlayerActor);
	void ApplyPlayerState(AActor* PlayerActor);

	UPROPERTY()
	FWxPersistedPlayerState PlayerPersistenceState;

	bool bPlayerRestorePending = false;
};
