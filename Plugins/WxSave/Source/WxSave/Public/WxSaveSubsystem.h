// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WxSaveSubsystem.generated.h"

class USaveGame;
class UWxSlotData;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnSlotLoadedSignature, UWxSlotData* /*SlotData*/);

/**
 * 범용 세이브 서브시스템 베이스. 어느 게임에서도 그대로 사용 가능한 슬롯 기반 저장/로드 메커니즘만 제공한다.
 *
 * 게임 특화 저장 로직은 본 클래스를 상속한 프로젝트 측 서브클래스에서 OnPreSerialize / OnPostDeserialize / OnGameWorldReady / OnGameWorldCleanup 가상 hook 을 오버라이드해 구현한다.
 *
 * ShouldCreateSubsystem 패턴 덕에 파생 클래스가 존재하면 본 베이스는 인스턴스화되지 않고 파생만 생성된다.
 * 따라서 GetSubsystem<UWxSaveSubsystem>() 으로 조회하면 항상 가장 파생된 인스턴스가 반환된다.
 *
 * 사용할 SlotData/SlotInfo 클래스는 Project Settings → Plugins → WxSave 에 지정된다.
 * 미지정 시 베이스 UWxSlotData 를 사용한다.
 */
UCLASS()
class WXSAVE_API UWxSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 본 서브시스템이 사용하는 기본 슬롯 이름. */
	static const FString DefaultSlotName;

	/** 현재 메모리상 SlotData 를 지정된 슬롯에 비동기로 직렬화한다. 직렬화 직전 OnPreSerialize 가 호출된다. */
	void SaveSlot(const FString& SlotName);

	/** 슬롯을 비동기 로드한다. 완료 시 CurrentSlotData 가 교체되고 OnPostDeserialize 후 OnCurrentSlotLoaded 가 브로드캐스트된다. */
	void AsyncLoadSlot(const FString& SlotName);

	UWxSlotData* GetCurrentSlotData() const;

	/** AsyncLoadSlot 완료 시 발사. 외부 시스템이 후처리 위해 구독한다. */
	FWxOnSlotLoadedSignature OnCurrentSlotLoaded;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	/** 파생 클래스가 있으면 베이스는 인스턴스화되지 않는다. 가장 파생된 클래스만 생성. */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** 저장 직전 호출. 파생이 게임 상태에서 SlotData 로 데이터를 옮길 곳. */
	virtual void OnPreSerialize(UWxSlotData* SlotData) {}

	/** 로드 직후 호출. 파생이 SlotData 에서 게임 상태로 데이터를 적용할 곳. CurrentSlotData 가 이미 교체된 후. */
	virtual void OnPostDeserialize(UWxSlotData* SlotData) {}

	/** 게임 월드 진입 시 호출. 파생이 월드 종속 구독을 시작할 곳. */
	virtual void OnGameWorldReady(UWorld* World) {}

	/** 게임 월드 정리 시 호출. 파생이 월드 종속 구독을 해제할 곳. */
	virtual void OnGameWorldCleanup(UWorld* World) {}

private:
	void HandlePostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);

	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	void HandleAsyncLoadCompleted(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGame);

	UClass* ResolveSlotDataClass() const;

	UPROPERTY()
	TObjectPtr<UWxSlotData> CurrentSlotData;

	FDelegateHandle PostWorldInitHandle;

	FDelegateHandle WorldCleanupHandle;
};
