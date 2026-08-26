# ActivateAbility BT Task — 재발동 종료 통지를 결론으로 삼지 않게 수정

## 계획

### 목표
`module_review_WxAI.md` 이슈 4. `UWxBTTask_ActivateAbility` 는 `TryActivateAbility` 호출 **전에** `ActivatedHandle` 을 세우고, `HandleAbilityEnded` 는 스펙 핸들만으로 "내 실행이 끝났다"를 판별한다. 그런데 엔진의 재발동 경로는 **같은 핸들로 기존 실행을 먼저 끝낸 뒤** 새로 활성화한다. 그래서 발동 구간에 들어온 종료 통지가 "지금 시작한 실행"의 것이 아닐 수 있는데, 현재 코드는 그 통지를 결론으로 믿고 즉시 태스크를 끝낸다. 결과적으로 **어빌리티는 계속 도는데 BT 는 다음 행동으로 넘어가고**(공격 모션 위에 이동·배회가 겹침), `CleanUp()` 이 이미 `ActivatedHandle` 을 비운 탓에 `AbortTask` 로 취소할 수도 없다.

리뷰는 확신도를 "중간(엔진 소스가 없어 미확인, 기본값 false 라 저확률)"으로 적었지만, UE 5.8 소스와 프로젝트 양쪽을 대조해 **둘 다 확정**했다.

- 엔진(`AbilitySystemComponent_Abilities.cpp:1832-1852`): `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성이면 `InstancedAbility->EndAbility(..., bWasCancelled=false)` 를 먼저 부른다.
- 그 `EndAbility` 는 `NotifyAbilityEnded` → `OnAbilityEnded.Broadcast(FAbilityEndedData(Ability, Handle, false, false))` 로 **같은 핸들·정상 종료**를 동기 브로드캐스트한다(`GameplayAbility.cpp:894`, `AbilitySystemComponent_Abilities.cpp:1254`).
- 프로젝트에서 `bRetriggerInstancedAbility = true` 인 어빌리티는 `WxAbility_Attack`·`WxAbility_Skill`·`WxAbility_HitReact` 셋이다. 즉 "디자이너가 켤 수도 있는 옵션"이 아니라 **AI 가 BT 로 발동하는 공격·스킬이 바로 그 대상**이다.

종료 판정을 "통지가 왔는가"가 아니라 **"실제로 도는 실행이 있는가"** 로 바꾼다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../WxBTTask_ActivateAbility.cpp` | `ExecuteTask` 결론 순서 교체, `HandleAbilityEnded` 의 `CleanUp()` 위치 이동 | 수정 |

헤더 변경 없음 — `bIsActivating`·`ActivationResult` 는 그대로 쓰고 의미만 좁아진다(주석 문구만 갱신).

### 접근 방식
리뷰가 제시한 두 갈래를 **함께** 적용한다. 어느 한쪽만으로는 부족하다 — 통지를 늦게 믿기만 하면 콜백이 이미 구독을 끊어 `AbortTask` 가 무력해지고, `CleanUp` 만 미루면 낡은 결과가 여전히 결론이 된다.

- **발동 구간의 콜백은 결과만 적는다.** `HandleAbilityEnded` 에서 `CleanUp()` 을 `bIsActivating` 분기 **뒤로** 옮긴다. 발동 구간에서는 구독도 `ActivatedHandle` 도 살아 있는 채로 `ActivationResult` 만 남기고 빠지며, 정리는 `ExecuteTask` 가 결론을 낼 때 한다. 비(非)발동 구간의 동작(`CleanUp` 후 `FinishLatentTask`)은 그대로다 — 재진입 시 구독이 겹치지 않게 하는 기존 순서를 유지한다.
- **결론은 "지금 도는 실행이 있는가"가 낸다.** `ExecuteTask` 에서 `ActivationResult` 를 먼저 믿던 분기를 지우고, 순서를 다음으로 바꾼다.
  1. `ActivatedHandle` 이 무효 → 아무것도 발동하지 못했다 → `CleanUp()` + `Failed`
  2. 핸들 재조회 후 스펙이 **활성** → 재발동이든 신규든 지금 도는 실행이 있다 → 구독·핸들을 살린 채 `InProgress`
  3. 비활성 → 발동 구간 안에서 끝난 것이다 → 통지를 받았으면 그 결과, 못 받았으면(스펙 제거 등 고착 방어) `Failed`. 어느 쪽이든 `CleanUp()` 후 반환

이 순서면 재발동 경로에서 낡은 `Succeeded` 는 자연히 버려지고, 구독과 `ActivatedHandle` 이 살아 있으므로 실제 종료 시 `FinishLatentTask` 가 돌고 `AbortTask` 도 정확히 그 실행을 취소한다. `FindAbilitySpecFromHandle` 재조회는 활성화 중 배열 재할당 대비로 이미 있던 것이라 그대로 재사용한다.

경로별 결론:

| 경로 | 스펙 활성 | 결론 |
|---|---|---|
| 정상 비동기 발동 | O | `InProgress` (콜백이 나중에 마감) |
| 즉발 어빌리티 동기 종료 | X | 통지 결과(`Succeeded`/`Failed`) — 2026-08-12 수정 유지 |
| **재발동(기존 실행 종료 → 재활성)** | **O** | **`InProgress`** ← 이번에 바뀌는 지점 |
| 재발동 후 새 실행도 동기 종료 | X | 마지막 통지 결과 |
| 후보 전멸 / 스펙 소실 | — | `Failed` |

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../WxBTTask_ActivateAbility.cpp` | `ExecuteTask` 결론 순서를 "스펙 활성 → 통지 결과" 로 교체, `CleanUp()` 을 콜백의 발동 구간 분기 뒤로 이동 | 수정 |
| `Plugins/WxAI/.../WxBTTask_ActivateAbility.h` | `bIsActivating`·`ActivationResult` 주석 갱신(멤버·시그니처 변경 없음) | 수정 |

### 구현·결정과 그 이유
- **결론의 주체를 통지에서 상태로 옮겼다.** 스펙 핸들은 실행 단위가 아니라 부여 단위라, 통지만으로는 "내가 방금 시작한 실행이 끝났다" 를 판별할 수 없다. 재발동이 같은 핸들로 이전 실행을 먼저 끝내기 때문이다. 반대로 "지금 도는 실행이 있는가" 는 재조회 한 번으로 명확히 답이 나오고, 발동 구간이 끝난 시점에는 그 답이 곧 결론이다. 통지는 이제 비활성일 때 성공·캔슬을 가르는 보조 정보로만 쓰인다.
- **`CleanUp()` 을 발동 구간 콜백에서 뺐다.** 구독과 `ActivatedHandle` 이 살아 있어야 재발동으로 새로 시작한 실행을 계속 관찰하고 `AbortTask` 로 정확히 취소할 수 있다. 비(非)발동 구간에서는 `FinishLatentTask` 로 노드가 즉시 재진입할 수 있어 기존의 선(先)해제 순서를 유지했다.
- **엔진 소스로 전제를 확정했다.** 리뷰가 미확인으로 남긴 재발동 경로(`AbilitySystemComponent_Abilities.cpp:1836`)와 그 종료가 같은 핸들·`bWasCancelled=false` 로 동기 브로드캐스트된다는 점(`:1254`)을 UE 5.8 설치본에서 대조했다. 프로젝트의 `WxAbility_Attack`·`WxAbility_Skill`·`WxAbility_HitReact` 가 이미 재발동을 켜 두어 가정이 아닌 실사용 경로다.
- **더 정밀한 대안은 버렸다.** `InternalTryActivateAbility` 의 실행별 종료 델리게이트는 실행 단위를 정확히 짚지만 `bWasCancelled` 를 싣지 않아 성공·실패를 가를 수 없고, 활성 여부를 보는 방법이 기존 재조회를 그대로 재사용하므로 더 싸다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 인게임 검증 미완(컴파일만 확인). 콤보 몽타주가 있는 공격 어빌리티를 AI BT 로 태워 어빌리티 진행 중 다음 노드로 넘어가지 않는지, 즉발 어빌리티의 Sequence 후속 노드 실행이 그대로인지 확인이 필요하다.
- `ExecuteTask` 반환 이후 **바깥에서** 같은 어빌리티를 재발동하면 여전히 남의 종료를 내 것으로 오인한다. 지금 AI 폰의 발동 진입점은 이 태스크뿐이라(입력 라우팅·UI 는 플레이어 전용, `OnGiven` 은 부여 시 1회) 실재 경로가 없어 두었다.
