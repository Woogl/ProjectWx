// Copyright Woogle. All Rights Reserved.

#include "BoxComponentVisualizer.h"

#include "ActorEditorUtils.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "SceneManagement.h"
#include "UObject/UnrealType.h"

IMPLEMENT_HIT_PROXY(HBoxComponentVisProxy, HComponentVisProxy)

HBoxComponentVisProxy::HBoxComponentVisProxy(const UActorComponent* InComponent, const FVector& InCornerSign)
	: HComponentVisProxy(InComponent, HPP_Foreground)
	, CornerSign(InCornerSign)
{
}

void FBoxComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UBoxComponent* BoxComponent = Cast<const UBoxComponent>(Component);
	if (!BoxComponent)
	{
		return;
	}

	const FTransform Transform = BoxComponent->GetComponentTransform();
	const FVector Extent = BoxComponent->GetUnscaledBoxExtent();

	for (const double SignX : { 1.0, -1.0 })
	{
		for (const double SignY : { 1.0, -1.0 })
		{
			for (const double SignZ : { 1.0, -1.0 })
			{
				const FVector CornerSign(SignX, SignY, SignZ);
				const FVector Corner = Transform.TransformPosition(CornerSign * Extent);

				// 사이를 비우지 않으면 뒤에 그리는 것까지 직전 프록시에 묶여 클릭이 엉킨다.
				PDI->SetHitProxy(new HBoxComponentVisProxy(BoxComponent, CornerSign));
				PDI->DrawPoint(Corner, HandleColor, HandleSize, SDPG_Foreground);
				PDI->SetHitProxy(nullptr);
			}
		}
	}
}

void FBoxComponentVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	if (!Canvas || Component != GetEditedComponent() || SelectedCornerSign.IsZero())
	{
		return;
	}

	// 핸들이 전부 같은 색이라 어느 것을 잡았는지는 글자로만 읽힌다.
	const TCHAR* AxisNames[] = { TEXT("X"), TEXT("Y"), TEXT("Z") };

	FString CornerText;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		CornerText += FString::Printf(TEXT("%s%s "), SelectedCornerSign[Axis] > 0.0 ? TEXT("+") : TEXT("-"), AxisNames[Axis]);
	}

	const FIntRect CanvasRect = Canvas->GetViewRect();
	const FVector2D TextPosition(
		CanvasRect.Min.X + CanvasRect.Width() * 0.5,
		CanvasRect.Max.Y - HudTextBottomMargin);

	FCanvasTextItem TextItem(TextPosition, FText::FromString(CornerText.TrimEnd()), GEngine->GetLargeFont(), FLinearColor::White);
	Canvas->DrawItem(TextItem);
}

bool FBoxComponentVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	if (!VisProxy || !VisProxy->IsA(HBoxComponentVisProxy::StaticGetType()))
	{
		return false;
	}

	const UBoxComponent* BoxComponent = Cast<const UBoxComponent>(VisProxy->Component.Get());
	if (!BoxComponent)
	{
		return false;
	}

	BoxPropertyPath = FComponentPropertyPath(BoxComponent);
	if (!BoxPropertyPath.IsValid())
	{
		BoxPropertyPath.Reset();
		return false;
	}

	const HBoxComponentVisProxy* CornerProxy = static_cast<HBoxComponentVisProxy*>(VisProxy);
	SelectedCornerSign = CornerProxy->CornerSign;

	return true;
}

bool FBoxComponentVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
	const UBoxComponent* BoxComponent = Cast<UBoxComponent>(GetEditedComponent());
	if (!BoxComponent || SelectedCornerSign.IsZero())
	{
		return false;
	}

	OutLocation = BoxComponent->GetComponentTransform().TransformPosition(SelectedCornerSign * BoxComponent->GetUnscaledBoxExtent());
	return true;
}

bool FBoxComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	UBoxComponent* BoxComponent = Cast<UBoxComponent>(GetEditedComponent());
	if (!BoxComponent || SelectedCornerSign.IsZero())
	{
		return false;
	}

	if (!DeltaTranslate.IsZero())
	{
		const FTransform Transform = BoxComponent->GetComponentTransform();
		const FVector LocalDelta = Transform.InverseTransformVector(DeltaTranslate);

		const FVector OldExtent = BoxComponent->GetUnscaledBoxExtent();
		FVector NewExtent = OldExtent;
		FVector LocalShift = FVector::ZeroVector;

		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			NewExtent[Axis] = FMath::Max(OldExtent[Axis] + LocalDelta[Axis] * SelectedCornerSign[Axis] * 0.5, 0.0);

			// 이동량을 익스텐트 변화에서 되뽑아야 0 에서 잘린 뒤로도 고정된 면이 끌려가지 않는다.
			LocalShift[Axis] = SelectedCornerSign[Axis] * (NewExtent[Axis] - OldExtent[Axis]);
		}

		BoxComponent->Modify();
		BoxComponent->SetBoxExtent(NewExtent, false);
		BoxComponent->SetWorldLocation(BoxComponent->GetComponentLocation() + Transform.TransformVector(LocalShift));

		// 이 통지가 BP 프리뷰 액터의 변경을 아키타입까지 밀어 준다. 직접 PostEditChangeProperty 만 부르면 BP 에디터에서 끌어 놓은 값이 컴파일과 함께 되돌아간다.
		TArray<FProperty*> ModifiedProperties;

		// BoxExtent 는 protected 라 GET_MEMBER_NAME_CHECKED 로 잡히지 않는다.
		ModifiedProperties.Add(FindFProperty<FProperty>(UBoxComponent::StaticClass(), TEXT("BoxExtent")));
		ModifiedProperties.Add(FindFProperty<FProperty>(USceneComponent::StaticClass(), USceneComponent::GetRelativeLocationPropertyName()));
		NotifyPropertiesModified(BoxComponent, ModifiedProperties, EPropertyChangeType::ValueSet);
	}

	GEditor->RedrawLevelEditingViewports(true);
	return true;
}

void FBoxComponentVisualizer::EndEditing()
{
	BoxPropertyPath.Reset();
	SelectedCornerSign = FVector::ZeroVector;
}

UActorComponent* FBoxComponentVisualizer::GetEditedComponent() const
{
	return BoxPropertyPath.GetComponent();
}

bool FBoxComponentVisualizer::IsVisualizingArchetype() const
{
	const AActor* Owner = BoxPropertyPath.GetParentOwningActor();
	return Owner && FActorEditorUtils::IsAPreviewOrInactiveActor(Owner);
}
