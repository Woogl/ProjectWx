// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "GameplayTagContainer.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeDelegate.h"
#include "StateTreeExecutionExtension.h"
#include "WxInteractable.h"
#include "WxSavable.h"
#include "WxGimmickStateTreeComponent.generated.h"

class ACharacter;
class AWxLeverDevice;
class UPrimitiveComponent;
class UWxGimmickStateTreeComponent;

/**
 * 잠든 트리가 깨어나기를 요청할 때 컴포넌트 틱을 다시 켜는 실행 컨텍스트 확장.
 * 엔진 컴포넌트도 같은 역할의 확장을 붙이지만 그 구현체가 모듈 밖으로 export 되지 않아, 저장된 상태에서 트리를 여는 경로에서는 이것을 대신 붙인다.
 */
USTRUCT()
struct FWxGimmickStateTreeExecutionExtension : public FStateTreeExecutionExtension
{
	GENERATED_BODY()

	virtual void ScheduleNextTick(const FContextParameters& Context, const FNextTickArguments& Args) override;

	UPROPERTY()
	TObjectPtr<UWxGimmickStateTreeComponent> Component;
};

/**
 * 상호작용이 켜져 있는 영역 하나 — 그 영역의 HUD 프롬프트와, 눌렸을 때 트리에 알릴 자리.
 * 발행자는 'Enable Interaction' 태스크가 자기 대상 메시 몫으로 하나씩 소유한다. 어느 영역이 눌렸는지를 발행자 자체가 가르므로 전이에 페이로드 비교 조건이 필요 없다.
 */
USTRUCT()
struct FWxGimmickInteractionRegion
{
	GENERATED_BODY()

	UPROPERTY()
	FText Prompt;

	/** 이 영역이 눌렸을 때 발행할 ST 델리게이트. 어느 상태로 갈지는 이것을 듣는 전이가 정한다. */
	UPROPERTY()
	FStateTreeDelegateDispatcher Dispatcher;

	/** 위 발행에 쓰는 실행 컨텍스트. 발행이 트리 틱 밖에서 일어나므로 태스크가 진입 시 자기 것을 넘겨 둔다. */
	FStateTreeWeakExecutionContext Context;
};

/**
 * 이 기믹에 연결된 레버 장치 하나. 공유 ST 에셋은 역할 이름만 알고, 그 역할이 어느 장치인지는 이 배선이 정한다.
 */
USTRUCT()
struct FWxGimmickDeviceLink
{
	GENERATED_BODY()

	/** ST 태스크가 장치를 지목하는 키. 수신자 관점의 명령형 동사로 짓고(예: 문 Open, 엘리베이터 CallToA/CallToB), BP 아키타입이 행으로 미리 깔아 어휘를 중앙화한다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FName Role = TEXT("Activate");

	/** 레벨 액터 참조라 인스턴스 전용이다. 비워 두면 그 역할의 기능을 쓰지 않는 것으로 본다. */
	UPROPERTY(EditInstanceOnly, Category = "Wx")
	TObjectPtr<AWxLeverDevice> Device;
};

/**
 * 역할 하나의 발행 자리 — 그 역할의 장치가 눌렸을 때 트리에 알릴 곳. 수명 규약은 FWxGimmickInteractionRegion 과 같다.
 */
USTRUCT()
struct FWxGimmickDeviceBinding
{
	GENERATED_BODY()

	UPROPERTY()
	FStateTreeDelegateDispatcher Dispatcher;

	FStateTreeWeakExecutionContext Context;
};

/**
 * 기믹의 상태머신·상호작용·영속을 한 몸에 담는 StateTree 컴포넌트.
 * 이 컴포넌트를 붙이면 어떤 액터든(순수 BP 포함) 기믹이 된다 — 전용 C++ 액터 클래스가 필요 없다.
 *
 * 세 가지 책임이 있다.
 *  - 상호작용 계약(IWxInteractable): 어느 메시가 지금 켜진 영역인지, 그 영역의 프롬프트가 무엇인지 답하고, 눌린 영역의 ST 델리게이트를 발행한다.
 *  - 상태 영속(IWxSavable): 지금 활성인 ST 상태의 Tag 를 저장·복제하고, 복원 시 그 상태에서 트리를 연다.
 *  - StateTree 구동: 자동 시작하며, 저장된 상태가 있으면 그 상태를 시작점으로 지정해 시작한다.
 *
 * 상태 구동 패턴 — 상태는 서버 권위다. 권위 트리만 상태를 정하고 클라 트리는 복제된 상태를 추종하기만 한다.
 *  - 전이는 전부 ST 에셋이 정한다. 권위 측이 상호작용을 받으면 눌린 영역의 발행자를 자기 트리에 발행하고, 어느 상태로 갈지는 그 발행자를 지목한 전이(On Delegate)가 정한다.
 *  - 상태 식별은 엔진 순정 상태 Tag 다. 상태 디테일의 Tag 필드에 태그를 달면 그 값이 곧 저장 키가 된다(에셋 안에서 유일해야 한다).
 *  - 권위 측은 틱마다 활성 상태의 Tag 를 StateTag 에 기록한다. 상태 변화는 전부 트리 틱 안에서 일어나므로 이 폴링이 전부를 잡는다.
 *  - 클라는 틱 말미에 자기 활성 태그를 StateTag 와 대조해, 어긋나 있으면 그 상태로 전이를 요청한다(라이브 전이라 트리거형 태스크가 정상 발동한다).
 *    지연·발행 유실·전이 조건 불일치를 가리지 않고 같은 자리에서 수렴하며, 클라가 로컬 완료로 먼저 넘어가더라도 권위 값이 언제나 이긴다.
 *  - 복원·재시작 진입은 SourceStateID 가 무효인 초기 진입이라, 노드들이 별도 마커 없이 스냅·스킵으로 처리한다. 트리 재시작은 세이브 복원·레이트조인·스트리밍 인 전용이다.
 *  - 위 경로는 오너 액터의 Replicates 가 켜져 있어야 성립한다. 컴포넌트는 자기 몫만 켤 수 있어 오너는 배치 측 책임이며, 꺼져 있으면 BeginPlay 가 Error 로그를 남긴다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxGimmickStateTreeComponent : public UStateTreeComponent, public IWxSavable, public IWxInteractable
{
	GENERATED_BODY()

public:
	UWxGimmickStateTreeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin UActorComponent
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent

#if WITH_EDITOR
	/** 저장 직전에 SaveId 를 오너의 ActorGuid 로 확정한다(런타임은 심긴 값을 읽기만 한다). */
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#endif

	//~ Begin UBrainComponent — 저장된 상태가 있으면 그 상태를 시작점으로 트리를 연다.
	virtual void StartLogic() override;
	virtual void RestartLogic() override;

	/** 트리가 멈추기 전에 마지막 활성 상태를 기록한다 — 정지 후엔 활성 상태가 비어 읽을 수 없다. */
	virtual void StopLogic(const FString& Reason) override;
	//~ End UBrainComponent

	//~ Begin IWxInteractable
	virtual bool IsInteractionMeshActive(const UPrimitiveComponent* Mesh) const override;

	/** 권위 측에서 상호작용을 받아 눌린 영역의 발행자를 자기 트리에 발행한다. 어느 상태로 갈지는 ST 에셋의 전이가 정하고, 그 결과가 복제되어 클라에 전해진다. */
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;

	virtual FText GetInteractionPrompt(const UActorComponent* Source) const override;
	//~ End IWxInteractable

	//~ Begin IWxSavable
	/**
	 * 에디터에서 부여되어 에셋에 직렬화된 슬롯 키(SaveId). 런타임·세션 간 불변이며 쿠킹 빌드에서도 그대로 성립한다.
	 * 오너 경로에서 파생하지 않는다 — World Partition 은 런타임 경로에 스트리밍 셀(그리드 좌표)을 포함하므로, 액터를 옮기거나 PIE 와 패키지 빌드를 오갈 때마다 키가 달라진다.
	 * 대신 배치 후 맵을 한 번 저장해야 키가 에셋에 남는다. 저장 전이거나 런타임 스폰된 기믹은 키가 무효라 저장/복원에서 제외된다.
	 */
	virtual FGuid GetSaveId() const override;
	virtual void OnSaveRestored() override;
	//~ End IWxSavable

	/**
	 * Mesh 영역의 상호작용을 켜고 끄며, 켤 때는 그 영역의 프롬프트와 발행 자리(Region)도 함께 담는다 — 끌 때 Region 은 쓰이지 않는다.
	 * 'Enable Interaction' 태스크가 상태 진입 시 자기 대상 메시로 호출한다. 계약의 대상 단위 토글(IWxInteractable::SetInteractionEnabled)과 달리 영역 단위라 이름을 가른다.
	 * 꺼진 영역은 IsInteractionMeshActive 가 false 를 답해 다음 스캔에서 후보에서 빠지고, 어빌리티의 서버 활성 검증에도 걸린다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetInteractionRegionEnabled(UPrimitiveComponent* Mesh, bool bEnabled, const FWxGimmickInteractionRegion& Region);

	/**
	 * Role 역할의 발행 자리를 여닫고, 그 역할로 링크된 레버 장치들을 같은 상태로 켜고 끈다 — 끌 때 Binding 은 쓰이지 않는다.
	 * '장치 상호작용 켜기' 태스크가 상태 진입 시 호출한다. 영역 토글과 같은 규약이며, 대상이 자기 메시가 아니라 링크된 장치라는 점만 다르다.
	 * 복제하지 않는다 — ST 가 각 피어에서 실행되어 같은 값에 수렴한다.
	 */
	void SetDeviceInteractionEnabled(FName Role, bool bEnabled, const FText& Prompt, const FWxGimmickDeviceBinding& Binding);

	/** 링크된 레버 장치가 눌렸을 때 장치가 서버 권위에서 부른다. 장치의 역할을 역조회해 열린 발행 자리에 알리며, 이후는 영역 상호작용과 같은 경로다. */
	void NotifyDeviceInteracted(AWxLeverDevice* Device, AActor* Interactor);

	ACharacter* GetInteractingCharacter() const;

	/** 지금 활성인 상태의 Tag. 활성 leaf 에서 위로 올라가며 처음 만나는 유효 태그를 답한다(태그 없는 중간 상태는 건너뛴다). */
	FGameplayTag GetActiveStateTag();

	/** 실행 컨텍스트 확장이 트리의 깨우기 요청을 전달하는 진입점. */
	void NotifyTickRequested();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 도착 시점엔 판정하지 않고 잠들어 있던 컴포넌트 틱만 깨운다. 추종 판정은 트리 틱 뒤(FollowAuthorityState)가 맡는다.
	 * 도착 즉시 비교하면 아직 대기 중인 발행을 소화하지 못한 트리를 「어긋났다」고 오판하게 된다.
	 */
	UFUNCTION()
	void OnRep_StateTag();

	/**
	 * 지금 활성인 상태의 Tag(복제 + SaveGame). 권위 측이 틱마다 갱신하며, 이 값이 곧 세이브 슬롯에 담기는 기믹의 상태다.
	 * 상태에 Tag 를 달지 않으면 저장되지 않으므로, 영속이 필요한 상태에는 반드시 Tag 를 지정한다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_StateTag, SaveGame, VisibleAnywhere, Category = "Wx")
	FGameplayTag StateTag;

	/**
	 * 지금 상호작용이 켜져 있는 영역들. 멤버십 자체가 활성 상태라 따로 담는 bool 이 없다.
	 * 'Enable Interaction' 태스크가 상태 진입 시 넣고 뺀다. 로컬 전용(복제·SaveGame 아님) — 각 피어의 ST 가 같은 값으로 수렴시킨다.
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FWxGimmickInteractionRegion> InteractionRegions;

	/**
	 * 이 기믹에 연결된 레버 장치 배선. 역할 행은 BP 아키타입이 미리 깔고(ST 에셋과 어휘 공유), 어느 장치인지는 레벨 인스턴스가 채운다.
	 * 하드 참조라 장치와 기믹은 World Partition 에서 함께 로드된다 — 늦은 등록을 따로 다루지 않는 근거다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TArray<FWxGimmickDeviceLink> DeviceLinks;

	/** 지금 발행 자리가 열린 역할들. 멤버십 자체가 활성 상태라 따로 담는 bool 이 없다 — InteractionRegions 와 같은 규약, 키만 역할. */
	UPROPERTY(Transient)
	TMap<FName, FWxGimmickDeviceBinding> DeviceBindings;

	/**
	 * 이번 상호작용의 당사자(플레이어 캐릭터). 권위가 쓰고 복제로 각 피어에 전해진다 — StateTag 와 같은 번치에 실리므로 추종 전이 시점에 언제나 짝이 맞는다.
	 * 비영속이라 복원 시엔 비어 있고, 그때는 이동/몽타주 태스크가 초기 진입으로 보아 건너뛴다.
	 */
	UPROPERTY(Replicated, Transient)
	TObjectPtr<ACharacter> InteractingCharacter;

private:
	/**
	 * 권위가 상태를 바꾸지 않은 채 같은 상태로 다시 진입했음을 알린다(자기 자신으로 가는 전이 — 예: 이미 켜진 체크포인트에서 다시 쉬기).
	 * 이것만은 StateTag 가 그대로라 값 복제로 관측되지 않아 이 경로가 필요하다. 상태를 정하지 않고 「권위가 정한 그 상태를 다시 열라」만 나르므로 클라가 목적지를 판단하는 일은 없다.
	 * 복제 카운터를 쓰지 않는 이유: 늦게 관련성을 얻은 피어에게는 초기값이 통째로 「변화」로 보여, 마침 같은 상태에서 시작했으면 재진입 연출이 헛발동한다.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ReenterState(FGameplayTag ReenteredTag);

	/** 트리 재시작 직전에 영역·장치의 켜짐 잔재를 걷는다 — 직후 시작이 전 상태를 초기 진입으로 다시 밟으며 필요한 것만 재등록한다. */
	void ResetInteractions();

	/**
	 * 저장된 StateTag 가 가리키는 상태에서 트리를 시작한다. 태그가 없거나 에셋에서 찾지 못하면 순정 시작(루트 선택)에 맡긴다.
	 * 엔진의 StartTree 가 시작 파라미터를 하드코딩하고 protected·비가상이라, 시작 상태를 넣는 경로만 여기서 다시 조립한다(엔진 업그레이드 시 확인 지점).
	 */
	void StartTreeAtSavedState();

	/** 지금 StateTag 가 로컬 에셋의 어느 상태인지 답한다. 태그가 비었거나 에셋에 그 상태가 없으면 무효 핸들. */
	FStateTreeStateHandle ResolveStateTag() const;

	/** 권위 측에서 활성 상태의 Tag 를 StateTag 에 반영하고, 상태가 그대로인 재진입이면 대신 전 피어에 알린다. 트리가 상태를 바꾸는 지점(시작·틱) 뒤에서 호출한다. */
	void PublishAuthorityState();

	/** 비권위 측에서 복제된 StateTag 를 추종한다. 어긋나 있으면 그 상태로 전이를 요청한다. 트리 틱 뒤에서 호출해야 대기 중인 발행을 소화한 결과를 본다. */
	void FollowAuthorityState();

	/** StateTag 가 가리키는 상태로 라이브 전이를 요청한다. 재시작과 달리 인스턴스 데이터·대기 중인 발행을 보존하고 진입이 초기 진입으로 취급되지 않는다. */
	void EnterReplicatedState();

	UPROPERTY()
	FGuid SaveId;

	/** 권위 전용. 상호작용을 받았지만 트리가 아직 그것을 소화하지 않은 상태. PublishAuthorityState 가 소비해 「상태가 안 바뀐 재진입」을 가려낸다. */
	bool bPendingInteractResolve = false;
};
