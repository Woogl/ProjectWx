# WxCombat — 코드 리뷰

> 전반적으로 건강하다. GAS 순정 확장점(태그 차단·GE 컴포넌트·예측 키) 위에 수명·권위 처리가 일관되게 얹혀 있고, 직전 리뷰의 콤보 동작 중복은 `UWxAbility_Combo`로 올라가 해소됐다. 남은 것은 중첩 슬로모션 해제 한 건이다.
> 커버리지: 어빌리티 전 타입·ASC·입력 버퍼·AttributeSet·피해 GE와 반응 컴포넌트·무기·투사체·컷신·소환물·루트 모션 modifier·노티파이의 cpp를 읽었고, 헤더는 규칙·GC 관점으로 전수 검색했다. 빌드·PIE·네트워크 실행은 하지 않았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 같은 배율의 슬로모션이 겹치면 먼저 끝난 요청이 전역 배율을 1로 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:20`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:42`
- **범주**: 버그/정확성
- **문제**: 각 태스크가 전역 배율을 직접 쓰고, 종료 시 현재 값이 자기 `AppliedDilation`과 같으면 1로 되돌린다. 이 비교는 요청의 주인을 구분하지 못한다. `UWxAnimNotifyState_SlowTime`의 기본 배율은 0.4이고 극한 회피·퍼펙트 가드가 모두 이것을 쓰므로, 멀티플레이에서 두 플레이어의 슬로모션이 시차를 두고 겹치면 먼저 끝난 쪽이 아직 진행 중인 다른 쪽의 슬로모션을 끊는다. 배율이 다른 경우에도 나중 요청이 먼저 끝나면 여전히 유효한 앞 요청을 복원하지 못한다. `WxSkillCutsceneComponent.cpp:141`·`:407`도 같은 비교 방식으로 같은 전역 값을 쓰므로 컷신 종료가 진행 중인 슬로모션을 1로 되돌리고, 컷신 도중 시작된 슬로모션은 0.001 정지를 풀어 버린다.
- **제안**: 월드 단위로 활성 배율 요청을 등록·해제하는 한 곳을 두고, 요청이 바뀔 때마다 남은 요청(컷신 정지 우선)으로 최종 배율을 다시 계산한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Combo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageReaction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_PerfectGuard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_AdditionalEffects.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, 나머지 어빌리티(Attack·Skill·Pattern·Passive·PlayMontageOnce)·태스크·GE·Cue·노티파이·타게팅 태스크 cpp, `Plugins/WxCombat/Source/WxCombat/Private/StateTreeTask/WxStateTreeTask_PlayMontageOnce.cpp`. 헤더는 첫 줄 저작권·인라인 정의(`GetInstanceDataType()` 예외 주석 확인)·Wx 접두사·friend·UObject 포인터의 UPROPERTY 여부를 전수 검색했고 위반은 없었다. 엔진 5.8의 `PlayMontageAndWait::Activate`, `ShouldBroadcastAbilityTaskDelegates`, ServerInitiated 활성화 키 생성, 몽타주 인스턴스 ID 전역 고유성을 대조했다.
- **미검토 / 한계**: 빌드·PIE·리슨 서버 재현은 하지 않았다. BP·몽타주·DataTable 저작 내용(노티파이 배치, GE 에셋 수치, 투사체 이동 복제 설정)은 범위 밖이다. `WxAnimNotifyState_CameraMove`·`WxTargetingPreview`의 에디터 프리뷰 경로와 `WxEffectComponent_UIData`는 깊이 보지 않았다. 구간 GE 노티파이의 `NotifyEnd` 유실 시 무적 잔존은 메모리에 알려진 미검증 위험으로 남아 있어 새 발견으로 올리지 않았다.

---
*문서 기준 커밋 `e09ed19d1` · 리뷰일 2026-09-30 · 소스 203파일 — `/module-review`로 갱신*
