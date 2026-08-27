// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "UObject/PropertyText.h"
#endif
#include "WxDeviceStateTreeComponent.generated.h"

/**
 * 장치의 상태머신 실행기이자 상태(StateTag)의 소유자. 트리 실행·틱 스케줄링·잠들기/깨우기·ST 에셋 저작(State Tree 프로퍼티) 전부 순정 UStateTreeComponent 경로를 그대로 쓴다.
 *
 * 상태 구동 패턴 — 상태는 서버 권위다. 권위 트리만 상태를 정하고 클라 트리는 복제된 상태를 추종하기만 한다.
 *  - 상태 식별은 엔진 순정 상태 Tag 다. 상태 디테일의 Tag 필드에 태그를 달면 그 값이 곧 저장 키가 된다(에셋 안에서 유일해야 한다).
 *  - 권위 측은 트리 틱 직후 활성 상태의 Tag 를 StateTag 에 기록한다. 상태 변화는 전부 트리 틱 안에서 일어나므로 이 폴링이 전부를 잡는다.
 *  - 클라는 같은 자리에서 자기 활성 태그를 StateTag 와 대조해, 어긋나 있으면 그 상태로 전이를 요청한다(라이브 전이라 트리거형 태스크가 정상 발동한다).
 *    지연·발행 유실·전이 조건 불일치를 가리지 않고 같은 자리에서 수렴하며, 클라가 로컬 완료로 먼저 넘어가더라도 권위 값이 언제나 이긴다.
 *  - 복원도 같은 수렴이다. 세이브·레이트조인·스트리밍 인은 StateTag 만 세팅하고, 트리는 루트에서 시작해 라이브 전이로 그 상태에 닿는다(그 상태로 가는 연출이 재생된다).
 *    권위는 수렴할 때까지 발행을 보류해 복원 값이 기본 상태 발행에 덮이지 않게 한다. 초기 진입(SourceStateID 무효)은 BeginPlay 의 루트 진입뿐이다.
 *  - 저장이 없을 때의 시작 상태(InitialState)도 같은 수렴이다. 권위가 BeginPlay 에서 그 태그를 StateTag 로 삼으면 나머지는 복원과 똑같이 추종이 데려간다.
 */
UCLASS()
class UWxDeviceStateTreeComponent : public UStateTreeComponent
{
	GENERATED_BODY()

public:
	UWxDeviceStateTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 권위가 정한 상태의 Tag.
	 * 트리를 훑는 것이 아니라 복제된 필드를 그대로 답하므로 매 틱 게이트로 써도 되고, 모든 피어가 같은 권위 값을 본다.
	 */
	FGameplayTag GetStateTag() const;

	/**
	 * 권위 상호작용의 발행이 트리에 닿아, 트리가 소화할 것이 대기 중임을 알린다.
	 * 상호작용이 상태를 바꾸는지는 트리가 발행을 소화해 봐야 알므로, PublishAuthorityState 가 이 예약을 소비해 「상태가 안 바뀐 재진입」을 가려낸다.
	 * 닿지 않은 발행에 걸면 소화할 것이 없어 누를 때마다 헛재진입이 되므로, 호출자가 발행 결과로 가르고 나서 부른다.
	 */
	void NotifyInteractionPending();

	/** SaveGame 복원 직후 호스트 액터가 부른다. 복원된 StateTag 를 진실로 삼는 수렴 추종을 열고 잠든 틱을 깨운다. */
	void NotifySaveRestored();

	/** 트리를 한 번 틱한 직후 상태를 동기화한다. */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 정지 후엔 활성 상태가 비어 읽을 수 없으므로, 멈추기 전에 마지막 상태를 한 번 알린다. */
	virtual void StopLogic(const FString& Reason) override;

	/**
	 * 저장된 상태가 없으면 트리를 열기 전에 InitialState 를 권위 StateTag 로 삼아 복원과 같은 추종에 태운다.
	 * 순정 자동 시작(Super)이 트리를 연 직후 상태를 한 번 동기화한다(추종이 열려 있으면 여기서 수렴 요청이 나간다).
	 */
	virtual void BeginPlay() override;
	
protected:
	/**
	 * 도착 시점엔 판정하지 않고 잠들어 있던 틱만 깨운다. 추종 판정은 트리 틱 뒤(SyncStateWithTree)가 맡는다.
	 * 도착 즉시 비교하면 아직 대기 중인 발행을 소화하지 못한 트리를 「어긋났다」고 오판하게 된다.
	 */
	UFUNCTION()
	void OnRep_StateTag();

	/**
	 * 지금 활성인 상태의 Tag. 권위 측이 트리 틱마다 갱신하며, 이 값이 곧 세이브 슬롯에 담기는 장치의 상태다.
	 * 상태에 Tag 를 달지 않으면 저장되지 않으므로, 영속이 필요한 상태에는 반드시 Tag 를 지정한다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_StateTag, SaveGame)
	FGameplayTag StateTag;

	/**
	 * 저장된 상태가 없을 때 시작할 상태의 Tag. 기본값 Root 는 예약어로, 에셋의 루트 기본 상태에서 시작한다는 뜻이다.
	 * 저장이 있으면 저장이 이긴다 — 첫 플레이의 StateTag 를 정할 뿐이고, 그 뒤는 복원과 같은 수렴 경로를 탄다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (GetOptions = "GetInitialStateOptions"))
	FName InitialState;

private:
	/**
	 * 권위가 상태를 바꾸지 않은 채 같은 상태로 다시 진입했음을 알린다(자기 자신으로 가는 전이 — 예: 이미 켜진 체크포인트에서 다시 쉬기).
	 * 이것만은 StateTag 가 그대로라 값 복제로 관측되지 않아 이 경로가 필요하다. 상태를 정하지 않고 「권위가 정한 그 상태를 다시 열라」만 나르므로 클라가 목적지를 판단하는 일은 없다.
	 * 복제 카운터를 쓰지 않는 이유: 늦게 관련성을 얻은 피어에게는 초기값이 통째로 「변화」로 보여, 마침 같은 상태에서 시작했으면 재진입 연출이 헛발동한다.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ReenterState(FGameplayTag ReenteredTag);

	/**
	 * 트리를 틱하거나 멈춘 직후의 상태 동기화. 트리가 진실이면 발행하고(권위 평시), StateTag 가 진실이면 추종한다(클라 상시 + 권위 복원 수렴 중).
	 * 트리 틱 밖에서 보면 대기 중인 발행을 소화하지 않은 트리를 「어긋났다」고 오판하므로, 틱 말미라는 호출 시점이 판정의 전제다.
	 */
	void SyncStateWithTree();

	/** 권위 측에서 활성 상태의 Tag 를 StateTag 에 반영하고, 상태가 그대로인 재진입이면 대신 전 피어에 알린다. */
	void PublishAuthorityState();

	/** StateTag 를 추종한다. 어긋나 있으면 그 상태로 전이를 요청하고, 복원 수렴이 끝나면 발행으로 복귀한다. */
	void FollowStateTag();

	/** 지금 활성인 상태의 Tag. 활성 leaf 에서 위로 올라가며 처음 만나는 유효 태그를 답한다(태그 없는 중간 상태는 건너뛴다). */
	FGameplayTag GetActiveStateTag();

	/** 지정 상태로 라이브 전이를 요청한다. 권위 값 수렴 용도라 에셋이 저작한 어떤 전이보다 우선하며(Critical), 잠든 트리는 요청이 깨운다. */
	void RequestState(FGameplayTag InStateTag);

	/** 태그가 로컬 에셋의 상태를 가리키는가. 상태 Tag 는 에셋 안에서 유일하다는 계약이라 정확 일치로만 찾는다. */
	bool HasState(FGameplayTag InStateTag) const;

#if WITH_EDITOR
	/**
	 * InitialState 드롭다운 항목. 예약어 Root 를 앞세우고, 에셋 상태 중 Tag 가 붙은 것만 잇는다 — 태그 없는 상태는 추종할 수 없어 시작 상태로도 쓸 수 없다.
	 * 값은 상태 Tag(저장·추종 키)이고, 화면에는 에디터에서 보는 상태 이름을 띄운다.
	 */
	UFUNCTION()
	TArray<FPropertyTextFName> GetInitialStateOptions() const;
#endif

	/**
	 * 권위 전용. 복원·초기 상태로 미리 정해진 StateTag 가 진실인 구간 — 수렴할 때까지 발행을 보류해 그 값이 기본 상태 발행에 덮이지 않게 한다.
	 * 활성 태그가 StateTag 와 일치하는 순간 내려가고, 그때부터 다시 트리가 진실이 되어 발행으로 복귀한다.
	 */
	bool bFollowRestoredState = false;

	/**
	 * 권위 전용. 상호작용 발행이 트리에 닿았지만 트리가 아직 그것을 소화하지 않은 구간.
	 * PublishAuthorityState 가 소비해 「상태가 안 바뀐 재진입」을 가려낸다.
	 */
	bool bPendingInteractResolve = false;
};
