// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeDelegate.h"
#include "StructUtils/StructView.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxDevice.generated.h"

class ACharacter;
class UWxDeviceStateTreeComponent;

/**
 * 상호작용이 켜져 있는 동안의 HUD 프롬프트와, 눌렸을 때 트리에 알릴 자리.
 * '상호작용 켜기' 태스크가 상태 진입 시 자기 몫을 하나 넘긴다 — 상태마다 발행자가 다르므로 전이에 페이로드 비교 조건이 필요 없다.
 */
USTRUCT()
struct FWxDeviceInteractionBinding
{
	GENERATED_BODY()

	UPROPERTY()
	FText Prompt;

	/** 눌렸을 때 발행할 ST 델리게이트. 어느 상태로 갈지는 이것을 듣는 전이가 정한다. */
	UPROPERTY()
	FStateTreeDelegateDispatcher Dispatcher;

	/** 위 발행에 쓰는 실행 컨텍스트. 발행이 트리 틱 밖에서 일어나므로 태스크가 진입 시 자기 것을 넘겨 둔다. */
	FStateTreeWeakExecutionContext Context;
};

/**
 * StateTree 로 자기 상태를 구동하는 월드 장치(문·상자·체크포인트·엘리베이터)의 공통 호스트.
 *
 * 루트를 만들지 않는다 — 파생 BP 가 저마다 다른 몸통을 세운다.
 * 버튼·레버 같은 발동 장치도 이 클래스다 — 누른 상태를 자기 트리로 몰면서 '이벤트 보내기' 태스크로 상대를 민다.
 *
 * 상태의 실행·소유(복제·SaveGame StateTag)·ST 에셋 저작은 전부 UWxDeviceStateTreeComponent 가 맡는다 — 상태 구동 패턴은 그 클래스 doc-comment 참조.
 * 이 액터에 남는 것은 상호작용 표면(IWxInteractable·프롬프트·당사자)과 세이브 신원(IWxSavable·SaveId), 그리고 배치가 정하는 배선(LinkedDevices)뿐이다.
 * 상호작용·복원 신호는 전부 액터가 받아 컴포넌트에 전달한다 — 스캐너·어빌리티·발동 장치·세이브가 보는 계약 상대는 액터 하나다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxDevice : public AActor, public IWxInteractable, public IWxSavable
{
	GENERATED_BODY()

public:
	AWxDevice();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#endif

	//~ Begin IWxInteractable
	virtual bool CanInteract(const AActor* Interactor) const override;
	virtual void SetInteractionEnabled(bool bEnabled) override;
	virtual void OnInteracted(AActor* Interactor) override;
	virtual FText GetInteractionPrompt() const override;
	//~ End IWxInteractable

	//~ Begin IWxSavable
	virtual FGuid GetSaveId() const override;

	/** 복원은 컴포넌트의 StateTag 세팅이 전부다 — 컴포넌트에 알리면 트리가 라이브 전이로 그 상태에 수렴한다. */
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

	void NotifyDeviceInteracted(AActor* Interactor, FGameplayTag EventTag, FConstStructView Payload = FConstStructView());
	ACharacter* GetInteractingCharacter() const;

	/**
	 * 상호작용을 켜고 끄며, 켤 때는 그 상태의 프롬프트와 발행 자리(Binding)도 함께 담는다 — 끌 때 Binding 은 쓰이지 않고 담겨 있던 것도 지우지 않는다.
	 * '상호작용 켜기' 태스크가 상태 진입 시 호출한다. 꺼져 있으면 CanInteract 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionBinding(bool bEnabled, const FWxDeviceInteractionBinding& Binding);

	/** 'SendEvent' 태스크의 대상 */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TArray<TObjectPtr<AWxDevice>> LinkedDevices;

	UPROPERTY(Replicated, Transient)
	TObjectPtr<ACharacter> InteractingCharacter;

protected:
	virtual void BeginPlay() override;

private:
	void BroadcastInteractionDelegate();

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxDeviceStateTreeComponent> StateTreeComponent;

	UPROPERTY()
	FGuid SaveId;

	bool bInteractionEnabled = false;
	FWxDeviceInteractionBinding InteractionBinding;
};
