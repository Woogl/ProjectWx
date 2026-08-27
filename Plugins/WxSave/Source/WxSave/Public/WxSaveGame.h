// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "WxSaveGame.generated.h"

/** 한 컴포넌트의 UPROPERTY(SaveGame) 직렬화 결과 바이트. UPROPERTY TMap 의 value 로 TArray 를 직접 받지 못해 wrapper. */
USTRUCT()
struct WXSAVE_API FWxComponentRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> ByteData;
};

USTRUCT()
struct WXSAVE_API FWxActorRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	UPROPERTY()
	TArray<uint8> ByteData;

	UPROPERTY()
	TMap<FName, FWxComponentRecord> ComponentData;

	/**
	 * [FPackageFileVersion][FCustomVersionContainer] 직렬화 헤더 블롭. 두 타입 모두 UPROPERTY 리플렉션이 없어 별도 바이트 블롭으로 보관한다.
	 * 복원 시 리더에 먼저 적용해 맨 FMemoryReader 의 커스텀 버전 리셋(현재 빌드 값으로 초기화) 함정을 막는다.
	 * 액터+컴포넌트 블롭은 CaptureActor 에서 한 빌드로 원자적으로 쓰이므로 레코드당 1개면 충분하다(레코드는 세션을 넘어 이기종 빌드로 누적되므로 파일 단위는 불가).
	 * 빈 배열은 구버전 레코드 — 버전 적용 없이 기존처럼 현재 빌드 버전으로 읽는다.
	 */
	UPROPERTY()
	TArray<uint8> VersionHeader;
};

/** 저장 시 SaveToFile 플러시가 대상 맵을 채우고, 로드 시 TravelFromSaveFile 이 그 맵으로 트래블한다. */
USTRUCT()
struct WXSAVE_API FWxSaveTravelData
{
	GENERATED_BODY()

	/** 트래블 대상 맵. PIE 접두사를 제거한 긴 패키지 경로로 구성한다(예: /Game/Maps/LV_DevCombat). null 은 구버전 파일/미기록 — 현재 맵 리로드로 폴백. */
	UPROPERTY()
	FSoftObjectPath Map;
};

UCLASS()
class WXSAVE_API UWxSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 이 SaveGame 을 AsyncSaveGameToSlot/LoadGameFromSlot 으로 식별하는 슬롯 이름. StartNewSaveFile/LoadFromFile 이 세팅한다. */
	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	int32 UserIndex = 0;

	/** PlayerTransform 유효성의 맵 일치 게이트 기준이기도 하다. */
	UPROPERTY()
	FWxSaveTravelData TravelData;

	/** false 면 신규 세션/미저장 — 데이터테이블 기본 스탯 유지. */
	UPROPERTY()
	bool bHasPlayerStats = false;

	/**
	 * 플레이어 캐릭터 어트리뷰트 스냅샷(어트리뷰트 프로퍼티 이름 -> base 값). 체크포인트 저장 시 캡처, 로드 후 스폰 경로가 적용한다.
	 * TravelData 밖 최상위에 두어 SaveToFile 의 FlushMapTravelData(TravelData 재구성)가 덮어쓰지 않게 한다.
	 * 좌표와 달리 스탯은 맵 무관이라 맵 일치 게이트 없이 bHasPlayerStats 만으로 적용한다.
	 */
	UPROPERTY()
	TMap<FName, float> PlayerStats;

	/**
	 * 마지막 저장 시점의 재개 지점. 로드 후 새 Pawn 스폰 지점으로 사용된다 — 로드도 사망 부활도 이 값 하나로 재개한다.
	 * 체크포인트 오토세이브는 그 체크포인트의 트랜스폼을, 메뉴의 명시 저장은 저장 시점 플레이어 폰의 트랜스폼을 담는다.
	 * Identity 는 "미설정" sentinel(신규 세션 + 미저장) — 이때 스폰은 엔진 ChoosePlayerStart(레벨의 APlayerStart)로 폴백한다.
	 * 값을 채우는 것은 언제나 SaveToFile 플러시다 — 외부는 이 필드를 직접 세팅하지 않는다.
	 */
	UPROPERTY()
	FTransform PlayerTransform = FTransform::Identity;

	/**
	 * WxSaveId -> 스냅샷. IWxSavable::GetSaveId() 의 에디터-부여 영속 GUID 를 안정적 키로 사용한다(쿠킹 빌드 안전).
	 * GUID 가 맵을 넘어 전역 유일하므로 샘플(PersistenceLab)의 맵별 키잉(SavedStatePerMap) 없이 평면 맵으로 충분하다.
	 */
	UPROPERTY()
	TMap<FGuid, FWxActorRecord> ActorRecords;
};
