// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxRewardLibrary.generated.h"

/**
 * 보상(FWxRewardTableRow) 지급의 서버 권위 진입점.
 * 상태가 없는 1회성 동작이라 컴포넌트 대신 라이브러리로 둔다.
 */
UCLASS()
class WXINVENTORY_API UWxRewardLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * RewardRow 의 유효한 (아이템, 수량) 항목을 서버 권한에서 지급한다.
	 * 비권한·미지정이면 노옵.
	 *
	 * Pickup Fragment 가 있는 보상은 SpawnTransform 위치에 픽업 액터를 스폰해 LaunchVelocity 방향·크기로 발사한다.
	 * Pickup Fragment 가 없는 보상(예: 재화)은 외형이 없으므로 DirectGrantTarget 의 인벤토리에 직접 지급한다(대상이 없으면 스킵).
	 *
	 * @param SourceActor        권위 판정과 월드(스폰) 출처. nullptr 이거나 비권한이면 아무것도 하지 않는다.
	 * @param RewardRow          지급할 보상. FWxRewardTableRow 로우. 비어 있으면 노옵.
	 * @param DirectGrantTarget  Pickup Fragment 없는 아이템을 인벤토리에 직접 지급할 대상. nullptr 이면 해당 아이템은 스킵된다.
	 * @param SpawnTransform     픽업 스폰 위치(픽업 보상에만 사용). 발사 방향은 트랜스폼과 무관하게 LaunchVelocity 가 정한다.
	 * @param LaunchVelocity     픽업 발사 속도 벡터(cm/s). 방향과 크기를 모두 담는다. 영벡터면 발사 없이 드랍된다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wx|Reward", meta = (AutoCreateRefTerm = "RewardRow,SpawnTransform"))
	static void GrantReward(AActor* SourceActor, const FDataTableRowHandle& RewardRow, AActor* DirectGrantTarget, const FTransform& SpawnTransform, FVector LaunchVelocity = FVector(0.f, 0.f, 300.f));
};
