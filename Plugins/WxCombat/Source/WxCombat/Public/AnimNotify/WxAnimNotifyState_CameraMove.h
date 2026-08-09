// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_CameraMove.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * 카메라 이동 연출 AnimNotifyState.
 * 구간 동안 로컬 플레이어의 뷰를 몽타주 재생 액터의 메시 기준 상대 위치에 스폰한 임시 카메라로 블렌드 전환하고, 끝나면 폰으로 되돌린다.
 * 플레이어 피니셔, 적 패턴처럼 그 순간을 고정 각도로 보여주는 용도.
 *
 * 순수 로컬 연출이라 이 클라이언트의 로컬 플레이어 뷰에만 적용되고, 데디서버·원격 클라에선 아무 것도 하지 않는다.
 */
// TODO: 게임 로직 이관 필요
UCLASS(meta = (DisplayName = "Wx Camera Move"))
class WXCOMBAT_API UWxAnimNotifyState_CameraMove : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	// 정지 상태에서도 오프셋·회전 편집을 프리뷰에 즉시 반영하기 위해 프로퍼티 변경을 가로챈다.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/** 메시(캐릭터 몸체) 로컬 기준 카메라 위치 오프셋. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera")
	FVector CameraRelativeLocation = FVector(250.f, 50.f, 180.f);

	/** 메시(캐릭터 몸체) 로컬 기준 카메라 회전. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera")
	FRotator CameraRelativeRotation = FRotator(-20.f, 180.f, 0.f);

	/** true면 카메라를 오너(몽타주 재생 액터)에 부착해 따라가고, false면 구간 시작 시점의 월드 트랜스폼에 고정한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera")
	bool bAttachToOwner = true;

	/** 카메라 시야각(FOV). 낮출수록 시네마틱한 압축감을 준다. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float FieldOfView = 90.0f;

	/** 임시 카메라로 전환하는 블렌드 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera", meta = (ClampMin = "0"))
	float BlendInTime = 0.5f;

	/** 폰 카메라로 복귀하는 블렌드 시간(초). */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera", meta = (ClampMin = "0"))
	float BlendOutTime = 0.5f;

#if WITH_EDITORONLY_DATA
	/** 애니메이션 에디터 프리뷰에서 카메라가 놓일 위치·각도를 카메라 모델 메시로 표시한다. 런타임 동작에는 영향 없음. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera")
	bool bDrawEditorPreview = true;

	/** 프리뷰에 표시할 카메라 모델 메시. 기본값은 엔진 카메라 모델(MatineeCam_SM). 런타임 동작에는 영향 없음. */
	UPROPERTY(EditAnywhere, Category = "Wx|Camera")
	TSoftObjectPtr<UStaticMesh> PreviewCameraMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/EditorMeshes/MatineeCam_SM.MatineeCam_SM")));

	/** 프리뷰 카메라 모델 메시를 프리뷰 월드에 렌더하는 컴포넌트. 파괴하지 않고 재사용하며, 구간 밖·토글 오프 시 비저빌리티만 끈다(월드 교체 시 폐기). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PreviewCameraMeshComponent = nullptr;

	/** 프리뷰 카메라 배치 기준 트랜스폼. 부착 모드는 실시간 몸체(매 틱 갱신), 고정 모드는 구간 첫 진입 스냅샷(유지). */
	TOptional<FTransform> PreviewCameraTransform;
#endif
};
