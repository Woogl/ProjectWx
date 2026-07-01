// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxBGMChooserContext.h"
#include "WxMusicSubsystem.generated.h"

class APawn;
class APlayerController;
class UAudioComponent;
class UChooserTable;
class UWxBGMData;

/**
 * 게임 상태(전투/보스/플레이어 상태태그/지역)를 입력으로 Chooser 테이블을 평가해 적절한 BGM 을 골라 재생하는 월드 서브시스템.
 *
 * 주기 타이머(설정값) + 이벤트(지역 변경/폰 교체)로 재평가하고, 선택된 UWxBGMData 가 직전 곡과 다를 때만 크로스페이드한다.
 * BGM 은 로컬 전용이므로 데디케이티드 서버에서는 동작하지 않으며, 로컬 플레이어의 ASC/상태만 읽는다.
 * 진입점은 UWxMusicLibrary(Blueprint) 가 제공한다.
 */
UCLASS()
class WXSOUND_API UWxMusicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** BGM 분류 태그를 설정하고 즉시 재평가해 Chooser 가 고른 곡을 재생한다(StopBGM 보류도 해제). */
	void StartBGM(const FGameplayTag& InBGMTag);

	/** 재생 중인 BGM 을 페이드아웃하고, 다음 명시 요청 전까지 선택을 보류한다. */
	void StopBGM();

private:
	void HandleReevaluate();

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 로컬 PlayerController 의 폰 교체 델리게이트에 한 번 바인딩한다(폰이 늦게 생기는 경우 대비, 매 재평가에서 멱등 호출). */
	void BindLocalController();

	/** 선택 결과를 적용한다. 직전 곡과 같으면 no-op, 다르면 크로스페이드. nullptr 이면 페이드아웃. */
	void ApplyBGM(UWxBGMData* NewBGM);

	/** 컨텍스트를 채우고 Chooser 를 평가해 BGM 을 고른다. */
	UWxBGMData* EvaluateBGM();

	APawn* GetLocalPlayerPawn() const;

	// 평가 컨텍스트. AddStructParam 이 참조만 저장하므로 멤버로 두어 평가 호출 동안 수명을 보장한다.
	FWxBGMChooserContext ChooserContext;

	// BP StartBGM 으로 주입된 현재 BGM 분류 태그.
	FGameplayTag BGMTag;

	UPROPERTY()
	TObjectPtr<UChooserTable> Chooser;

	UPROPERTY()
	TObjectPtr<UWxBGMData> CurrentBGM;

	// 현재 재생 중(페이드 인) 컴포넌트.
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentComponent;

	// 페이드 아웃 중인 직전 컴포넌트. 페이드 동안 GC 를 막기 위해 보유한다.
	UPROPERTY()
	TObjectPtr<UAudioComponent> PreviousComponent;

	TWeakObjectPtr<APlayerController> BoundController;

	FTimerHandle ReevaluateTimerHandle;

	// StopBGM 으로 보류된 상태. StartBGM 호출 시 해제된다.
	bool bSuspended = false;
};
