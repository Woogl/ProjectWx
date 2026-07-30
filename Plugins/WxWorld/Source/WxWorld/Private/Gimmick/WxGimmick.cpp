// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxGimmick.h"

#include "Components/ArrowComponent.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"

AWxGimmick::AWxGimmick()
{
	PrimaryActorTick.bCanEverTick = false;

	// 기믹 컴포넌트가 상태 태그를 복제하므로 액터도 복제 대상이어야 한다.
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 서브오브젝트 이름은 컴포넌트 승격 전과 같게 유지한다 — BP 에 저장된 ST 에셋 지정이 이름으로 맞물려 있다.
	StateTree = CreateDefaultSubobject<UWxGimmickStateTreeComponent>(TEXT("StateTree"));

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->ArrowSize = 1.0f;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif
}
