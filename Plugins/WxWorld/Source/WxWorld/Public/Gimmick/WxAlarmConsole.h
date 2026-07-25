// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxAlarmConsole.generated.h"

class UStaticMeshComponent;

/**
 * 1회성 경보 콘솔.
 * 상호작용 시 권위 측이 State 를 Alarmed 로 확정한다.
 * 발동 후 재상호작용 불가.
 * 상태는 자체 State 태그(Gimmick.AlarmConsole.*)가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * Niagara/사운드 재생과 인터랙션 비활성은 GimmickStateTree(ST_AlarmConsole)가 State 태그 이벤트로 진입한 상태에서 적용한다(라이브 발동에서만 FX 재생, 복원 시 StateTree.Restore 마커로 침묵).
 */
UCLASS(Abstract)
class WXWORLD_API AWxAlarmConsole : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxAlarmConsole();

	//~ Begin IWxInteractable — 상호작용 시 State 를 Alarmed 로 확정(프롬프트는 베이스 InteractionPrompt).
	virtual void OnInteracted(AActor* Interactor, const UActorComponent* Source) override;
	//~ End IWxInteractable

protected:

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Spawn Niagara 가 attach 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Console;
};
