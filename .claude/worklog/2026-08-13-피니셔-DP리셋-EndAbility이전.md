# 피니셔 DP 리셋을 EndAbility 무조건 적용으로 이전

## 계획

### 목표

그로기 해제(`UWxEffect_ResetDP`)가 앞잡 공격 몽타주의 `OnCompleted`에만 걸려 있어, 몽타주가 인터럽트·캔슬로 끝나면 리셋이 누락된다. 처형 자격은 `State.Groggy` 보유 + `State.Finisher` 부재로 판정하므로 연출만 끊긴 채 적이 그로기로 남아 남은 그로기 시간 동안 처형이 다시 걸린다. 리셋 시점을 어빌리티 종료로 옮기고, 앞잡·뒤잡 구분 없이 무조건 적용한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Finisher.h` | `HandleFinisherMontageCompleted` 선언 삭제, 클래스 주석의 DP 리셋 서술 갱신 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/Ability/WxAbility_Finisher.cpp` | `OnCompleted` 바인딩 분기 제거, `EndAbility`에서 무조건 리셋, `HandleFinisherMontageCompleted` 정의 삭제 | 수정 |

### 접근 방식

- **리셋 위치는 `EndAbility`의 기존 권위 블록 안**: 이미 `IsNetAuthority()` 게이트 안에서 대상 ASC를 얻어 `State.Finisher`를 떼고 있다. 같은 조회로 `UWxEffect_ResetDP`까지 적용하면 새 게이트도 새 조회도 필요 없고, 정상 종료·인터럽트·캔슬이 전부 이 경로를 지나므로 종료 사유와 무관하게 한 번 적용된다. 적용은 `Super::EndAbility` 앞, `TargetActor`를 비우기 전이다.
- **적용 코드는 기존 것을 그대로 옮긴다**: `MakeEffectContext` → `MakeOutgoingSpec` → `ApplyGameplayEffectSpecToTarget` 3단을 이동한다. 소스 ASC는 `CurrentActorInfo`를 타는 `GetAbilitySystemComponentFromActorInfo()` 대신 인자로 받은 `ActorInfo->AbilitySystemComponent`에서 얻는다.
- **콜백은 하나로 합친다**: 변형 분기가 사라져 `OnCompleted`·`OnInterrupted`·`OnCancelled` 셋 다 `HandleMontageFinished`에 바인딩된다.
- **뒤잡 대상의 누적 DP도 리셋된다**: 그로기가 아닌 적을 백스탭하면 쌓아둔 DP가 0으로 돌아간다. 구분 없이 무조건 적용하기로 한 결정에 따른 것이다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbility_Finisher.h` | `HandleFinisherMontageCompleted` 선언 삭제, 클래스 주석의 변형별 차이·DP 리셋 서술 갱신 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/Ability/WxAbility_Finisher.cpp` | `OnCompleted`도 `HandleMontageFinished`에 바인딩(변형 분기 삭제), `EndAbility` 권위 블록에서 `UWxEffect_ResetDP` 적용, `HandleFinisherMontageCompleted` 정의 삭제 | 수정 |
| `Plugins/WxCombat/.../Public/AbilitySystem/Effect/WxEffect_ResetDP.h` | 적용 시점을 "앞잡 몽타주 정상 완료"에서 "피니셔 종료"로 정정 | 수정 |

### 구현·결정과 그 이유
- **리셋을 대상 태그 해제와 같은 블록에 뒀다**: `EndAbility`의 권위 게이트 안에서 이미 대상 ASC를 조회해 `State.Finisher`를 떼고 있었다. 여기 얹으면 게이트도 조회도 하나로 끝나고, 그로기 해제와 어포던스 복구가 항상 같은 프레임에 함께 일어나 둘이 어긋날 여지가 없다.
- **소스 ASC를 `ActorInfo` 인자에서 얻었다**: 옮겨온 코드는 `GetAbilitySystemComponentFromActorInfo()`를 쓰고 있었는데, 그건 `CurrentActorInfo`를 타고 `EndAbility`는 그 값이 정리되는 경로다. 인자로 받은 쪽이 이 시점에 확실하다.
- **뒤잡도 무조건 리셋(사용자 지시)**: 그로기가 아닌 적을 백스탭하면 쌓아둔 DP가 0으로 돌아간다. 변형 구분을 없애 종료 경로를 하나로 만드는 쪽을 택했다.
- **캔슬 종료도 리셋한다**: 피격 등으로 연출이 끊겨도 그로기가 풀린다. 연출이 끊긴 채 그로기만 남아 처형이 다시 걸리던 구멍이 이걸로 닫힌다.

### 계획 대비 달라진 점
- `WxEffect_ResetDP.h`의 낡은 주석 정정 1건을 추가했다. 적용 시점을 못 박아 둔 서술이라 그대로 두면 코드와 어긋난다.

### 후속 과제
- **PIE 미검증**: 컴파일까지만 확인했다. 앞잡 완주 시 그로기 해제(회귀), 앞잡 캔슬 시 그로기 해제·프롬프트 재노출 없음, 뒤잡 대상 DP 0.
- **피니셔 후딜 콤보 배선**: 이 수정이 열어준 작업. `AM_Finisher`에 `AN_StartRecovery` + `ANS_ComboWindow`를 놓고 `GA_Attack_Light`에 `State.Finisher` + `ANS.ComboWindow` 세트를 추가하면 피니셔 후딜에서 전용 콤보로 이어진다. 창 위치는 피해자 짝 피격 몽타주가 마무리되는 근처여야 그림이 어긋나지 않는다.
