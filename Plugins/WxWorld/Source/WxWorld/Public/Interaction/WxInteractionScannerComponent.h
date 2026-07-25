// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "WxInteractionScannerComponent.generated.h"

class UPrimitiveComponent;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionListChanged, const TArray<FText>&, Prompts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractionSelectionChanged, int32, SelectedIndex);

/**
 * 상호작용 스캐너 컴포넌트.
 * AWxPlayerController 에 붙어, 소유 클라(리슨호스트 포함)에서 주변 상호작용 메시를 주기 스캔해 in-range 집합을 모은다.
 * HUD 리스트 뷰모델(UWxViewModel_InteractionList)이 이 목록을, 전역 선택 VM(UWxViewModel_Selection)이 선택 항목을 표시한다.
 *
 * 상호작용 영역은 대상 액터의 메시 그 자체다 — 대상이 IWxInteractable 로 그 메시가 지금 켜져 있는 영역인지 답하고, 꺼지면 다음 스캔에서 탈락한다.
 * 주변 후보는 반경 구 오버랩으로 모으되 전 오브젝트 채널로 던지므로, 대상 자격은 콜리전 프리셋·응답과 무관하다(쿼리 콜리전만 켜져 있으면 된다). 응답·프롬프트도 같은 인터페이스가 제공한다.
 *
 * PlayerController 소유인 이유: 폰 리스폰에도 생존하고, 소유 클라 연결로 net-owned 라 ServerInteract RPC 를 직접 들 수 있으며, 타 클라에 복제되지 않아 로컬리티가 좋다.
 * 감지·선택·하이라이트는 로컬 어포던스라 소유 클라에서만 구동한다(데디 서버 PC 는 스캔하지 않는다). ServerInteract 수신만 서버에서 실행된다.
 *
 * 입력 수신: 본 컴포넌트는 입력을 직접 바인딩하지 않는다. HUD 리스트 위젯이 Enhanced Input 으로 받아 리스트 뷰모델에 넘기고, 뷰모델이 TryInteractSelected/CycleSelection 을 호출한다.
 * 선택 전달: 입력 시 로컬 선택을 읽어 ServerInteract 로 메시 포인터를 원자 전송한다(선택을 복제하지 않으므로 "사이클→즉시입력" 순서가 로컬 동기 읽기로 보장된다).
 * 서버는 Event.Interact(OptionalObject=선택)를 폰 ASC 로 송출해 ServerOnly WxAbility_Interact 가 권위에서 사거리·활성 검증 후 대상 인터페이스를 호출하게 한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxInteractionScannerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxInteractionScannerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 상호작용 실행 진입점. 로컬 선택을 읽어 ServerInteract 로 전송한다.
	 * 선택이 없으면 무동작. 리슨호스트에선 ServerInteract 가 로컬 권위 호출이 된다.
	 */
	void TryInteractSelected();

	/** 현재 인-레인지 메시들의 프롬프트 텍스트를 순서대로 반환한다. 리졸버가 초기 시드로 읽는다. */
	TArray<FText> GetPrompts() const;

	/** 현재 선택 인덱스(없으면 INDEX_NONE). 리졸버가 초기 시드로 읽는다. */
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/** 현재 선택된 인-레인지 메시(없으면 nullptr). */
	UPrimitiveComponent* GetSelectedMesh() const;

	/** 선택을 Delta 만큼 순환 이동한다(휠/방향키). 목록이 비면 무시. */
	void CycleSelection(int32 Delta);

	/** 인-레인지 목록 변경 시 발사. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionListChanged OnListChanged;

	/** 선택 인덱스 변경 시 발사. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractionSelectionChanged OnSelectionChanged;

protected:
	/** 주변 상호작용 메시를 수집할 반경(cm). 이 반경의 구를 오버랩해 후보를 모으므로, 서버 사거리 검증(WxAbility_Interact)의 반경과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanRadius = 150.f;

	/** 스캔 주기(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanInterval = 0.1f;

	/** 외곽선 강조용 Custom Depth Stencil 값. 포스트프로세스 아웃라인 머티리얼이 비교하는 값과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact", meta = (ClampMin = 0, ClampMax = 255))
	int32 HighlightStencilValue = 1;

private:
	/** 선택을 서버로 전송한다. 서버가 Event.Interact 를 폰 ASC 로 송출해 권위 실행을 시작한다. */
	UFUNCTION(Server, Reliable)
	void ServerInteract(UPrimitiveComponent* Selected);

	/**
	 * 소유 클라에서만 주기 스캔 타이머를 건다. 폰 주위 ScanRadius 구를 오버랩해 겹친 컴포넌트 중 IWxInteractable 의 활성 영역인 것만 남기고 거리순으로 UpdateInRange 한다.
	 * 겹쳤다는 사실이 곧 사거리 판정이라 메시별 사거리 재측정은 하지 않는다(오버랩 구가 IsMeshInRange 와 같은 원점·반경·형상이다).
	 * 상호작용 불가(사망·처형 중)면 후보를 비워 프롬프트·하이라이트를 정리한다.
	 */
	void ScanAndPush();

	/**
	 * 후보 집합으로 in-range 멤버십을 갱신한다. 기존 순서 보존·신규만 뒤에 추가·이탈은 제거.
	 * 멤버십이 실제로 바뀐 경우에만 강조/목록/선택을 갱신·발화한다(불변이면 침묵).
	 */
	void UpdateInRange(const TArray<UPrimitiveComponent*>& InCandidates);

	/** 선택 인덱스를 갱신하고(변경 시) 강조 갱신 + 선택 변경을 알린다. */
	void UpdateSelection(int32 NewIndex);

	/** 선택된 메시만 외곽선 강조 ON, 나머지는 OFF. 외곽선을 쓰는 유일한 주체다(선택을 소유하므로). */
	void ApplyHighlight();

	/** 지정 메시의 외곽선을 켜고 끈다. */
	void SetMeshHighlighted(UPrimitiveComponent* Mesh, bool bHighlighted) const;

	/**
	 * 상호작용 어빌리티(Ability.Interact 애셋 태그)를 찾아 그 CanActivateAbility 로 현재 상호작용 가능 여부를 판정한다.
	 * 차단 조건의 단일 소스는 어빌리티(ActivationBlockedTags 등)이므로 컴포넌트가 상태 태그를 하드코딩하지 않는다.
	 * 어빌리티가 아직 부여되지 않았으면 true(표시를 열어둔다).
	 */
	bool CanInteractNow(const UAbilitySystemComponent* ASC) const;

	/** 소유 PC 의 현재 폰(없으면 nullptr). 스캔 원점·이벤트 instigator 로 쓴다. */
	APawn* GetOwnerPawn() const;

	TArray<TWeakObjectPtr<UPrimitiveComponent>> InRangeMeshes;

	int32 SelectedIndex = INDEX_NONE;

	/** 주기 스캔 타이머 핸들. BeginPlay(로컬)에서 설정, EndPlay 에서 해제. */
	FTimerHandle ScanTimerHandle;
};
