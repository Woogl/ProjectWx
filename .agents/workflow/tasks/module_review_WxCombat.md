# WxCombat — 코드 리뷰

상태: 확인 대기 · 코드 리뷰 결과 판단 대기
다음 행동: 몽타주 실패·슬로모션 중첩과 나머지 미해결 지적의 수정 여부를 판단한다.

> 현재 작업 트리의 콤보 몽타주 배열·방향 섹션·공용 차단과 기존 지적을 재검토했다. 몽타주 재생 실패 뒤의 후속 처리와 중첩 슬로모션의 종료 처리에 결함이 있으며, 기존 피해 쿼리 비용·콤보 중복도 남아 있다.
> 커버리지: AbilityBase·ASC·Attack·Skill·Pattern·HitReact·무기·범위 피해·슬로모션의 핵심 cpp와 관련 헤더를 검토했다. 빌드·PIE·네트워크 실행은 하지 않았다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 몽타주 재생이 동기로 실패해도 성공을 반환한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:413`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:85`
- **범주**: 버그/정확성
- **문제**: `PlayMontageInternal`은 `ReadyForActivation()` 뒤 무조건 `true`를 반환한다. UE 5.8의 `UAbilityTask_PlayMontageAndWait::Activate`는 AnimInstance가 없거나 `ASC->PlayMontage`가 실패하면 같은 호출 안에서 `OnCancelled`를 발행한다. 이는 베이스의 `HandleMontageCancelled`를 통해 어빌리티를 종료하지만, 호출자는 성공으로 판단한다. 방향 데이터 대기를 거치지 않고 즉시 재생하는 경로에서 KnockUp 피격은 몽타주·어빌리티가 끝난 상태에서도 `WxAbility_HitReact.cpp:95`의 `LaunchCharacter`를 실행하고, 호출자는 실패를 성공으로 받아 후속 처리를 계속할 수 있다. 사망처럼 취소 콜백을 재정의하는 타입까지 있어 단순한 `IsActive()` 확인만으로도 모든 재생 실패를 구분하지 못한다.
- **제안**: 태스크 활성화 직후 실제 새 몽타주 재생 성립 여부와 어빌리티 수명을 확인해 반환값에 반영한다. 실패 콜백으로 이미 종료된 경우 후속 회전·띄우기·태스크 등록이 진행되지 않게 한다.
- **확신도**: 높음

### 2. 🟡 같은 배율의 슬로모션이 겹치면 먼저 끝난 태스크가 나머지도 해제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:18`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:40`
- **범주**: 버그/정확성
- **문제**: 각 태스크가 전역 배율을 직접 쓰고, 종료 시 현재 값이 자기 `AppliedDilation`과 같으면 1로 돌린다. 이 비교는 요청의 소유자를 구분하지 못한다. 두 플레이어가 조금 다른 시점에 기본 배율 0.4의 극한 회피·퍼펙트 가드를 발동하면 두 번째 태스크가 실행 중이어도 첫 번째 태스크의 종료가 월드 배율을 1로 바꾼다. `UWxAnimNotifyState_SlowTime::NotifyBegin`은 매 구간마다 별도 태스크를 만들며, 활성 요청을 합치는 계층이 없다. 서로 다른 배율도 나중 요청이 종료될 때 여전히 유효한 앞 요청을 복원하지 못한다.
- **제안**: 월드 단위로 활성 슬로모션 요청을 식별하고 수명을 관리한다. 요청이 추가·제거될 때 남은 요청으로 최종 배율을 계산하며, 컷신과의 우선순위도 같은 규칙으로 정한다.
- **확신도**: 높음

### 3. 🟡 피해 판정 쿼리가 권위 없는 머신에서도 실행된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:48`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:178`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp:32`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:49`
- **범주**: 성능/안전
- **문제**: WeaponAttack 구간과 AreaDamage 노티파이에는 권위 게이트가 없다. 클라이언트에서 재생되는 몽타주도 무기의 판정 형상을 켜고 형상마다 매 틱 `SweepMultiByChannel`을 실행하며, 범위 피해는 TargetingSystem 요청을 실행한다. 결과 소비는 `ApplyDamage`뿐인데 이 함수는 권위 없는 출처를 곧바로 거절한다. 무기에는 별도의 로컬 피격 연출 소비도 없어, 화면에 보이는 다수 캐릭터의 공격마다 불필요한 충돌·타겟팅 비용이 생긴다.
- **제안**: 무기 공격 시작과 범위 피해 노티파이에서 권위를 확인해 판정 쿼리 자체를 생략한다. 충돌 결과로 로컬 ImpactFX를 내는 투사체 경로와는 구분한다.
- **확신도**: 높음

### 4. 🟡 콤보 발동과 초기화 로직이 여러 어빌리티에 중복되어 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:16`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:20`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: ComboMontages·ComboIndex·GetMontage는 UWxAbility_Combo로 모였지만 Attack·Skill의 `ActivateAbility`, `EndAbility`, `HandleMontageCompleted`, `OnComboWindowClosed`가 동일하다. Pattern도 커밋·단계 전진·몽타주 재생과 취소 시 초기화를 복제한다. 공통 데이터만 기반으로 이동했고 동작은 여전히 복제되어, 재발동·취소·창 종료 규칙을 변경할 때 복수 타입을 함께 수정해야 한다.
- **제안**: 단계 선택과 초기화 같은 공통 콤보 상태 처리를 한곳으로 모으고, Pattern의 블렌드아웃 자동 연결은 타입별 동작으로 남긴다.
- **확신도**: 높음

## 판단

- 사용자(2026-09-26): “이 부분은 문제가 아닙니다. 왜냐면 엔진 순정으로 제공하는 매크로이기 때문입니다.” AttributeSet 접근자 지적은 철회한다. 엔진 순정 제공 매크로가 생성하는 인라인 함수는 인라인 금지 대상이 아니며, 이 해석을 AGENTS.md 코딩 규칙 3에 명시했다.
## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 기존 지적 재대조 | 현재 C++와 로컬 UE 5.8 엔진 구현 확인 | AI | 통과 | 현재 미해결 네 지적의 근거를 확인하고 콤보 공통 데이터 이동과 행 번호를 반영했다. |
| 생성 참조 목록 | Export-AbilitySystemLists.ps1 실행 | AI | 통과 | 목록 3개 unchanged, 40 abilities·9 sets·7 characters |
| 지적 수용 여부 | 실패 반환·중첩 배율·클라이언트 쿼리·중복 판단 | 사람 | 대기 | 소스 수정은 별도 요청으로 진행한다. |

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Combo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SlowTime.cpp`.
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, 콤보·Attack·Skill 공개 헤더, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`의 재생 후 처리. 로컬 UE 5.8의 PlayMontageAndWait 실패 콜백과 AttributeSet 매크로도 확인했다.
- **미검토 / 한계**: 제공된 HEAD `ad0db6de0`와 현재 미커밋 작업 트리를 기준으로 확인했다. 203개 소스 전체를 통독하지 않았으며, 나머지 피해 계산·투사체·컷신·소환물·Cue·타겟팅·GE·Context 직렬화는 이번에 깊이 재검토하지 않았다. 빌드·PIE·네트워크 재현·BP 내부 검증은 하지 않았다. 생성 참조 목록과 Wiki는 경계 확인에 사용했다.

---
*문서 기준 커밋 `ad0db6de0` · 리뷰일 2026-09-26 · 소스 203파일 — `/module-review`로 갱신*