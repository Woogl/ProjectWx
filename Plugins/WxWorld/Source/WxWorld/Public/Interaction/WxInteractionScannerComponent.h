// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Engine/TimerHandle.h"
#include "WxInteractionScannerComponent.generated.h"

class AActor;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionListChanged, const TArray<FText>&, Prompts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionSelectionChanged, int32, SelectedIndex);

/** 소유 액터는 인자로 준 컴포넌트에서 얻는다. */
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnScannerReady, UWxInteractionScannerComponent* /*Scanner*/);

/**
 * AWxPlayerController 에 붙어, 소유 클라(리슨호스트 포함)에서 주변 상호작용 액터를 주기 스캔해 in-range 집합을 모은다.
 * HUD 리스트 뷰모델(UWxViewModel_InteractionList)이 이 목록과 선택 인덱스를 함께 표시한다.
 *
 * 후보는 액터 단위다 — 겹친 컴포넌트를 소유 액터로 모아 중복을 없앤 뒤, 대상이 IWxInteractable 로 지금 켜져 있는지 답하고 꺼지면 다음 스캔에서 탈락한다.
 * 주변 후보는 반경 구 오버랩으로 모으되 전 오브젝트 채널로 던지므로, 대상 자격은 콜리전 프리셋·응답과 무관하다(쿼리 콜리전만 켜져 있으면 된다). 응답·프롬프트도 같은 인터페이스가 제공한다.
 *
 * PlayerController 소유인 이유: 폰 리스폰에도 생존하고, 소유 클라 연결로 net-owned 라 ServerInteract RPC 를 직접 들 수 있으며, 타 클라에 복제되지 않아 로컬리티가 좋다.
 * 감지·선택·하이라이트는 로컬 어포던스라 소유 클라에서만 구동한다(데디 서버 PC 는 스캔하지 않는다). ServerInteract 수신만 서버에서 실행된다.
 *
 * 입력 수신: 본 컴포넌트는 입력을 직접 바인딩하지 않는다. HUD 리스트 위젯이 Enhanced Input 으로 받아 리스트 뷰모델에 넘기고, 뷰모델이 TryInteractSelected/CycleSelection 을 호출한다.
 * 선택 전달: 입력 시 로컬 선택을 읽어 ServerInteract 로 액터 포인터를 원자 전송한다(선택을 복제하지 않으므로 "사이클→즉시입력" 순서가 로컬 동기 읽기로 보장된다).
 * 서버는 Event.Interact(OptionalObject=선택)를 폰 ASC 로 송출해 ServerOnly WxAbility_Interact 가 권위에서 사거리·활성 검증 후 대상 인터페이스를 호출하게 한다.
 *
 * 부착은 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 설정으로 한다(컨트롤러는 본 클래스를 모른다).
 */
UCLASS()
class WXWORLD_API UWxInteractionScannerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UWxInteractionScannerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 리슨호스트에선 ServerInteract 가 로컬 권위 호출이 된다. */
	void TryInteractSelected();

	/** 뷰모델이 초기 시드로 읽는다. */
	TArray<FText> GetPrompts() const;

	/** 현재 선택 인덱스(없으면 INDEX_NONE). 뷰모델이 초기 시드로 읽는다. */
	int32 GetSelectedIndex() const;

	AActor* GetSelectedActor() const;

	void CycleSelection(int32 Delta);

	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionListChanged OnListChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionSelectionChanged OnSelectionChanged;

	/**
	 * 스캐너가 쓸 수 있게 될 때마다 발행된다. 주입(서버)·복제 도착(클라) 어느 경로든 BeginPlay 로 수렴한다.
	 * 관찰자가 스캐너보다 먼저 존재할 수 있어(HUD 뷰모델) 인스턴스가 아니라 클래스 차원에 둔다 — 구독자는 소유 액터로 자기 것인지 가린다.
	 */
	static FWxOnScannerReady OnAnyScannerReady;

protected:
	/** 주변 상호작용 액터를 수집할 반경(cm). 이 반경의 구를 오버랩해 후보를 모으므로, 서버 사거리 검증(WxAbility_Interact)의 반경과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanRadius = 150.f;

	/** 스캔 주기(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanInterval = 0.1f;

	/** 외곽선 강조용 Custom Depth Stencil 값. 포스트프로세스 아웃라인 머티리얼이 비교하는 값과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact", meta = (ClampMin = 0, ClampMax = 255))
	int32 HighlightStencilValue = 1;

private:
	/** 서버가 Event.Interact 를 폰 ASC 로 송출해 권위 실행을 시작한다. */
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* Selected);

	/**
	 * 소유 클라에서 주기 타이머로 호출된다.
	 * 상호작용 불가면 후보를 비워 프롬프트·하이라이트를 정리한다.
	 */
	void ScanAndPush();

	/**
	 * 후보 집합으로 in-range 멤버십을 갱신한다. 기존 순서 보존·신규만 뒤에 추가·이탈은 제거.
	 * 멤버십이 실제로 바뀐 경우에만 강조·선택을 갱신·발화하고, 목록(프롬프트)은 문구 스냅샷이 달라졌을 때 발화한다.
	 */
	void UpdateInRange(const TArray<AActor*>& InCandidates);

	void UpdateSelection(int32 NewIndex);

	/** 외곽선을 쓰는 유일한 주체다(선택을 소유하므로). */
	void ApplyHighlight();

	/** 액터의 보이는 프리미티브에 모두 건다 — 어느 메시가 대상인지 가리지 않는 것이 액터 단위 계약이다. */
	void SetActorHighlighted(AActor* Actor, bool bHighlighted) const;

	/**
	 * 상호작용 어빌리티(Ability.Interact 애셋 태그)를 찾아 그 CanActivateAbility 로 현재 상호작용 가능 여부를 판정한다.
	 * 차단 조건의 단일 소스는 어빌리티(ActivationBlockedTags 등)이므로 컴포넌트가 상태 태그를 하드코딩하지 않는다.
	 */
	bool CanActivateInteract(const UAbilitySystemComponent* ASC) const;

	/** 스캔 원점·이벤트 instigator 로 쓴다. */
	APawn* GetOwnerPawn() const;

	TArray<TWeakObjectPtr<AActor>> InRangeActors;

	/** 마지막으로 내보낸 프롬프트 스냅샷. 대상이 pull 로 주는 문구가 실제로 달라졌을 때만 OnListChanged 를 발화하려고 든다. */
	TArray<FText> LastPrompts;

	int32 SelectedIndex = INDEX_NONE;

	FTimerHandle ScanTimerHandle;
};
