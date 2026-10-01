// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotifyState_ApplyGameplayEffect.generated.h"

class UGameplayEffect;

/**
 * 무적(Effect.Invincible)·퍼펙트가드(Effect.PerfectGuard) 판정 구간이 이걸로 열린다.
 *
 * 노티파이는 구간 신호와 설정만 제공하고, 몽타주 소유 어빌리티가 효과 핸들을 보관하고 정리한다.
 * GE에 지속시간을 실어 스스로 만료시키면 애니메이션 시계와 GE 시계가 둘로 갈려, 재생 속도가 도중에 바뀌는 순간 구간이 애니메이션과 어긋난다.
 *
 * 부여·제거는 서버에서만 실행하고 소유 클라를 포함한 클라이언트는 GAS 복제를 따른다.
 * 클라이언트 예측은 별도 동기화 설계 전까지 사용하지 않는다.
 *
 * EffectClass는 지속시간이 없는 GE여야 한다 — Instant나 HasDuration을 지정하면 구간이 성립하지 않는다.
 * 끝에서는 이 구간이 건 핸들의 스택 하나만 걷으므로, 스택형이 아닌 GE도 겹친 다른 구간·소유자의 효과를 건드리지 않는다.
 * 여러 캐릭터가 에셋을 공유해도 실행 상태는 각 어빌리티에 격리된다. 몽타주에서만 쓴다.
 */
UCLASS()
class WXGAME_API UWxAnimNotifyState_ApplyGameplayEffect : public UWxAnimNotifyState_AbilityEvent
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TSubclassOf<UGameplayEffect> EffectClass;

};
