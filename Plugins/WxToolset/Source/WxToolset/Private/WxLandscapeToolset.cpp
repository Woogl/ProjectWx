// Copyright Woogle. All Rights Reserved.

#include "WxLandscapeToolset.h"

#include "Editor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeEdit.h"
#include "LandscapeEditLayer.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeSubsystem.h"
#include "LandscapeUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	bool LoadRaw(const FString& Path, const int64 ExpectedBytes, TArray<uint8>& OutBytes)
	{
		if (!FFileHelper::LoadFileToArray(OutBytes, *Path))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 를 읽지 못했다."), *Path));
			return false;
		}
		if (OutBytes.Num() != ExpectedBytes)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 는 %lld 바이트여야 하는데 %d 바이트다."), *Path, ExpectedBytes, OutBytes.Num()));
			return false;
		}
		return true;
	}

	bool LoadHeights(const FString& Path, const int64 NumVerts, TArray<uint16>& OutHeights)
	{
		TArray<uint8> Bytes;
		if (!LoadRaw(Path, NumVerts * sizeof(uint16), Bytes))
		{
			return false;
		}
		OutHeights.SetNumUninitialized(NumVerts);
		FMemory::Memcpy(OutHeights.GetData(), Bytes.GetData(), Bytes.Num());
		return true;
	}

	ULandscapeInfo* GetInfoAndExtent(ALandscape* Landscape, FIntRect& OutExtent)
	{
		ULandscapeInfo* LandscapeInfo = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
		if (!LandscapeInfo || !LandscapeInfo->GetLandscapeExtent(OutExtent))
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("랜드스케이프가 null 이거나 컴포넌트가 없다."));
			return nullptr;
		}
		return LandscapeInfo;
	}
}

ALandscape* UWxLandscapeToolset::CreateLandscape(const FString& HeightmapPath, int32 SizeVerts, int32 QuadsPerSection, int32 SectionsPerComponent, FVector Location, FVector Scale, UMaterialInterface* Material, const TArray<FName>& LayerNames, const FString& WeightmapFolder, const FString& LayerInfoPath, int32 GridSizeInComponents)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("에디터 월드가 없다."));
		return nullptr;
	}

	const int32 QuadsPerComponent = QuadsPerSection * SectionsPerComponent;
	if (QuadsPerComponent <= 0 || SizeVerts <= 1 || (SizeVerts - 1) % QuadsPerComponent != 0)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("SizeVerts %d 에서 1 을 뺀 값이 컴포넌트 쿼드 수 %d 의 배수가 아니다."), SizeVerts, QuadsPerComponent));
		return nullptr;
	}

	const int64 NumVerts = static_cast<int64>(SizeVerts) * SizeVerts;
	TArray<uint16> HeightData;
	if (!LoadHeights(HeightmapPath, NumVerts, HeightData))
	{
		return nullptr;
	}

	TArray<FLandscapeImportLayerInfo> ImportLayers;
	for (const FName LayerName : LayerNames)
	{
		FLandscapeImportLayerInfo& ImportLayer = ImportLayers.Emplace_GetRef(LayerName);
		if (!LoadRaw(FPaths::Combine(WeightmapFolder, LayerName.ToString() + TEXT(".raw")), NumVerts, ImportLayer.LayerData))
		{
			return nullptr;
		}

		const FString AssetName = TEXT("LI_") + LayerName.ToString();
		ImportLayer.LayerInfo = Cast<ULandscapeLayerInfoObject>(FSoftObjectPath(LayerInfoPath / AssetName + TEXT(".") + AssetName).TryLoad());
		if (!ImportLayer.LayerInfo)
		{
			ImportLayer.LayerInfo = UE::Landscape::CreateTargetLayerInfo(LayerName, LayerInfoPath, AssetName);
		}
	}

	TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
	HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
	MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(ImportLayers));

	ALandscape* Landscape = World->SpawnActor<ALandscape>(Location, FRotator::ZeroRotator);
	Landscape->LandscapeMaterial = Material;
	Landscape->SetActorRelativeScale3D(Scale);
	// 에디터 New Landscape 와 같은 식이다 — 라이트매스가 큰 지형에서 터지지 않는 LOD.
	Landscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((SizeVerts * SizeVerts) / (2048 * 2048) + 1), static_cast<uint32>(2));
	Landscape->Import(FGuid::NewGuid(), 0, 0, SizeVerts - 1, SizeVerts - 1, SectionsPerComponent, QuadsPerSection, HeightDataPerLayers, *HeightmapPath, MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
	Landscape->SetActorLabel(TEXT("Landscape"));

	World->GetSubsystem<ULandscapeSubsystem>()->ChangeGridSize(Landscape->GetLandscapeInfo(), GridSizeInComponents);
	return Landscape;
}

bool UWxLandscapeToolset::ImportHeightmap(ALandscape* Landscape, const FString& HeightmapPath)
{
	FIntRect Extent;
	ULandscapeInfo* LandscapeInfo = GetInfoAndExtent(Landscape, Extent);
	if (!LandscapeInfo)
	{
		return false;
	}

	TArray<uint16> HeightData;
	if (!LoadHeights(HeightmapPath, static_cast<int64>(Extent.Width() + 1) * (Extent.Height() + 1), HeightData))
	{
		return false;
	}

	FScopedSetLandscapeEditingLayer Scope(Landscape, Landscape->GetEditLayerConst(0)->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
	FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
	HeightmapAccessor.SetData(Extent.Min.X, Extent.Min.Y, Extent.Max.X, Extent.Max.Y, HeightData.GetData());
	return true;
}

bool UWxLandscapeToolset::ImportWeightmaps(ALandscape* Landscape, const TArray<FName>& LayerNames, const FString& WeightmapFolder)
{
	FIntRect Extent;
	ULandscapeInfo* LandscapeInfo = GetInfoAndExtent(Landscape, Extent);
	if (!LandscapeInfo)
	{
		return false;
	}

	// 파일을 모두 읽은 뒤에 쓴다 — 도중에 실패해 일부 레이어만 바뀐 채 남지 않게.
	const int64 NumVerts = static_cast<int64>(Extent.Width() + 1) * (Extent.Height() + 1);
	TArray<TPair<ULandscapeLayerInfoObject*, TArray<uint8>>> Layers;
	for (const FName LayerName : LayerNames)
	{
		ULandscapeLayerInfoObject* LayerInfo = LandscapeInfo->GetLayerInfoByName(LayerName);
		if (!LayerInfo)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("랜드스케이프에 '%s' 레이어가 없다."), *LayerName.ToString()));
			return false;
		}

		TArray<uint8>& LayerData = Layers.Emplace_GetRef(LayerInfo, TArray<uint8>()).Value;
		if (!LoadRaw(FPaths::Combine(WeightmapFolder, LayerName.ToString() + TEXT(".raw")), NumVerts, LayerData))
		{
			return false;
		}
	}

	FScopedSetLandscapeEditingLayer Scope(Landscape, Landscape->GetEditLayerConst(0)->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All); });
	for (const TPair<ULandscapeLayerInfoObject*, TArray<uint8>>& Layer : Layers)
	{
		TAlphamapAccessor<false> AlphamapAccessor(LandscapeInfo, Layer.Key);
		AlphamapAccessor.SetData(Extent.Min.X, Extent.Min.Y, Extent.Max.X, Extent.Max.Y, Layer.Value.GetData(), ELandscapeLayerPaintingRestriction::None);
	}
	return true;
}
