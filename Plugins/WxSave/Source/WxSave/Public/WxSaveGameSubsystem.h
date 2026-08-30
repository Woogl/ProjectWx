// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "WxSaveGame.h"
#include "WxSaveGameSubsystem.generated.h"

class AActor;
class UWorld;

namespace WxSave
{
	/** 부트스트랩 기본 슬롯 이름(=디스크 파일명). UI 의 명명 세이브 슬롯과 분리된 전용 슬롯으로, LoadFromFile/StartNewSaveFile 이 재지정하기 전까지 활성 슬롯이다. */
	inline const TCHAR* DefaultSaveSlotName = TEXT("Default");
}

/**
 * 메모리 SaveGame 의 수명·디스크 I/O·맵 트래블을 담당하는 GameInstance 서브시스템 (샘플 UPersistenceGameSubsystem 골격 이식).
 * 슬롯 정체성(SlotName/UserIndex)은 SaveGame 이 보유하므로 SaveToFile 은 슬롯 인자를 생략하면 활성 슬롯에 기록한다.
 * GameInstanceSubsystem 이라 SaveGame 이 맵 트래블을 가로질러 유지되고, 월드 단위 플러시/복원 오케스트레이션은 UWxSaveWorldSubsystem 이 맡는다.
 * Initialize 가 활성 SaveGame 을 항상 보장한다: 모드(PIE/스탠드얼론/패키지) 무관하게 항상 빈 새 파일로 시작하고, 이후 로드는 UI 의 LoadFromFile 몫이다.
 */
UCLASS()
class WXSAVE_API UWxSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UWxSaveGame* GetSaveGame() const;

	/** LoadFromFile 트래블 시작 ~ 새 월드 복원 완료(BeginPlay 보고) 사이 true. 월드 서브시스템의 자동 캡처가 이 동안 스킵돼 막 로드한 세이브의 오염을 막는다. */
	bool IsTravelingFromSaveFile() const;

	/** SpecificClass 의 새 SaveGame 을 만들어 활성 슬롯으로 등록한다. SlotName 은 그대로 슬롯 정체성이 되므로 유효한 이름을 넘겨야 한다. @return 생성 실패 시 nullptr. */
	UWxSaveGame* StartNewSaveFile(const FString& SlotName, int32 UserIndex, TSubclassOf<UWxSaveGame> SpecificClass);

	/**
	 * 슬롯 파일을 메모리로 로드하고 bStartTravel 이면 TravelFromSaveFile 로 이어간다.
	 * SlotName 이 비면 활성 슬롯을 다시 읽는다(사망 리스폰 경로 — 저장 안 된 변경을 버리고 마지막 세이브로 재개). SaveToFile 의 빈 슬롯 규칙과 대칭이다.
	 * 파일 부재/손상 시 같은 슬롯의 새 SaveGame 으로 리셋한 뒤에도 동일하게 트래블한다(샘플과 달리 중단하지 않음 — 사망 리스폰이 파일 없이도 월드 리로드에 의존).
	 * @return 활성 SaveGame(항상 유효). 파일 존재 여부는 로그로 구분한다.
	 */
	UWxSaveGame* LoadFromFile(const FString& SlotName = FString(), int32 UserIndex = 0, bool bStartTravel = true);

	/**
	 * TravelData.Map 으로 ServerTravel 한다(authority 전제). Map 이 비면(구버전 파일/신규 슬롯) 현재 맵 리로드로 폴백한다(샘플은 경고 후 중단 — Wx 는 사망 리스폰 경로가 리로드에 의존).
	 * 액터 복원은 새 월드의 UWxSaveWorldSubsystem 이, 플레이어 스폰은 UWxPlayerSpawnComponent 가 PlayerTransform 우선 + ChoosePlayerStart 폴백으로 담당한다.
	 */
	void TravelFromSaveFile();

	/**
	 * 저장을 시작한다: 월드 서브시스템의 RequestSaveFlush 로 라이브 상태(트래블 데이터 + savable 액터)를 SaveGame 에 플러시한 뒤 디스크에 기록한다.
	 * 월드 서브시스템이 없으면(트랜지션 등) 플러시 없이 바로 기록한다. 활성 SaveGame 이 없으면 경고 후 중단.
	 * SlotName 이 비면 활성 슬롯에 그대로 기록하고(체크포인트 오토세이브 경로), 지정되면 활성 슬롯을 그 이름으로 재지정한 뒤 기록한다(이후 저장도 그 슬롯을 이어감).
	 * ResumeTransform 이 주어지면 재개 지점을 그 값으로 확정한다(체크포인트가 자기 자리를 넘기는 경로). null 이면 플러시가 저장 시점 폰 위치를 캡처한다.
	 * 앞선 기록이 아직 끝나지 않았으면 거절한다 — 겹친 기록은 엔진이 순서를 보장하지 않아 더 오래된 스냅샷이 최종 파일로 남을 수 있다.
	 * @return 요청을 접수했는가. 거절이면 완료를 기다려선 안 된다 — 그 신호는 남의 기록을 가리킨다.
	 */
	bool SaveToFile(const FString& SlotName = FString(), int32 UserIndex = 0, const FTransform* ResumeTransform = nullptr);

	bool DoesSaveFileExist(const FString& SlotName, int32 UserIndex) const;

	/** 인메모리 활성 SaveGame 은 건드리지 않으므로, 활성 슬롯을 지웠다면 다음 SaveToFile 이 그 파일을 다시 만든다.
	* @return 삭제 성공 여부(파일 없음 등 실패 시 false). */
	bool DeleteSaveFile(const FString& SlotName, int32 UserIndex);

	void SetTravelData(FWxSaveTravelData InTravelData);

	/**
	 * 저장된 재개 지점(마지막 저장 시점의 플레이어 트랜스폼)이 유효하면 OutTransform 에 채우고 true. 없으면(신규 세션/미저장/맵 불일치) false.
	 * 유효성은 Identity(미설정) sentinel 로 판정한다(별도 bool 플래그 없음).
	 * 좌표는 맵 종속이라 저장 맵이 World 와 일치할 때만 유효하다(정상 로드-트래블은 같은 맵으로 오므로 통과, 크로스맵 오적용만 차단).
	 */
	bool TryGetPlayerTransform(const UWorld* World, FTransform& OutTransform) const;

	/** 저장된 플레이어 스탯이 있으면 PlayerActor 의 ASC 어트리뷰트에 복원한다. 저장 스탯이 없으면(신규 세션) noop — 데이터테이블 기본 스탯이 유지된다. */
	void ApplySavedPlayerStats(AActor* PlayerActor) const;

	/** 새 월드의 UWxSaveWorldSubsystem::OnWorldBeginPlay 가 호출 — 트래블 완료를 보고받아 가드를 해제한다. */
	void ReportTravelFromSaveFileComplete(UWorld* World);

	/** PIE 접두사를 제거한 월드의 긴 패키지 이름. 세이브의 맵 키 표현을 한 곳에서 강제한다(트래블 데이터 스탬프·일치 판정 공유). */
	static FName GetStableMapPackageName(const UWorld* World);

	/** 콘솔 명령 Wx.Save.Dump 의 구현. */
	void LogSaveState() const;

	/**
	 * SaveToFile 이 요청한 기록이 아직 끝나지 않았는가. 직렬화는 동기지만 디스크 쓰기는 비동기라, 요청 직후엔 아직 파일이 없다.
	 * 기다려야 하는 쪽('Save Game' ST 태스크)은 요청 직후 이것으로 이미 끝났는지 가르고, 아니면 OnSaveCompleted 를 기다린다.
	 */
	bool IsSaveInProgress() const;

	/**
	 * 진행 중이던 기록이 끝나면 한 번 발화하고 스스로 비워진다. 기다리는 쪽이 요청 직후 붙인다.
	 */
	FSimpleMulticastDelegate OnSaveCompleted;

private:
	/** SaveToFile 에서 직접(월드 서브시스템 부재) 또는 RequestSaveFlush 완료 콜백으로 호출된다. */
	void ContinueSaveToFileToDisk();

	/** 성공·실패·중단 어느 경로든 여기로 모인다. */
	void FinishSaveInProgress();

	bool bSaveInProgress = false;

	UPROPERTY()
	TObjectPtr<UWxSaveGame> SaveGame;

	bool bTravelingFromSaveFile = false;
};
