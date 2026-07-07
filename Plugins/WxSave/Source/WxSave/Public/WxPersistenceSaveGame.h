// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "WxPersistenceSaveGame.generated.h"

/** 한 컴포넌트의 UPROPERTY(SaveGame) 직렬화 결과 바이트. UPROPERTY TMap 의 value 로 TArray 를 직접 받지 못해 wrapper. */
USTRUCT()
struct WXSAVE_API FWxComponentRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> ByteData;
};

/** 한 액터의 상태 스냅샷. Transform + 액터 본체와 컴포넌트별 UPROPERTY(SaveGame) 직렬화 결과를 보관한다. */
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

/** 맵 트래블 후 플레이어 위치를 복원하는 데 필요한 데이터. 저장 시 SaveToFile 플러시가 채우고, 로드 시 TravelFromSaveFile 과 GameMode 스폰 경로가 소비한다. */
USTRUCT()
struct WXSAVE_API FWxPersistenceTravelData
{
	GENERATED_BODY()

	/** 트래블 대상 맵. PIE 접두사를 제거한 긴 패키지 경로로 구성한다(예: /Game/Level/LV_Combat). null 은 구버전 파일/미기록 — 현재 맵 리로드로 폴백. */
	UPROPERTY()
	FSoftObjectPath Map;

	/** PawnTransform 이 유효한 저장 값인지 여부. */
	UPROPERTY()
	bool bHasPawnTransform = false;

	/** 저장 시점 첫 플레이어 폰의 월드 트랜스폼. */
	UPROPERTY()
	FTransform PawnTransform = FTransform::Identity;

	/** ControlRotation 이 유효한 저장 값인지 여부. */
	UPROPERTY()
	bool bHasControlRotation = false;

	/** 저장 시점 첫 플레이어의 컨트롤 로테이션. */
	UPROPERTY()
	FRotator ControlRotation = FRotator::ZeroRotator;
};

/** WxSave 슬롯 데이터. 슬롯 정체성 + 트래블 데이터 + savable 액터 상태 맵 + 부활/시작 PlayerStart 식별자를 보관한다. */
UCLASS()
class WXSAVE_API UWxPersistenceSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 이 SaveGame 을 SaveGameToSlot/LoadGameFromSlot 으로 식별하는 슬롯 이름. StartNewSaveFile/LoadFromFile 이 세팅한다. */
	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	int32 UserIndex = 0;

	/** 맵 트래블 후 플레이어 위치 복원용 데이터. */
	UPROPERTY()
	FWxPersistenceTravelData TravelData;

	/** PlayerStats 가 유효한 저장 값인지 여부. false 면 신규 세션/미저장 — 데이터테이블 기본 스탯 유지. */
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
	 * WxSaveId -> 스냅샷. IWxSavable::GetWxSaveId() 의 에디터-부여 영속 GUID 를 안정적 키로 사용한다(쿠킹 빌드 안전).
	 * GUID 가 맵을 넘어 전역 유일하므로 샘플(PersistenceLab)의 맵별 키잉(SavedStatePerMap) 없이 평면 맵으로 충분하다.
	 */
	UPROPERTY()
	TMap<FGuid, FWxActorRecord> ActorRecords;

	/**
	 * 마지막으로 시작/상호작용한 PlayerStart 의 PlayerStartTag. 레벨 시작 지점이자 사망 후 새 Pawn 스폰 지점으로 사용된다.
	 * (대표 사용처는 체크포인트 AWxCheckPoint=APlayerStart 지만, 일반 PlayerStart 도 동일하게 기록된다.)
	 * NAME_None 은 "미설정" sentinel — 신규 세션 + 미기록 상태와 같다(이때 기본 PlayerStart 로 폴백).
	 * 좌표가 아니라 식별자만 저장하므로 ChoosePlayerStart 가 FindPlayerStartByTag 로 실제 배치된 액터를 찾아 부활시킨다(TravelData 폰 트랜스폼의 폴백).
	 */
	UPROPERTY()
	FName PlayerStartTag = NAME_None;
};
