// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSpawnerLibrary.generated.h"

struct FUniversalObjectLocator;

/**
 * 서버 권위 호출만 의미가 있다. 클라에서 호출되면 Spawner 가 내부에서 무시한다.
 */
UCLASS()
class WXWORLD_API UWxSpawnerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Manual 모드는 제외.
	 * 부활 가능 Spawner 는 새 인스턴스 생성, 영구 사망 (보스) 은 스킵.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Spawner", meta = (WorldContext = "WorldContextObject"))
	static void TryRespawnAll(const UObject* WorldContextObject);

#if WITH_EDITOR
	/** 로케이터의 표시명. 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 none. */
	static FString GetSpawnerLocatorDisplayName(const FUniversalObjectLocator& Locator);

	/** 지정 목록의 표시 텍스트. 라벨 3개까지 나열하고 초과분은 +N 로 줄인다. 빈 배열은 none. */
	static FText GetSpawnerLocatorsText(const TArray<FUniversalObjectLocator>& Spawners);
#endif
};
