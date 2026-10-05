// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxPackageToolset.generated.h"

/**
 * AssetTools.save_assets 가 true 만 돌려주고 넘어가는 경우까지 확실히 저장한다 — 수정 표시가 없는 패키지(컴파일만 거친 BP 등), 월드 파티션 외부 액터 패키지, 읽기 전용 파일.
 */
UCLASS(BlueprintType, Hidden)
class UWxPackageToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 넘긴 오브젝트가 든 패키지를 수정 여부와 상관없이 저장하고, 실제로 새로 쓰인 파일 경로를 돌려준다.
	 * 외부 액터는 그 액터를 넘겨야 액터 패키지가 저장되고, 맵을 넘기면 맵 패키지만 저장된다.
	 * 읽기 전용 파일이 하나라도 있으면 아무것도 저장하지 않고 실패하며, 엔진이 저장하지 않은 패키지가 있어도 실패한다.
	 * @param Objects 저장할 패키지 안의 아무 오브젝트. 예: [{"refPath":"/Game/Anim/AM_X.AM_X"}, {"refPath":"/Game/Maps/LV_X.LV_X:PersistentLevel.액터명"}]
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static TArray<FString> SavePackages(const TArray<UObject*>& Objects);
};
