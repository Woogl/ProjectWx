// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StateTreeAsyncExecutionContext.h"
#include "WxInteractable.h"
#include "WxDevice.generated.h"

class ACharacter;
class UWxDeviceStateTreeComponent;
struct FWxStateTreeTask_WaitForTrigger;

/**
 * StateTree 로 자기 상태를 구동하는 월드 장치(문·상자·체크포인트·엘리베이터)의 공통 호스트.
 *
 * 루트를 만들지 않는다 — 파생 BP 가 저마다 다른 몸통을 세운다.
 * 버튼·레버 같은 발동 장치도 이 클래스다 — 누른 상태를 자기 트리로 몰면서 '연결 장치 작동' 태스크로 상대를 민다.
 *
 * 상태의 실행·소유(복제 StateTag)·ST 에셋 저작은 전부 UWxDeviceStateTreeComponent 가 맡는다 — 상태 구동 패턴은 그 클래스 doc-comment 참조.
 * 이 액터에 남는 것은 상호작용 표면(IWxInteractable·프롬프트·당사자), 그리고 배치가 정하는 배선(LinkedDevices)뿐이다.
 * 상호작용 신호는 액터가 받아 트리에 전달한다 — 스캐너·어빌리티·발동 장치가 보는 계약 상대는 액터 하나다.
 *
 * 작동 신호는 하나다. 직접 눌리든 다른 장치가 밀든 그 상태에서 기다리던 '작동 대기' 태스크(FWxStateTreeTask_WaitForTrigger)를 완료시키고, 어느 상태로 갈지는 그 상태의 「성공 시」 전이가 정한다.
 * 그 태스크가 없는 상태에서는 작동도 플레이어 상호작용도 받지 않는다 — 이 장치를 미는 버튼은 그동안 잠긴다. 프롬프트와 수락 규칙은 그 태스크의 설정에서 읽는다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDevice : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxDevice();

	//~ Begin IWxInteractable
	virtual void GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const override;
	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) override;
	//~ End IWxInteractable

	/**
	 * 작동 신호의 입구. Sender 는 신호를 보낸 장치(직접 눌렸으면 nullptr), Value 는 그 장치에서 플레이어가 고른 선택지 값이다.
	 * 지금 기다리는 대기 태스크가 받아 줄 선택지면 그 태스크를 완료시키고 등록을 걷는다(같은 프레임의 다음 신호는 받지 않는다). 아니면 Verbose 로그만 남긴다.
	 */
	void NotifyDeviceInteracted(AActor* Interactor, const AWxDevice* Sender, int32 Value);
	ACharacter* GetInteractingCharacter() const;
	int32 GetSelectedOptionValue() const;

	/**
	 * 지금 Sender 가 작동 신호를 보내면 받아 줄 선택지. 이 장치를 미는 쪽이 자기 상호작용 목록을 이것으로 만든다 — 비어 있으면 미는 쪽이 잠긴다.
	 * 문구가 빈 선택지는 「그냥 받는다」는 뜻이라 미는 쪽이 자기 프롬프트로 채운다.
	 */
	void GetAcceptedOptions(const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const;

	/**
	 * '작동 대기' 태스크가 상태 진입에 등록하고 이탈에 걷는다. 노드 포인터는 그 상태가 활성인 동안만 유효하므로 이탈에서 반드시 걷는다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void BeginWaitForTrigger(const FWxStateTreeTask_WaitForTrigger& Task, const FStateTreeWeakExecutionContext& Context);
	void EndWaitForTrigger(const FWxStateTreeTask_WaitForTrigger& Task);

	/** '연결 장치 작동' 태스크의 대상. 자식 장치는 BeginPlay 에 부모 장치를 여기에 넣는다. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TObjectPtr<AWxDevice>> LinkedDevices;

	/** 다른 도메인의 ST 태스크(몽타주 1회 재생 등)가 Actor 바인딩으로 읽는다. 클라에는 컴포넌트의 상태 스냅샷이 실어 나른다. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Wx")
	TObjectPtr<ACharacter> InteractingCharacter;

	/**
	 * 마지막으로 받아들인 작동 신호의 선택지 값(엘리베이터는 정차 지점 번호). 받은 적 없으면 INDEX_NONE.
	 * ST 태스크가 Actor 바인딩으로 읽는다. 클라에는 컴포넌트의 상태 스냅샷이 실어 나른다.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Wx")
	int32 SelectedOptionValue = INDEX_NONE;

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDeviceStateTreeComponent> StateTreeComponent;

	/** 없으면 이 장치는 작동도 플레이어 상호작용도 받지 않는다. 트리가 끝나거나 멈추면 대기 노드의 이탈이 걷으므로, 있으면 트리는 돌고 있다. */
	const FWxStateTreeTask_WaitForTrigger* WaitingTask = nullptr;
	FStateTreeWeakExecutionContext WaitingContext;
};
