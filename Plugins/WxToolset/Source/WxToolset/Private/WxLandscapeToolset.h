// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxLandscapeToolset.generated.h"

class ALandscape;
class UMaterialInterface;

/**
 * 스크립트 표면이 없는 랜드스케이프 생성과 높이·가중치 통째 가져오기만 뚫는다 — 에디터 New Landscape → Import 와 Import 도구가 하는 일.
 * 머티리얼·나나이트 같은 일반 속성은 ObjectTools.set_properties 로 바꾼다.
 * RAW 파일은 X 가 빠른 축인 행 우선·리틀 엔디언이고, 높이는 uint16 한 장, 가중치는 레이어마다 uint8 한 장이다.
 */
UCLASS(BlueprintType, Hidden)
class UWxLandscapeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 현재 에디터 월드에 새 랜드스케이프를 만든다. 월드 파티션 월드면 GridSizeInComponents 단위로 스트리밍 프록시를 나눈다.
	 * @param HeightmapPath SizeVerts×SizeVerts uint16 RAW 파일.
	 * @param SizeVerts 한 변의 정점 수. QuadsPerSection×SectionsPerComponent 의 배수 + 1 이어야 한다. 예: 4033 = 126×32 + 1
	 * @param QuadsPerSection 엔진 허용값: 7, 15, 31, 63, 127, 255.
	 * @param SectionsPerComponent 한 축의 섹션 수: 1 또는 2.
	 * @param GridSizeInComponents 스트리밍 프록시 한 축의 컴포넌트 수: 에디터와 같은 1~16.
	 * @param Location 정점 (0,0)의 월드 위치(cm).
	 * @param LayerNames 페인트 레이어 이름. 가중치는 WeightmapFolder/<이름>.raw 에서 읽고, 레이어 인포는 LayerInfoPath/LI_<이름> 을 쓰되 없으면 만든다.
	 * @param LayerInfoPath 레이어 인포 에셋 폴더. 예: "/Game/LevelDesign/Landscape/LayerInfo"
	 * @return 만든 랜드스케이프. 레벨과 레이어 인포 에셋은 저장하지 않으므로 WxPackageToolset.SavePackages 를 따로 부른다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static ALandscape* CreateLandscape(const FString& HeightmapPath, int32 SizeVerts, int32 QuadsPerSection, int32 SectionsPerComponent, FVector Location, FVector Scale, UMaterialInterface* Material, const TArray<FName>& LayerNames, const FString& WeightmapFolder, const FString& LayerInfoPath, int32 GridSizeInComponents);

	/**
	 * 기본 편집 레이어의 높이를 RAW 파일로 통째로 바꾼다. 파일 해상도는 지형 정점 수와 같아야 한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool ImportHeightmap(ALandscape* Landscape, const FString& HeightmapPath);

	/**
	 * 기본 편집 레이어의 페인트 가중치를 WeightmapFolder/<이름>.raw 로 통째로 바꾼다. 레이어는 랜드스케이프에 이미 있어야 한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool ImportWeightmaps(ALandscape* Landscape, const TArray<FName>& LayerNames, const FString& WeightmapFolder);
};
