// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxProjectileComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 투사체 AnimNotify가 보낸 GameplayEvent를 받아 서버 권위로 투사체를 생성한다.
 * 투사체 클래스와 소켓은 이벤트 페이로드가 소유하므로 어빌리티와 무관하게 동작한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxProjectileComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleSpawnProjectileEvent(const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle SpawnProjectileEventHandle;
};
