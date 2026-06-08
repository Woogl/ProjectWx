// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Net/UnrealNetwork.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowColor = FColor(255, 200, 0);
		ArrowComponent->bTreatAsASprite = true;
	}
#endif
}

void AWxGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWxGimmick, bTriggered);
}

void AWxGimmick::MarkTriggered()
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	bTriggered = true;
	ApplyState();
}

FGuid AWxGimmick::GetWxSaveId() const
{
	return WxSaveId;
}

void AWxGimmick::OnWxSaveRestored()
{
	// BeginPlay 이전 복원이면 곧 호출될 BeginPlay 가 ApplyState 를 호출하므로 생략.
	// BeginPlay 이후 복원(스트리밍 인-스트림 등) 이면 즉시 시각/인터랙션 동기화.
	if (HasActorBegunPlay())
	{
		ApplyState();
	}
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 런타임 가용 UPROPERTY 로 복사한다. ActorGuid 는 에디터에서 액터별로 안정·고유하고,
// 쿠킹 시 쿠커가 PostLoad 를 거쳐 WxSaveId 에 이 값을 구워넣으므로 레벨 재저장 없이도 런타임 키가 보장된다.
void AWxGimmick::PostActorCreated()
{
	Super::PostActorCreated();

	WxSaveId = GetActorGuid();
}

void AWxGimmick::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// 복제 시 엔진이 새 ActorGuid 를 부여하므로 그대로 따라가면 원본과 충돌하지 않는다.
	WxSaveId = GetActorGuid();
}

void AWxGimmick::PostLoad()
{
	Super::PostLoad();

	WxSaveId = GetActorGuid();
}
#endif

void AWxGimmick::OnRep_bTriggered()
{
	ApplyState();
}
