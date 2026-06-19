// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Components/StateTreeComponent.h"
#include "Interaction/WxInteractionComponent.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	// 컴포넌트 의존 순서(인터랙션 바인딩·초기 위치 스냅 등) 보장을 위해 자동 시작을 끄고, 각 자식 BeginPlay 끝에서 StartLogic 을 호출한다.
	StateTree->SetStartLogicAutomatically(false);

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

FGuid AWxGimmick::GetWxSaveId() const
{
	return WxSaveId;
}

#if WITH_EDITOR
// 에디터 전용 GetActorGuid() 를 런타임 가용 UPROPERTY 로 복사한다. ActorGuid 는 에디터에서 액터별로 안정·고유하고,
// 부여된 WxSaveId 는 에셋 저장 시 직렬화되어 쿠커가 그대로 읽으므로 런타임 키가 보장된다.
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
#endif

void AWxGimmick::SetInteractionEnabled(bool bEnabled)
{
	TInlineComponentArray<UWxInteractionComponent*> Interactions(this);
	for (UWxInteractionComponent* Interaction : Interactions)
	{
		Interaction->SetInteractionEnabled(bEnabled);
	}
}
