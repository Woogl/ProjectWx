# WxCombat — 코드 리뷰

> 소환 로스터의 재진입 안전성과 카메라 연출 수명, 패턴 실패 종료에서 개선할 지점을 확인했다. 변경된 소환·쿨다운·타겟팅 프리뷰와 기존 주요 발견을 재검토하고, 어빌리티 기반·ASC·대미지 계산 및 반응 경로를 집중적으로 읽었다. 현재 작업 트리의 미커밋 변경을 포함한다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 소환물 초기화 중 로스터가 늘어나면 보관한 배열 참조가 무효화될 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:40`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:72`
- **범주**: 버그/정확성
- **문제**: `Rosters.FindOrAdd(&Master)`의 값 참조를 보관한 채 `FinishSpawning`을 호출하고 74행에서 그 참조에 추가한다. 소환물의 초기화·BeginPlay에서 다른 주인의 소환을 요청하면 같은 TMap에 새 항목이 추가되고 저장 공간 재할당에 의해 기존 참조가 무효화될 수 있다. 그러면 바깥 호출의 `Minions.Add`가 유효하지 않은 배열을 건드린다. 앞선 기존 소환물 `Destroy` 역시 동기 EndPlay 재진입을 허용하므로 그 구간에서도 장기 참조에 주의해야 한다.
- **제안**: 외부 수명주기 호출을 넘겨 TMap 값 참조를 보관하지 않는다. 제거 대상을 별도로 확보하고, 초기화 완료 뒤 주인과 소환물의 유효성을 검사한 다음 로스터를 다시 조회해 등록한다.
- **확신도**: 중간(참조 무효화 가능성은 명확하나 중첩 소환의 실제 에셋 구성은 확인하지 않았다)

### 2. 🟡 카메라 노티파이가 생성한 카메라의 수명과 뷰 소유권을 추적하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:69`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:180`
- **범주**: 버그/정확성
- **문제**: 임시 카메라 수명은 `TotalDuration + BlendOutTime + 1`로 고정되지만 몽타주는 ASPD에 따른 가변 속도로 재생된다. 충분히 느리게 재생하면 구간 종료 전에 카메라가 파괴될 수 있다. 또한 NotifyEnd는 생성했던 카메라를 확인하지 않고 무조건 폰으로 뷰를 돌려, 두 카메라 구간이 겹치면 먼저 끝난 구간이 뒤 구간의 뷰를 해제한다. Begin에서 생성에 실패한 경우에도 같은 복귀가 실행된다.
- **제안**: 액터별 연출 상태에 카메라와 이전 뷰를 보관하고 자신이 현재 뷰를 소유할 때만 복구한다. 구간 종료와 카메라 정리를 연결하고 안전망 수명에도 재생 속도를 반영한다.
- **확신도**: 높음

### 3. 🟡 패턴 연쇄 몽타주 재생 실패를 정상 종료로 처리해 다음 발동 인덱스가 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:55`
- **범주**: 버그/정확성
- **문제**: `HandleMontageBlendOut`이 다음 슬롯으로 인덱스를 증가시킨 뒤 재생에 실패하면 58행에서 `bWasCancelled=false`로 종료한다. `EndAbility`는 취소일 때만 인덱스를 초기화하므로, 중간 슬롯이 비어 있거나 재생에 실패한 경우 다음 발동이 29행에서 그다음 슬롯부터 시작한다. 초기 발동의 재생 실패는 취소로 끝내는 처리와도 다르다.
- **제안**: 연쇄 재생 실패도 취소 종료로 처리해 기존 인덱스 초기화 경로를 사용한다. 세 슬롯 중 두 번째가 비어 있는 구성으로 다음 발동이 첫 슬롯에서 시작하는지 검증한다.
- **확신도**: 높음

### 4. 🟢 콤보 창에서 진입 조건뿐 아니라 차단 조건도 함께 면제된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:169`
- **범주**: 설계/구조
- **문제**: 활성 상태의 ComboWindow이면 태그 검사를 부모에 넘기지 않고 즉시 true를 반환한다. 헤더가 설명하는 진입 조건 면제보다 넓어, 공격 BP에 설정한 `ActivationBlockedTags`도 콤보 재발동 중에는 검사하지 않는다. 후딜의 설명은 해당 차단 태그를 유지한다고 명시하여 저작자가 두 구간의 차이를 놓치기 쉽다.
- **제안**: 면제 대상이 필수 진입 태그뿐인지 차단 태그까지인지 정한다. 전자라면 차단 검사를 유지하고, 후자라면 헤더에 면제 범위를 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingPreview.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`.
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxTargetingPreview.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SnapToTarget.cpp`.
- **미검토 / 한계**: 전 파일 통독·빌드·PIE·네트워크 실행 검증은 하지 않았다. DT/BP·몽타주 배치·나머지 어빌리티·태스크·무기·히트스톱은 이번 집중 검토 범위 밖이다. 기존 크리티컬 난수 지적은 LocalPredicted만으로 실행 계산이 양쪽에서 실행된다고 단정한 근거가 부족하고 현재 플로터도 서버 실행 결과 경로를 사용하므로 이월하지 않았다. 플로터 생성 비용은 코드상 존재하지만 실제 부하 자료 없이 성능 결함으로 확정하지 않았다. 기존 메시 틱 복원·락온 ASC 무효화·비-Pawn 반사 항목은 해당 분기를 다시 확인했으나 실제 도달 경로와 영향의 근거가 부족하여 결과에서 제외했다. 이는 수정 완료 판정이 아니다. 소환 중 재진입과 콤보 면제의 실제 저작 의도는 추가 확인이 필요하다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 177파일 — `/module-review`로 갱신*
