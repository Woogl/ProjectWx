// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxBGMChooserContext.h"
#include "WxMusicSubsystem.generated.h"

class APawn;
class APlayerController;
class ULocalPlayer;
class UAbilitySystemComponent;
class UAudioComponent;
class UChooserTable;
class UWxBGMData;

/** 활성 BGM 소스 한 건의 등록 정보. 배열 순서로 최근성을 표현하므로 UPROPERTY 없이 TWeakObjectPtr 로 보유한다. */
struct FWxBGMSourceRequest
{
	TWeakObjectPtr<UObject> Source;
	// 소스 컴포넌트의 소유자 액터. Chooser 가 클래스로 필터할 수 있게 컨텍스트로 전달한다.
	TWeakObjectPtr<AActor> Owner;
	FGameplayTag MusicTag;
	int32 Priority = 0;
};

/**
 * 게임 상태(플레이어 상태태그 + BGM 분류 태그)를 입력으로 Chooser 테이블을 평가해 적절한 BGM 을 골라 재생하는 월드 서브시스템.
 *
 * 재평가는 완전 이벤트 구동이다: BGM 분류 태그 주입(StartBGM), 로컬 플레이어 폰 교체(OnPossessedPawnChanged),
 * 폰 ASC 의 owned-tag 변경(RegisterGenericGameplayTagEvent) 시점에만 재평가하고, 선택된 UWxBGMData 가 직전 곡과
 * 다를 때만 크로스페이드한다(같으면 no-op). 로컬 플레이어/컨트롤러의 늦은 스폰은 OnLocalPlayerAddedEvent 체인으로
 * 잡으므로 주기 폴링이 없다.
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

	/** 상태 기반 BGM 소스가 활성화될 때 자신을 등록한다(같은 Source 는 갱신). 등록/해제 즉시 재평가한다. */
	void RegisterBGMSource(UObject* Source, const FGameplayTag& InMusicTag, int32 InPriority);

	/** 활성화가 끝난 BGM 소스를 해제한다. */
	void UnregisterBGMSource(UObject* Source);

private:
	/** 보류 중이 아니면 Chooser 를 평가해 결과를 적용한다. 모든 입력 이벤트가 이 함수로 수렴한다. */
	void Reevaluate();

	/** GameInstance 의 로컬 플레이어(기존/신규)마다 컨트롤러 교체 델리게이트를 건다. */
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);

	/** 로컬 플레이어의 컨트롤러가 설정/교체될 때 폰 교체 델리게이트를 재바인딩한다. */
	void HandlePlayerControllerChanged(APlayerController* PC);

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 폰 ASC 의 owned-tag 변경 콜백. 곧바로 재평가한다. */
	void HandleOwnedTagsChanged(const FGameplayTag Tag, int32 NewCount);

	/** generic tag 이벤트 구독을 새 폰의 ASC 로 옮긴다(기존 구독 해제 포함). */
	void RebindAbilitySystem(APawn* NewPawn);

	/** 선택 결과를 적용한다. 직전 곡과 같으면 no-op, 다르면 크로스페이드. nullptr 이면 페이드아웃. */
	void ApplyBGM(UWxBGMData* NewBGM);

	/** 컨텍스트를 채우고 Chooser 를 평가해 BGM 을 고른다. */
	UWxBGMData* EvaluateBGM();

	/** 활성 소스 중 최고 우선순위(동률은 최근 등록)의 유효 소스를 반환한다. 활성 소스가 없으면 nullptr. */
	const FWxBGMSourceRequest* GetTopSource() const;

	// 평가 컨텍스트. AddStructParam 이 참조만 저장하므로 멤버로 두어 평가 호출 동안 수명을 보장한다.
	FWxBGMChooserContext ChooserContext;

	// BP StartBGM 으로 주입된 베이스라인 BGM 분류 태그(활성 소스가 없을 때의 폴백).
	FGameplayTag BGMTag;

	// 활성 상태 기반 소스들의 등록 집합. 배열 순서 = 등록 순(동률 우선순위 시 뒤쪽=최근이 이긴다).
	TArray<FWxBGMSourceRequest> ActiveSources;

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

	// 폰 교체 델리게이트를 건 현재 컨트롤러.
	TWeakObjectPtr<APlayerController> BoundController;

	// owned-tag 이벤트를 구독 중인 현재 폰의 ASC. 폰 교체 시 옮겨 단다.
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	// StopBGM 으로 보류된 상태. StartBGM 호출 시 해제된다.
	bool bSuspended = false;
};
