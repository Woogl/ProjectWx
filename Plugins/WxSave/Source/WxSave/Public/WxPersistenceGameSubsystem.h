// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "WxPersistenceSaveGame.h"
#include "WxPersistenceGameSubsystem.generated.h"

class UWorld;

namespace WxPersistence
{
	/** 개발 단계 기본 슬롯 이름. 체크포인트 저장·PIE 자동 로드·UI 가 공유한다. */
	inline constexpr const TCHAR* DefaultSlotName = TEXT("Test");
}

/**
 * 메모리 SaveGame 의 수명·디스크 I/O·맵 트래블을 담당하는 GameInstance 서브시스템 (샘플 UPersistenceGameSubsystem 골격 이식).
 * 슬롯 정체성(SlotName/UserIndex)은 SaveGame 이 보유하므로 SaveToFile 은 무인자다.
 * GameInstanceSubsystem 이라 SaveGame 이 맵 트래블을 가로질러 유지되고, 월드 단위 플러시/복원 오케스트레이션은 UWxPersistenceWorldSubsystem 이 맡는다.
 * Initialize 가 활성 SaveGame 을 항상 보장한다: PIE 는 기본 슬롯 자동 로드(실패 시 새 파일), 그 외는 새 파일 시작.
 */
UCLASS()
class WXSAVE_API UWxPersistenceGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UWxPersistenceSaveGame* GetSaveGame() const { return SaveGame; }

	/** LoadFromFile 트래블 시작 ~ 새 월드 복원 완료(BeginPlay 보고) 사이 true. 월드 서브시스템의 자동 캡처가 이 동안 스킵돼 막 로드한 세이브의 오염을 막는다. */
	bool IsTravelingFromSaveFile() const { return bTravelingFromSaveFile; }

	/** SpecificClass 의 새 SaveGame 을 만들어 활성 슬롯으로 등록한다. SlotName 이 비면 기본 슬롯 이름을 쓴다. @return 생성 실패 시 nullptr. */
	UWxPersistenceSaveGame* StartNewSaveFile(const FString& SlotName, int32 UserIndex, TSubclassOf<UWxPersistenceSaveGame> SpecificClass);

	/**
	 * 슬롯 파일을 메모리로 로드하고 bStartTravel 이면 TravelFromSaveFile 로 이어간다.
	 * 파일 부재/손상 시 같은 슬롯의 새 SaveGame 으로 리셋한 뒤에도 동일하게 트래블한다(샘플과 달리 중단하지 않음 — 사망 리스폰이 파일 없이도 월드 리로드에 의존).
	 * @return 활성 SaveGame(항상 유효). 파일 존재 여부는 로그로 구분한다.
	 */
	UWxPersistenceSaveGame* LoadFromFile(const FString& SlotName, int32 UserIndex, bool bStartTravel = true);

	/** 활성 SaveGame 의 슬롯을 다시 로드해 저장 안 된 변경을 버린다. 활성 SaveGame 이 없으면 경고 후 nullptr. */
	UWxPersistenceSaveGame* ReloadFromFile(bool bStartTravel = true);

	/**
	 * TravelData.Map 으로 ServerTravel 한다(authority 전제). Map 이 비면(구버전 파일/신규 슬롯) 현재 맵 리로드로 폴백한다(샘플은 경고 후 중단 — Wx 는 사망 리스폰 경로가 리로드에 의존).
	 * 액터 복원은 새 월드의 UWxPersistenceWorldSubsystem 이, 플레이어 스폰은 GameMode 가 TravelData 폰 트랜스폼 우선 + PlayerStartTag 폴백으로 담당한다.
	 */
	void TravelFromSaveFile();

	/**
	 * 저장을 시작한다: 월드 서브시스템의 RequestSaveFlush 로 라이브 상태(트래블 데이터 + savable 액터)를 SaveGame 에 플러시한 뒤 디스크에 기록한다.
	 * 월드 서브시스템이 없으면(트랜지션 등) 플러시 없이 바로 기록한다. 활성 SaveGame 이 없으면 경고 후 중단.
	 */
	void SaveToFile();

	void SetPersistenceTravelData(FWxPersistenceTravelData InTravelData);

	/** 부활/시작 진입점 PlayerStartTag 를 메모리 슬롯에 기록한다(다음 SaveToFile 이 디스크 영속). 레벨 시작·체크포인트 상호작용 시 호출된다. */
	void SetPlayerStartTag(FName InPlayerStartTag);

	/** GameMode 스폰 경로가 호출. 저장된 부활/시작 PlayerStartTag 를 반환한다(미설정이면 NAME_None). */
	FName GetPlayerStartTag() const;

	/** 새 월드의 UWxPersistenceWorldSubsystem::OnWorldBeginPlay 가 호출 — 트래블 완료를 보고받아 가드를 해제한다. */
	void ReportTravelFromSaveFileComplete(UWorld* World);

	/** PIE 접두사를 제거한 월드의 긴 패키지 이름. 세이브의 맵 키 표현을 한 곳에서 강제한다(트래블 데이터 스탬프·일치 판정 공유). */
	static FName GetStableMapPackageName(const UWorld* World);

	/** 현재 메모리 슬롯 상태(슬롯·맵·폰·레코드 목록)를 LogWxSave 로 덤프한다. 콘솔 명령 Wx.Save.Dump 의 구현. */
	void LogSaveState() const;

private:
	/** SaveGame 을 자기 슬롯에 기록한다. SaveToFile 에서 직접(월드 서브시스템 부재) 또는 RequestSaveFlush 완료 콜백으로 호출된다. */
	void ContinueSaveToFileToDisk();

	UPROPERTY()
	TObjectPtr<UWxPersistenceSaveGame> SaveGame;

	/** 프레임을 넘는 수명 상태(트래블 시작~새 월드 복원 완료)라 멤버로 둔다. */
	bool bTravelingFromSaveFile = false;
};
