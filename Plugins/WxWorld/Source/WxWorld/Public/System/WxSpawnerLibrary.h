// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSpawnerLibrary.generated.h"

/**
 * Blueprint 진입점.
 * 월드의 Spawner 를 TActorIterator 로 순회해 일괄 리스폰한다.
 *
 * 서버 권위 호출만 의미가 있다.
 * 클라에서 호출되면 Spawner 가 내부에서 무시한다.
 */
UCLASS()
class WXWORLD_API UWxSpawnerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 월드의 Auto 모드 Spawner 의 Respawn() 호출.
	 * Manual 모드는 제외.
	 * 부활 가능 Spawner 는 새 인스턴스 생성, 영구 사망 (보스) 은 스킵.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Spawner", meta = (WorldContext = "WorldContextObject"))
	static void TryRespawnAll(const UObject* WorldContextObject);
};
