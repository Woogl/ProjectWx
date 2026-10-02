# WX 프로젝트 전체 코드 리뷰 — 2026-10-02

## 결과

텍스트 코드·설정 **538개 파일, 41,559줄을 모두 읽었다**. 결함 후보 117건은 독립 검증자가 반증을 시도했고, 99건이 확정됐다. 크래시·세이브 손상·일반 진행 불가(P0)는 없었다. 기준 커밋은 `af83fcefc`다.

수정을 권고하는 결함은 **P1 1건, P2 10건**이다. 아래 표는 중요도 순서이고, 같은 중요도 안에서는 확신도 순서다.

| ID | 중요도 | 확신도 | 문제 | 확인 방법 |
|---|---|---|---|---|
| R1 | P1 | 상 | 일반 피격이 진행 중인 다른 어빌리티의 노티파이 구간을 버림 | 코드·엔진 소스·몽타주 슬롯 대조, 검증자 2인 확정 |
| R2 | P2 | 상 | 퍼펙트 가드 직후 GuardReact 내내 퍼펙트 가드 판정이 유지됨 | R1과 같은 원인, 가드 몽타주 노티파이 시각 파싱 |
| R3 | P2 | 상 | 회피 반격 1단 후딜이 시작되면 2단 대신 약공격 1단이 나감 | 몽타주 노티파이 시각 파싱, AbilitySet 순서 대조 |
| R4 | P2 | 상 | HGTest 회피 반격 2단이 피해를 주지 않음 | DT_Damage 행 이름과 몽타주 참조 대조, git 이력 |
| R5 | P2 | 상 | 일반 피격으로 아이템 사용이 끊기지 않음 | 취소 목록·몽타주 슬롯 그룹·위키 규칙 대조 |
| R6 | P2 | 상 | 회복 전에 끊긴 포션이 소비되지 않음 | 차감 시점 추적, 기획서·git 이력 대조 |
| R7 | P2 | 상 | 분신이 가드를 따라 하면 가드 자세로 굳음 | BT 에셋 파싱, 가드 종료 경로 추적 |
| R8 | P2 | 상 | 스포너 셀이 다시 로드되면 처치 퀘스트가 끝나지 않음 | WP 설정·스포너 배치·태스크 판정 추적 |
| R9 | P2 | 중 | 셀이 다시 로드되면 퀘스트 볼륨이 다시 발동해 퀘스트가 재시작됨 | BP 그래프는 uasset 문자열로만 확인, WP 설정 대조 |
| R10 | P2 | 중 | 원격 클라에서 선입력한 후딜 캔슬이 서버에서 거부됨 | 엔진 CMC·GAS 실행 순서 추적(실행 미확인) |
| R11 | P2 | 중 | 원격 클라의 회피 반격 2단이 항상 서버에서 거부됨 | 엔진 재발동 RPC 순서 추적(실행 미확인) |

**중요도** — P1: 일반 전투 흐름에서 자주 깨진다. P2: 구체적인 조건에서 기능 오류가 난다. P3: 드물거나 경미하거나, 현재 에셋에서는 드러나지 않는다.

**확신도** — 상: 코드 경로와 현재 에셋 값만으로 결과가 정해진다. 중: 결론이 실행 순서·타이밍 같은 엔진 동작 추론이나, 에디터에서 직접 열어 보지 않은 에셋 내용에 기댄다. 중인 항목은 실행 재현으로 확정하는 것이 좋다.

그 밖의 결과는 다음과 같다.

| 구분 | 건수 | 비고 |
|---|---:|---|
| P3 — 지금 플레이에서 드러남 | 16 | R12~R27 |
| P3 — 기획 확인이 필요한 차이 | 8 | R28~R35, 수치·규칙이 기획과 다르고 의도 기록이 없음 |
| P3 — 현재 에셋에서는 드러나지 않음 | 10 | R36~R45, 코드상 결함은 확실함 |
| 도구 결함 | 12 | T1~T12, 게임 런타임 영향 없음 |
| 품질 개선 | 40 | Q1~Q40 |
| 기각 / 판정 보류 | 16 / 2 | X1~X16, 같은 지적이 반복되지 않도록 사유를 남김 |

**함께 고쳐야 하는 항목이 있다.** R1과 R2는 같은 조건 하나가 원인이다. R5만 고치면 일반 피격이 아이템 사용을 끊게 되어, 포션이 줄지 않는 R6이 더 자주 드러난다. R9만 고치면(볼륨 상시 로드) 지금 R8을 우연히 풀어 주는 경로가 사라진다.

AGENTS.md 코딩 규칙 1~3(Wx 접두사, 저작권 첫 줄, 인라인 함수 정의 금지)은 전수 검사에서 모두 통과했다. 사용자 규칙인 익명 namespace·static 자유 함수 금지는 13개 파일에서 어긴다(Q1).

## 검토 기준과 범위

- 대상은 `Source/` 전체, 자체 플러그인 3개, `Wx.uproject`, `Config/`, `BatchFiles/`, `.agents/`의 스크립트다. 단위별 집계는 [부록](#부록-검토-단위)에 있다.
- 19개 파일 단위로 나눠 각 단위의 모든 파일을 읽고, 호출 관계를 범위 밖까지 추적했다. 5개 횡단 관점(네트워크·권위, 수명·델리게이트, GAS 종단 흐름, 저장·부활·월드 상태, 태그·설정·프레임 비용)으로 모듈을 가로질러 흐름도 따라갔다.
- 후보 128건 중 중복 4건을 병합했다. 규칙 위반 7건은 전수 검사 결과 하나로 묶었다. 남은 117건은 검증자가 코드를 직접 읽고 반증을 시도했다. P1·P2로 보고된 20건은 앞선 판정을 모르는 2차 검증자가 다시 확인했다. 의견이 갈린 2건은 3차 판정에서 둘 다 기각됐다.
- 판단 근거는 사용자 메모리의 설계 결정, Wiki, Docs 기획서, git 이력, 어빌리티·이펙트·캐릭터 목록(리뷰 시작 때 Export-AbilitySystemLists.ps1로 재생성), 에셋 바이너리 파싱, UE 5.8 엔진 소스다. 메모리에 의도·결정으로 기록된 동작은 결함으로 세지 않았다.
- **정적 리뷰다.** PIE·멀티플레이로 재현하지 않았고, 성능 프로파일링·패키징·전체 클린 빌드도 하지 않았다. 블루프린트 그래프·StateTree·레벨 배치는 판정에 필요한 것만 확인했다.
- 놓친 흐름을 다시 찾는 2차 점검은 사용량 한도 때문에 생략했다. 기획서 대비 점검도 따로 돌리지 않았고, 단위 리뷰어가 담당 범위에서 함께 봤다.
- 링크의 줄 번호는 리뷰 시점 기준이다. ID(R·T·Q·X)는 이 문서에서만 쓰는 번호다.

## R1. [P1 · 확신도 상] 일반 피격이 진행 중인 다른 어빌리티의 노티파이 구간을 버림

**발생 조건:** 적이 패턴 단계 몽타주(예: AM_Soldier_Pattern_1_2)를 재생하는 중에 플레이어 평타에 맞는다. 플레이어 평타 행은 모두 `HitReact.Normal`이라 일반 전투에서 자주 일어난다. 회피·아이템 사용 중의 일반 피격도 같다.

`OwnsMontageSignal`은 몽타주 인스턴스 ID가 일치해도 `ASC->GetAnimatingAbility() == Ability`를 함께 요구한다([WxAbilityTask_MontageEvents.cpp:83](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L83)). 일반 피격 몽타주 AM_Shared_HitReact는 AdditiveHitReact 슬롯이라 패턴 몽타주(DefaultSlot)를 끊지 않는다. 그런데 `ASC::PlayMontage`는 슬롯과 상관없이 AnimatingAbility를 HitReact로 덮어쓴다. HitReact가 끝나면 엔진은 이 값을 nullptr로 비울 뿐, 원래 어빌리티로 되돌리지 않는다.

그 사이 원래 어빌리티의 구간 시작·종료 신호가 모두 버려진다. 열린 구간은 태스크가 끝날 때에야 회수된다. [IsPlayingMontageInstance](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L228)도 `ASC->GetCurrentMontage()`를 기준으로 삼아, 같은 이유로 StartRecovery·ComboWindow를 무시한다.

`6a2dec191`(10-01, 몽타주 노티파이 실행을 어빌리티의 몽타주 이벤트 태스크로 통합) 이전에는 노티파이가 AnimatingAbility와 상관없이 MontageInstanceID로 처리됐다. 그 통합에서 생긴 회귀다.

**영향:** 판정이 아직 열리지 않았으면 그 단계의 WeaponAttack이 끝내 열리지 않아, 적 공격이 피해를 주지 않는다. 이미 열려 있었으면 구간이 끝나도 닫히지 않아, 후딜 중인 무기에도 맞는다. 패턴·회피·아이템의 후딜 캔슬도 사라진다. 아이템 회복은 `WxAnimNotify_UseItem` 경로라 영향이 없다.

**수정 방향:** `OwnsMontageSignal`에서 AnimatingAbility 조건을 빼고, `OwnedMontageInstanceID`(전역 고유)와 메시 일치만으로 판정한다. 끊긴 인스턴스는 엔진이 새 노티파이를 모으지 않으므로 정지 여부를 따로 검사할 필요가 없다. `IsPlayingMontageInstance`는 제거하거나 `GetMontageInstanceForID(MontageInstanceID)`의 활성 여부로 바꾼다. 같은 기준을 쓰는 [WxAbilityTask_Rush.cpp:97](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L97)도 함께 점검한다.

**수정 후 확인:** 패턴 예비 동작 중에 평타로 맞힌 뒤에도 그 단계의 공격이 피해를 주고, 판정이 구간 끝에서 닫히는지 확인한다. 피격이 없을 때의 패턴·콤보·후딜 캔슬은 그대로여야 한다. R2도 함께 확인한다.

## R2. [P2 · 확신도 상] 퍼펙트 가드 직후 GuardReact 내내 퍼펙트 가드 판정이 유지됨

**발생 조건:** 퍼펙트 가드가 날 때마다 생긴다. GuardReact(PerfectGuard 섹션) 중에 막을 수 있는 공격이 한 번 더 들어오면 드러난다. SlowTime 때문에 실제 시간은 더 길다.

원인은 R1과 같다. GuardReact가 DefaultSlot 몽타주로 가드 몽타주를 끊으면 AnimatingAbility가 GuardReact로 바뀐다. 끊긴 가드 몽타주의 ApplyGameplayEffect 구간 종료 신호가 다음 틱에 와도 Guard의 태스크가 버린다. Guard는 GuardReact 때문에 끊기면 종료하지 않고 대기한다([WxAbility_Guard.cpp:73](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L73)). 그래서 Infinite GE인 `WxEffect_PerfectGuard`가 GuardReact가 끝날 때까지 남는다.

**영향:** 0.25초로 저작한 퍼펙트 가드 구간이 반응 몽타주 길이만큼 늘어난다. 연속 공격의 다음 1타가 입력 타이밍과 상관없이 퍼펙트 가드(반사 GP, 공격자 패리, 투사체 되돌림, 슬로모션)로 처리된다. 서버 판정이라 모든 머신에 복제된다.

**수정 방향:** R1 수정으로 함께 해결된다. Guard 쪽에 우회 장치를 따로 넣지 않는다.

**수정 후 확인:** 퍼펙트 가드 직후 GuardReact 중에 들어온 타격이 일반 가드로 판정되고, `Effect.PerfectGuard`가 0.25초 뒤 사라지는지 확인한다.

## R3. [P2 · 확신도 상] 회피 반격 1단 후딜이 시작되면 2단 대신 약공격 1단이 나감

**발생 조건:** 모든 머신에서 생긴다. 회피 반격 1단을 발동한 뒤 StartRecovery(0.421초)부터 콤보 창 끝(0.967초) 사이에 약공격을 누른다. 시각은 ASPD 1 기준이며, AM_HGTest_Attack_DodgeCounter1과 AM_Template_DodgeCounter_1 모두 해당한다.

콤보 재발동은 `ActionPhase == ComboWindow`일 때만 소유자 조건을 면제받는다([WxAbilityBase.cpp:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)). StartRecovery가 창 도중에 오면 남은 구간은 Recovery가 되고 차단이 풀린다. 이때 회피 반격은 `Ability.Action.Dodge`를 요구하는데, 회피는 1단 발동 때 이미 취소됐다. 같은 입력의 약공격은 AbilitySet 순서상 먼저 시도되고, 차단이 풀려 있어 통과한다([WxAbilitySystemComponent.cpp:181](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L181)).

| 구간(초) | 단계 | 약공격 입력 결과 |
|---|---|---|
| 0.348~0.421 | ComboWindow | 회피 반격 2단 |
| 0.421~0.967 | Recovery(창은 아직 열림) | **약공격 1단** |

**영향:** 2단(HGTest는 분신 소환 포함)에 들어갈 수 있는 구간이 약 73ms와 그 전 선입력뿐이다.

**수정 방향:** 두 가지를 함께 바꾼다. 첫째, 면제를 "활성 인스턴스이고 창이 아직 닫히지 않음"으로 판정하고, 차단 1건 제외는 Occupant가 있을 때만 적용한다. 지금 구조에서 `bComboRetrigger`만 Recovery까지 넓히면 253~258행이 null Occupant를 역참조한다. 둘째, 같은 입력에서는 창이 열린 활성 콤보 스펙을 먼저 시도한다. 데이터만으로 막으려면 ComboWindow 끝을 StartRecovery 앞으로 당기면 되지만, 후딜 캔슬 시작이 늦어지므로 기획 확인이 필요하다.

**수정 후 확인:** 콤보 창 전 구간에서 약공격을 누르면 2단이 나가고, 창이 닫힌 뒤에는 약공격 1단이 나가는지 확인한다. 일반 약공격 콤보도 그대로인지 확인한다.

## R4. [P2 · 확신도 상] HGTest 회피 반격 2단이 피해를 주지 않음

**발생 조건:** 프론트엔드에서 BP_HGTest를 고르고 회피 반격 2단(AM_HGTest_Attack_DodgeCounter2)을 적에게 맞힌다.

[AM_HGTest_Attack_DodgeCounter2](../../Content/Character/HGTest/Abilities/Attack_DodgeCounter/AM_HGTest_Attack_DodgeCounter2.uasset)의 WeaponAttack 노티파이는 `DT_Damage.AM_Attack_LLLL`을 가리키는데, 이 행이 DT_Damage에 없다. `a9bfa4625`(09-15, 에셋 테이블에도 Template_ 붙임)에서 행 이름이 `AM_Template_Attack_LLLL`로 바뀌었는데, 이 몽타주만 갱신되지 않았다. DataTableRowFixup 도입(09-23)보다 앞선 일이다. 행을 찾지 못하면 [ApplyDamage](../../Source/WxGame/Combat/WxCombatLibrary.cpp#L71)가 GE 없이 false를 반환한다. 어빌리티·이펙트 목록 생성기도 이 참조를 "표에 없는 행을 가리키는 참조"로 표시하고 있다.

**영향:** 마무리 타격이 피해·피격 반응·히트스톱·큐를 모두 내지 않는다. 적중할 때마다 FindRow 경고만 남는다.

**수정 방향:** DamageDataRow를 있는 행으로 바꾼다. 원래 수치는 `AM_Template_Attack_LLLL`이다. HGTest 전용 수치가 필요하면 DT_Damage에 행을 추가한다. 쓰이지 않는 번호 없는 AM_HGTest_Attack_DodgeCounter도 같은 행을 가리킨다(Q39).

**수정 후 확인:** 2단 적중 시 피해·피격 반응·히트스톱이 나고 FindRow 경고가 없는지 확인한다. 목록을 다시 생성해 "표에 없는 행" 항목이 사라졌는지도 본다.

## R5. [P2 · 확신도 상] 일반 피격으로 아이템 사용이 끊기지 않음

**발생 조건:** 포션을 마시는 도중(회복 노티파이 0.276초 전)에 적 공격을 받는다. 현재 적 피해 행은 모두 `HitReact.Normal`이다. 무적이 끝난 회피 후딜(0.5~0.9초)에 맞아도 같다.

HitReact의 `CancelAbilitiesWithTag`에는 `Ability.Action.Attack`과 `Ability.Action.Skill`만 있다([WxAbility_HitReact.cpp:24](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L24)). 일반 피격 몽타주는 가산 슬롯 그룹이라 UseItem(UpperBody)·Dodge(DefaultSlot) 몽타주도 끊지 않는다. 예전에는 피격 몽타주가 같은 슬롯을 덮어 액션이 저절로 끝났지만(`2bbbea01a`), 일반 피격이 가산 슬롯으로 옮겨지면서 이 전제가 깨졌다.

**영향:** 피격 중에도 포션 회복이 끝까지 적용된다. 위키·기획의 "아이템 사용: 피격 시 캔슬 O" 규칙이 동작하지 않는다.

**수정 방향:** `CancelAbilitiesWithTag`에 `Ability.Action.UseItem`을 추가한다. 무적 밖 회피도 끊는 것이 의도라면 `Ability.Action.Dodge`도 추가한다. 위키 표기가 "△(무적 프레임)"라 이 부분은 확인이 필요하다. 무적 구간에서는 `WxEffect_Invincible`이 HitReact 발동 자체를 막으므로 무적 프레임 예외는 유지된다. **R6과 함께 고친다.**

**수정 후 확인:** 포션 사용 중 일반 피격으로 사용이 끊기는지, 무적 프레임 회피는 끊기지 않는지 확인한다.

## R6. [P2 · 확신도 상] 회복 전에 끊긴 포션이 소비되지 않음

**발생 조건:** 포션 사용 입력 뒤, 회복 노티파이 전에 어빌리티가 끊긴다. 지금은 넉 계열 피격(같은 슬롯 그룹의 UseItem 몽타주를 멈춤), 사망, 그로기가 해당한다. R5를 고치면 일반 피격도 해당한다.

발동 시에는 `BeginUseItem`으로 대기 플래그만 세운다([WxAbility_UseItem.cpp:59](../../Source/WxGame/Inventory/WxAbility_UseItem.cpp#L59)). 충전 차감과 회복 GE 적용은 모두 노티파이 시점의 `UseConsumable` 한 곳에서 일어난다([WxItemUseComponent.cpp:85](../../Source/WxGame/Inventory/WxItemUseComponent.cpp#L85)). 노티파이 전에 끝나면 `EndUseItem`이 플래그를 지워 아무것도 차감되지 않는다.

에스트병 기획서(05-30)는 "GA 실행 시 1 감소, 회복 전 피격은 포션만 소비"다. 구현(`43860c8aa`, 06-10)은 처음부터 노티파이 시점 차감이었고, 이 차이를 의도로 정한 기록은 없다.

**영향:** 맞아도 포션이 줄지 않아, 기획 목적인 "사용 중 빈틈의 리스크"가 사라진다. 보스 앞에서 실패해도 손실 없이 다시 마실 수 있다.

**수정 방향:** `UseConsumable`을 차감과 적용으로 나눈다. 권위에서 커밋 직후 고른 인스턴스의 충전을 1 줄이고, `bUsePending` 플래그 자리에 그 인스턴스를 약참조로 둔다. 노티파이에서는 그 인스턴스의 GE만 적용한다. 차감 뒤 충전이 0이면 `FindConsumableInstance`가 그 인스턴스를 건너뛰므로, 노티파이에서 다시 찾지 말고 기억한 인스턴스를 쓴다. 클라 예측 장치는 추가하지 않는다. 기획과의 차이가 의도라면 결정으로 기록한다.

**수정 후 확인:** 회복 전에 끊으면 충전이 1 줄고 회복은 없는지, 정상 사용 시 1회만 차감되는지 확인한다. 충전이 1개 남은 상태에서 끊긴 경우도 본다.

## R7. [P2 · 확신도 상] 분신이 가드를 따라 하면 가드 자세로 굳음

**발생 조건:** BP_HGTest가 궁극기 1로 분신(BP_Doppelganger)을 소환한 뒤 가드 키를 눌렀다 뗀다.

BT_Doppelganger의 MirrorAbility `ExcludedAbilities`에는 `Ability.Finisher`와 `Ability.Action.Ultimate`만 있다([WxBTTask_MirrorAbility.cpp:85](../../Source/WxGame/AI/WxBTTask_MirrorAbility.cpp#L85)). 그래서 마스터의 가드가 커밋되면 분신도 같은 가드를 발동한다. 가드가 끝나는 정상 경로는 입력 해제뿐인데, AI 분신에는 입력이 오지 않고 `IsInputHeld`도 항상 true다([WxAbility_Guard.cpp:110](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L110)). MirrorAbility는 마스터의 종료를 따라가지 않는다. 가드는 Recovery 단계가 없어 `Ability.Action` 차단을 계속 건다.

**영향:** 분신은 궁극기 2 등으로 거둘 때까지 가드 자세로 남는다. 그동안 공격·회피·스킬 따라 하기가 모두 거절되고, 위치 보정 텔레포트도 멈춘다. 이동 추종과 궁극기 2 발동은 막히지 않는다. 기획(현광)상 분신이 따라 하는 동작은 기본 공격·회피 반격·이동·대시이고 가드는 없다.

**수정 방향:** BT_Doppelganger의 `ExcludedAbilities`에 `Ability.Action.Guard`와 `Ability.GuardReact`를 추가한다(데이터 변경). GuardReact도 빼야 한다. 분신은 `IgnoreAbilityActivationTags`로 필수 태그를 건너뛰므로, 가드하지 않은 분신이 가드 피격 연출을 재생할 수 있다. 마스터 종료를 추적하는 동기화 장치는 만들지 않는다.

**수정 후 확인:** 분신 소환 뒤 가드를 눌렀다 떼도 분신이 공격·회피를 계속 따라 하고, 가드 피격 연출을 재생하지 않는지 확인한다.

## R8. [P2 · 확신도 상] 스포너 셀이 다시 로드되면 처치 퀘스트가 끝나지 않음

**발생 조건:** Standalone에서 LV_OpenWorld Main2의 처치 단계 중, 적을 남긴 채 스포너 셀에서 약 768 m(LoadingRange) 이상 떨어졌다가 돌아온다. 적이 KillZ 아래로 떨어진 경우도 같다(R15). 리슨 서버는 서버가 셀을 언로드하지 않아 해당하지 않는다. 체크포인트 부활 거리로는 셀이 내려가지 않는다.

셀이 다시 로드된 Manual 스포너는 `bIsKilled=false`인 새 인스턴스이고, BeginPlay에서 스폰하지 않는다([WxSpawner.cpp:111](../../Source/WxGame/Spawner/WxSpawner.cpp#L111)). 발동 태스크(TriggerSpawners)는 단계에 들어갈 때 한 번만 실행된다. [WaitSpawnersKilled](../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L136)는 해석된 스포너의 `IsKilled()`만 보므로 영원히 Running에 머문다.

처치 기록이 셀과 함께 사라지는 것 자체는 `b3f982b0d`(09-01, WxSave 삭제) 때 감수한 대가다([WxSpawner.h:67](../../Source/WxGame/Spawner/WxSpawner.h#L67) 주석). 하지만 그 결과 메인 퀘스트가 막히는 것은 별개 문제다.

**영향:** 퀘스트 목표가 "적 처치"에서 멈추고, 처치할 적이 다시 나오지 않는다. 지금은 R9 때문에 퀘스트 볼륨을 다시 밟으면 Main2가 처음부터 재시작되어 우연히 풀린다.

**수정 방향:** 판단 주체는 처치 대기 태스크로 둔다. 주기 판정에서 "해석된 스포너가 처치되지 않았고 추적 중인 스폰 대상도 없음"이면 `Respawn()`을 부른다. 스포너에는 스폰 대상 유효 여부를 묻는 조회 함수가 하나 필요하다. 재로드되면 처치 기록도 사라지므로, 이미 잡은 대상을 다시 스폰하지 않도록 태스크 인스턴스 데이터에 "처치를 확인한 스포너"를 남긴다. 스포너를 상시 로드로 바꾸는 우회는 쓰지 않는다. 스폰 대상이 퍼시스턴트 레벨에 생겨, 지형이 언로드되면 떨어질 수 있기 때문이다. 저장 범위를 정할 때 함께 다시 볼 수도 있다.

**수정 후 확인:** 처치 단계 중 768 m 밖으로 갔다 돌아와도 남은 적이 다시 나오고, 모두 처치하면 단계가 끝나는지 확인한다. 이미 잡은 적은 다시 나오지 않아야 한다.

## R9. [P2 · 확신도 중] 셀이 다시 로드되면 퀘스트 볼륨이 다시 발동해 퀘스트가 재시작됨

**발생 조건:** Standalone에서 LV_OpenWorld의 퀘스트 볼륨((732,-268) m)을 밟아 Main2를 받는다. 진행 중이든 완료했든 상관없다. 그 뒤 약 768 m 이상 떨어진 곳까지 갔다가 돌아와 볼륨에 다시 들어간다. 리슨 서버는 해당하지 않고, 체크포인트 부활 거리(약 570 m)로도 일어나지 않는다.

BP_QuestVolume은 겹침 시 `StartQuest`를 부른 뒤 `SetActorEnableCollision(false)`로 1회 발동을 구현한다. 이 상태는 액터의 런타임 값일 뿐이다. 배치된 볼륨은 공간 로드 기본값이라, 셀이 다시 로드되면 콜리전이 켜진 새 인스턴스가 된다. [ActivateQuest](../../Source/WxGame/Quest/WxQuestComponent.cpp#L34)는 같은 에셋이 실행 중인지, 완료했는지 확인하지 않고 `StopLogic`→`StartLogic`한다. 퀘스트 트리에는 장치 컴포넌트가 없어 `IsRestoring`이 false이므로 GiveRewards도 매번 지급한다.

`b3f982b0d`(WxSave 삭제)가 감수한 것은 "셀 재진입 시 처치·장치 상태 유지"다. 진행 중인 메인 퀘스트를 되돌리는 이 결함은 그 범위 밖이다. 확신도를 중으로 둔 이유는 BP 그래프에 다른 가드(DoOnce 등)가 없다는 것을 uasset 문자열로만 확인했기 때문이다.

**영향:** 진행 중인 메인 퀘스트가 첫 단계로 돌아가고 대화가 다시 재생된다. 완료한 퀘스트도 다시 진행할 수 있게 되어, 끝까지 하면 보상(DT_Reward)을 다시 받는다.

**수정 방향:** BP_QuestVolume 클래스 기본값에서 Is Spatially Loaded를 끈다(엔진 순정 설정). 세션 동안 언로드되지 않아 1회 발동 상태가 유지된다. `ActivateQuest`에 "같은 에셋이 Running이면 무시"만 넣으면 완료한 퀘스트의 재수주와 보상 재지급은 막지 못한다. 완료 기록의 영속화는 저장 범위를 정할 때 다룬다. **R8과 함께 고친다.**

**수정 후 확인:** 에디터에서 BP_QuestVolume 그래프에 다른 가드가 없는지 먼저 본다. 수정 뒤 Main2를 받거나 완료한 다음, 멀리 갔다 돌아와 볼륨에 들어가도 재시작·보상 재지급이 없는지 확인한다.

## R10. [P2 · 확신도 중] 원격 클라에서 선입력한 후딜 캔슬이 서버에서 거부됨

**발생 조건:** 리슨 서버에 접속한 원격 클라가 루트모션 공격 몽타주의 본동작 중에 회피·스킬·가드·아이템·궁극기를 미리 입력해 둔다. 그 입력이 StartRecovery 시점에 버퍼에서 재생될 때 생긴다. 리슨 호스트, Standalone, 같은 어빌리티의 콤보 재발동, StartRecovery 이후 직접 누른 입력은 해당하지 않는다.

소유 클라에서 루트모션 몽타주의 노티파이는 CMC `PerformMovement` 안에서 디스패치된다. StartRecovery가 그 콜스택에서 `FlushBufferedInputs`를 부르고([WxAbilityBase.cpp:223](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L223)), 곧바로 예측 발동이 일어난다. 엔진은 발동 RPC 전에 보류 중인 무브만 보내므로, StartRecovery를 넘긴 이번 프레임 무브는 발동 RPC보다 늦게 서버에 도착한다.

| 순서 | 원격 클라 | 서버 |
|---|---|---|
| 1 | 무브 수행 중 StartRecovery → 버퍼 재생 → 회피 예측 발동 RPC | |
| 2 | 회피 안에서 공격 취소 RPC | 발동 RPC 처리: 공격이 아직 Blocking이라 **회피 거부** |
| 3 | 이번 프레임 무브 전송 | 공격 취소 RPC 처리: 공격도 취소 |

회피·스킬 등은 공격을 `CancelAbilitiesWithTag`로 지목하지 않으므로, 서버에서는 차단에 걸린다([:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)).

**영향:** 클라에서 잠깐 나간 동작이 `ClientActivateAbilityFailed`로 되감기고, 공격 후딜도 이미 취소돼 캐릭터가 아무 동작 없이 선다. 입력은 성공으로 처리돼 버퍼에서 지워졌으므로 다시 나가지 않는다.

**수정 방향:** StartRecovery의 버퍼 재생만 `HandleAbilityEnded` 경로처럼 `SetTimerForNextTick`으로 미룬다([WxInputBufferComponent.cpp:144](../../Source/WxGame/Input/WxInputBufferComponent.cpp#L144)에 선례가 있다). 다음 틱 타이머는 CMC 틱 뒤에 돌아, 현재 무브가 먼저 전송된다. OpenComboWindow의 재생은 이 결함과 관계없다. 다만 R11을 서버 단계 기준으로 고친다면 함께 미뤄야 한다.

**수정 후 확인:** 네트워크 PIE(리슨 서버 + 클라 1)에서 원격 클라가 공격 본동작 중 회피를 선입력하면, StartRecovery에서 회피가 나가고 `ClientActivateAbilityFailed` 로그가 없는지 확인한다. 수정 전에 같은 절차로 먼저 재현해 확신도를 올린다.

## R11. [P2 · 확신도 중] 원격 클라의 회피 반격 2단이 항상 서버에서 거부됨

**발생 조건:** 원격 클라가 회피 반격 1단의 콤보 창(0.348~0.421초) 안에서, 또는 그 전에 미리 약공격을 입력해 2단을 발동한다.

클라는 `Occupant == this && ComboWindow`라 소유자 발동 조건(`Ability.Action.Dodge`)을 면제받고 2단을 예측 발동한다([WxAbilityBase.cpp:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)). 엔진의 재발동은 기존 인스턴스를 먼저 끝내는 `ServerEndAbility`를 보낸 뒤 발동 RPC를 보낸다. 두 RPC는 순서가 보장되는 reliable RPC라, 서버는 자기 인스턴스를 먼저 끝낸다. 서버가 발동을 판정할 때는 Occupant가 없어 면제가 성립하지 않는다. 회피는 1단 발동 때 취소돼 `Ability.Action.Dodge` 태그도 없으므로 거부된다([WxAbility_Attack.cpp:57](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Attack.cpp#L57)). 약공격 콤보는 요구 태그가 없어 같은 순서로도 통과하므로, 회피 반격만 끊긴다.

**영향:** 원격 플레이어의 2단이 시작했다가 바로 끊긴다(예측 비용·쿨다운 롤백). 서버에서만 실행되는 분신 소환(SpawnMinion)과 피해가 일어나지 않는다. 호스트와 원격 플레이어의 회피 반격이 다르게 동작한다.

**수정 방향:** 서버 판정이 서버 쪽 ActionPhase에 기대지 않게 한다. 새 태그나 전달 인자 없이 할 수 있는 최소안은 둘 중 하나다. 회피 반격의 "회피 중" 조건을 첫 단 진입에만 적용하거나, 같은 인스턴스가 직전에 취소가 아닌 원격 종료로 끝났다는 사실만으로 재발동 면제를 인정한다. 서버 인스턴스가 "ComboWindow 단계에서" 끝났는지로 판정하는 안은 쓸 수 없다. 선입력 재생이 R10과 같은 순서로 일어나면 서버 몽타주는 아직 창 전일 수 있다.

**수정 후 확인:** 네트워크 PIE에서 원격 클라의 2단이 나가고 서버에서 분신 소환·피해가 실행되는지, 호스트 동작은 그대로인지 확인한다. R3 수정과 함께 확인한다.

## P3 결함

P3는 드물거나 경미하거나, 현재 에셋에서는 드러나지 않는 결함이다. 표 안에서는 확신도 순서다.

### 지금 플레이에서 드러남

| ID | 확신도 | 문제 | 수정 방향 |
|---|---|---|---|
| R12 | 상 | 넉백 경직 중 약 1초 동안 이동 입력·AI 경로가 캐릭터를 움직인다. DisableRootMotion은 KnockBack 섹션 전체(0~1.316초)를 덮는데, 상수 힘은 0.3초만 민다([WxAbility_HitReact.cpp:136](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L136)). 지금은 플레이어 궁극기에 맞은 적에게서 드러난다. | 애니 루트모션을 끈 구간 동안 루트모션 소스를 유지한다(예: ConstantForce Duration을 섹션 길이로, 커브로 앞 0.3초만 밀기). DisableRootMotion만 줄이면 애니 변위가 이중으로 적용될 수 있어 에셋 확인이 필요하다. |
| R13 | 상 | 퍼펙트 가드의 반사 GP로 그로기에 든 공격자에게 패리 리액션도 보낸다. 그로기 시작 자세 대신 패리 넉이 먼저 나오고, 그로기 몽타주 꼬리가 잘린다([WxEffectComponent_PerfectGuard.cpp:46](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_PerfectGuard.cpp#L46)). | 패리 이벤트 조건에 AddGP 뒤 시점의 `!SourceASC->HasMatchingGameplayTag(Ability_Groggy)` 한 줄 추가 |
| R14 | 상 | 회피 후딜에서 다음 회피로 후딜 캔슬이 되지 않아, 연속 회피의 두 번째가 약 0.4초 늦다. Dodge가 `bRetriggerInstancedAbility`를 켜지 않았다([WxAbility_Dodge.cpp:21](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Dodge.cpp#L21)). 위키 규칙은 "StartRecovery 이후 모든 액션으로 캔슬 가능"이다. | 생성자에서 `bRetriggerInstancedAbility = true`(엔진 순정). 연속 회피 금지가 의도라면 주석으로 남긴다. |
| R15 | 상 | KillZ 아래로 떨어진 캐릭터가 사망 경로 없이 파괴된다. 플레이어는 사망 화면이 뜨지 않아 강제 종료해야 하고([WxRespawnLibrary.cpp:34](../../Source/WxGame/Player/WxRespawnLibrary.cpp#L34)), 적은 처치 통지가 없어 처치 퀘스트가 멈춘다([WxSpawner.cpp:191](../../Source/WxGame/Spawner/WxSpawner.cpp#L191)). 기본 KillZ(약 -10.5 km)까지 떨어져야 해서 드물다. | Lyra처럼 AWxCharacterBase에서 `FellOutOfWorld`를 재정의해, Destroy 대신 아직 `Ability.Death`가 없을 때만 정상 사망 경로를 태운다. 맵 KillZ를 지형에 맞게 올리는 것도 검토한다. |
| R16 | 상 | 리슨 서버에서 한 사람이 체크포인트에서 휴식하면 월드의 모든 Auto 스포너가 리젠된다. 다른 플레이어가 싸우던 적이 만피로 다시 나온다. 저장·부활은 Standalone 전용인데 리젠만 그대로 돈다([WxStateTreeTask_RespawnSpawners.cpp:31](../../Source/WxGame/Spawner/WxStateTreeTask_RespawnSpawners.cpp#L31)). | 호출부 `EnterState`에 SaveCheckpoint와 같은 Standalone 조건 한 줄. 멀티플레이 휴식 규칙은 기획 결정으로 받는다. |
| R17 | 상 | 체크포인트 휴식의 포션 리필이 휴식한 플레이어가 아니라 0번 PC(리슨 호스트)에게 간다. 같은 휴식의 HP 회복은 상호작용자에게 간다([WxStateTreeTask_RefillItemCharges.cpp:36](../../Source/WxGame/Inventory/WxStateTreeTask_RefillItemCharges.cpp#L36)). | Owner를 AWxDevice로 캐스트해 `GetInteractingCharacter()`의 인벤토리를 쓴다. 0번 전제 주석도 고친다. |
| R18 | 상 | MaxSP가 0인 아바타의 질주가 움직이는 즉시 끝난다(지금은 마스터 질주를 따라 하는 분신). 주석은 "제한 없이 달린다"인데, 엔진은 값이 그대로여도 변경 델리게이트를 보낸다([WxAbility_Sprint.cpp:130](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Sprint.cpp#L130)). | "제한 없이"가 의도라면 실제 감소 때만 종료하고, 분신의 `ExcludedAbilities`에 `Ability.Sprint`를 넣는다. 아니면 주석을 실제 동작에 맞춘다. |
| R19 | 상 | BP_HGTest의 회피 잔상이 숨긴 구동 메시(SKM_Quinn_Simple) 모습으로 나온다. 잔상이 MetaHuman 리더인 `GetMesh()`를 복사한다([WxCueNotify_GhostTrail.cpp:42](../../Source/WxGame/AbilitySystem/Cues/WxCueNotify_GhostTrail.cpp#L42)). | `UWxMetaHumanComponent`에 표시 바디 에셋 조회 함수를 두고, 있으면 그 에셋으로 바꾼 뒤 포즈를 복사한다. Face·그룸·Outfit은 여전히 빠진다. |
| R20 | 상 | WxTeleport 치트가 스트리밍 완료를 기다리지 않아, 미로딩 셀의 지면 근처로 가면 지형 아래로 떨어질 수 있다([WxCheatManager.cpp:24](../../Source/WxGame/Development/WxCheatManager.cpp#L24)). 개발 기능 한정이다. | 부활 경로처럼 `UpdateCamera` 뒤 `BlockTillLevelStreamingCompleted`로 기다린다. |
| R21 | 중 | 그로기 적을 처형하는 중 제3자(미니언·다른 플레이어)의 일반 피격이 들어오면, 그로기 폴링이 처형 짝 몽타주를 그로기 자세로 덮는다. 폴링이 "재생 중 몽타주 없음"을 `ASC->GetCurrentMontage()`로 판정한다([WxAbility_Groggy.cpp:115](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp#L115), :135). R1과 같은 계열이다. | 두 곳의 판정을 "그로기 몽타주와 같은 그룹에 활성 인스턴스가 있는가"로 바꾼다([WxCharacterMovementComponent.cpp:110](../../Source/WxGame/Character/WxCharacterMovementComponent.cpp#L110)처럼 순회). 폴링 구조는 유지한다. |
| R22 | 중 | 같은 몽타주를 연속 재발동하면 섹션 시작의 Queued 구간 노티파이가 엔진에서 병합돼 두 번째 재생에서 빠진다. 예: GuardReact SlowTime 구간 안에서 퍼펙트 가드가 다시 나면 두 번째 슬로모션이 없다([WxAnimNotify_AbilityEvent.h:24](../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.h#L24)). | 생성자에서 `NotifyStateBehaviorFlags`에 `NoMergeOnConcurrentPlay`를 켠다(엔진 순정 플래그). |
| R23 | 중 | 원격 클라에서 극한 회피 성공 신호가 클라 후딜 뒤에 도착하면, 클라에서만 성공 섹션이 Recovery로 시작한다. 클라는 다른 액션으로 캔슬할 수 있지만 서버는 막아 롤백이 생긴다([WxAbility_Dodge.cpp:202](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Dodge.cpp#L202)). | `SetActionPhase`를 protected로 옮기고, `HandleDodgeSuccess`에서 PlayMontage 전에 Blocking으로 설정한다. |
| R24 | 중 | 원격 클라가 RTT 안에 락온 대상을 연달아 바꾸면 서버의 지난 값 복제가 로컬 값을 덮어, 카메라가 이전 대상으로 잠깐 꺾인다. 그 사이 공격하면 스냅 워프가 클라·서버에서 달라질 수 있다([WxLockOnComponent.cpp:17](../../Source/WxGame/Targeting/WxLockOnComponent.cpp#L17)). | `DOREPLIFETIME_CONDITION(..., COND_SkipOwner)` 한 줄. 관련 주석(cpp 39행, 헤더 18~22·47~48행)도 고친다. |
| R25 | 중 | 클라 위치 보정으로 저장 무브를 재실행할 때 OnJumped가 다시 불려, 진행 중인 후딜 액션이 이유 없이 취소된다([WxCharacterBase.cpp:115](../../Source/WxGame/Character/WxCharacterBase.cpp#L115)). | `bClientUpdating`이면 후딜 취소를 건너뛴다(조건 한 줄). |
| R26 | 중 | 죽은 적의 AI 퍼셉션이 계속 돌아 시체마다 시야 쿼리 비용이 남는다. 시체의 15 m·120° 안에 플레이어 쪽 폰이 있을 때 트레이스가 생긴다. 비용 규모는 측정하지 않았다([WxAIController.cpp:179](../../Source/WxGame/AI/WxAIController.cpp#L179)). | `HandlePawnDeath`에서 퍼셉션 컴포넌트를 `UnregisterComponent()`한다. |
| R27 | 중 | 스포너 에디터 프리뷰의 자식 BP_Soldier가 레벨 패키지 2곳(Spawner_BP_Soldier9, Spawner_BP_Soldier11)에 저장돼 있다. 프리뷰를 `RF_Transient`만으로 만들어 T3D 복사로 살아남았다([WxSpawner.cpp:211](../../Source/WxGame/Spawner/WxSpawner.cpp#L211)). PIE에서 추적되지 않는 적이 하나 더 생기는지는 확인하지 못했다. | 생성 플래그를 `RF_Transient \| RF_TextExportTransient \| RF_DuplicateTransient`로 바꾸고, 두 패키지에서 프리뷰 객체를 걷어내 다시 저장한다. |

### 기획 확인이 필요한 차이

코드상 동작은 확인됐지만, 기획과 다른 것이 의도인지 기록이 없다. 기획 확인 뒤 고치거나 결정으로 남긴다.

| ID | 확신도 | 문제 | 수정 방향 |
|---|---|---|---|
| R28 | 상 | 강공격·스킬도 반응이 Normal이면 적 패턴을 끊지 못한다. HitReact가 피해 출처를 보지 않고 Pattern을 늘 취소 대상에서 뺀다([WxAbility_HitReact.cpp:22](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L22)). 어빌리티 규칙 브리핑은 "플레이어 평타만 패턴 캔슬 불가"다. | 원인 어빌리티가 `Ability.Action.Attack.Light`가 아니면 `Ability.Action.Pattern`을 취소한다. Pattern 주석(Q26)도 함께 고친다. |
| R29 | 상 | 그로기 중 받는 피해 +30% 규칙이 피해 계산에 없다. `bIsGroggy`를 GP 누적 판정에만 쓴다([WxEffect_Damage.cpp:124](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L124)). | `UWxCombatDeveloperSettings`에 배율(1.3)을 두고 `CalculateFinalDamage`에서 곱한다. 앞잡 피해가 86→112로 올라 TemplateEnemy가 한 번에 죽으므로, 앞잡 적용 여부를 확인한다. |
| R30 | 상 | 그로기를 일으킨 타격이 일반 피격 반응을 함께 재생한다. 그로기가 먼저 발동되고 DamageReaction이 진입 타격도 "그로기 중"으로 본다([WxEffectComponent_DamageReaction.cpp:59](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_DamageReaction.cpp#L59)). 기획은 반응 없음 또는 그로기 시작 리액션이다. | "이번 실행에 GP 기록 + `Ability.Groggy`"면 ReactionTag를 비운다. `Event.Hit`과 DamageDealt는 그대로 보낸다. |
| R31 | 상 | 가드 키를 누른 채 연속 공격을 막으면, GuardReact 뒤 가드를 다시 올릴 때마다 0.25초 퍼펙트 가드 구간이 입력 없이 다시 열린다. 몽타주를 0초부터 재생하기 때문이다([WxAbility_Guard.cpp:103](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L103)). 예전 방식도 0초부터 재생했다. | 의도가 아니면 다시 올릴 때 유지 구간 섹션부터 재생한다. 의도라면 주석으로 남긴다. |
| R32 | 상 | DA_Potion 최대 충전이 C++ 기본값 3회로 기획(기본·최대 4회)과 다르다([WxItemFragment.h:69](../../Source/WxGame/Inventory/WxItemFragment.h#L69)). 충전 아이콘도 3회 기준이다. | 4회가 맞으면 DA_Potion MaxCharges=4, ChargeIcons 5장. 3회가 의도라면 기획서 수정을 기획자에게 제안한다. |
| R33 | 상 | 순찰 속도 배율 기본값 0.5가 기획(기본 걷기의 0.75배)과 다르고, BT_Soldier·BT_Template 모두 덮어쓰지 않는다([WxBTTask_Patrol.h:39](../../Source/WxGame/AI/WxBTTask_Patrol.h#L39)). | 기본값 0.75 또는 두 BT에서 지정 |
| R34 | 상 | 실제 청각 거리가 3 m로 기획(1 m)과 다르다. 리스너 반경 1000 cm와 Jog 노티파이 300 cm 중 짧은 쪽이 적용된다([WxAIBehaviorComponent.h:71](../../Source/WxGame/AI/WxAIBehaviorComponent.h#L71)). | 적 BP에서 HearingRadius를 100으로 지정한다. C++ 기본값을 바꾸면 BP_Minion 청각도 1 m가 된다. |
| R35 | 중 | 처형 중에도 대상의 GP 드레인·누적이 계속돼, 그로기가 도중에 풀리거나 처형 피해로 다시 걸린다([WxAbility_Finisher.cpp:62](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L62)). | GP 누적 조건([WxEffect_Damage.cpp:204](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L204))에 "대상에 `State.FinisherReserved` 없음"을 건다. 드레인 정지는 그로기 종료 판정과 함께 설계가 필요하다. |

### 현재 에셋에서는 드러나지 않음

코드상 결함은 확실하지만, 지금의 에셋·배치에서는 조건이 성립하지 않는다. 해당 기능을 쓰기 전에 고치면 된다.

| ID | 확신도 | 문제 | 수정 방향 |
|---|---|---|---|
| R36 | 상 | 정찰 지점이 스폰 지점에서 30 m(LeashRadius) 밖이면 정찰과 복귀를 끝없이 반복한다. 리시 브랜치에 TargetActor 조건이 없다([WxBTDecorator_BeyondLeash.cpp:72](../../Source/WxGame/AI/WxBTDecorator_BeyondLeash.cpp#L72)). 지금 정찰 경로의 가장 먼 지점은 11.7 m다. | 최소안은 `UWxPatrolComponent` 주석에 "정찰 지점은 리시 반경 안"을 적는 것이다. BT로 막으려면 리시 브랜치에 TargetActor Is Set을 Observer aborts **Lower Priority**로 둔다(None이면 빈틈이 생긴다). |
| R37 | 상 | 청각만 엔진 기본 피아 판정을 써서 Neutral(255) 팀 소음을 적대로 듣는다. 시야·피해 감지는 폰 규칙(255면 Neutral)을 쓴다([WxAIController.cpp:31](../../Source/WxGame/AI/WxAIController.cpp#L31)). 지금은 Neutral 캐릭터가 없다. | 모듈 시작 시 `FGenericTeamId::SetAttitudeSolver`로 같은 규칙을 등록하고, `GetTeamAttitudeTowards`의 규칙 복제를 걷어낸다. 지금은 모듈 클래스가 없어 하나 필요하다. |
| R38 | 상 | 마지막 타격으로 죽은 대상에도 피해 행의 추가 효과 GE를 적용한다([WxEffectComponent_AdditionalEffects.cpp:26](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_AdditionalEffects.cpp#L26)). 지금은 추가 효과를 쓰는 행이 없다. | 루프 앞에 대상이 `Ability.Death`면 return |
| R39 | 상 | Rush 돌진 시간 계산이 몽타주 RateScale을 빠뜨려, RateScale이 1이 아니면 이동과 노티파이 구간이 어긋난다([WxAbilityTask_MontageEvents.cpp:341](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L341)). 지금 두 에셋은 1이다. | `GetPlayRate() * Montage->RateScale`로 나눈다. |
| R40 | 상 | 퀘스트 트리의 GiveRewards가 픽업 보상을 월드 원점 근처에 스폰한다. 퀘스트 트리의 Owner가 AWxGameState(AInfo)다([WxStateTreeTask_GiveRewards.cpp:37](../../Source/WxGame/Inventory/WxStateTreeTask_GiveRewards.cpp#L37)). 지금 DT_Reward에는 픽업 아이템이 없다. | 오너가 AWxDevice면 오너 위치를, 아니면 지급 대상 PC의 폰 위치를 쓴다. 루트 컴포넌트 유무로 판정하면 PIE와 쿠킹 빌드가 갈린다. |
| R41 | 상 | "목적지 이동" 퀘스트 목표가 볼륨 진입을 판정하지 못하고, 대상 원점과의 3D 거리만 본다([WxStateTreeTask_WaitMoveToTarget.cpp:49](../../Source/WxGame/Quest/WxStateTreeTask_WaitMoveToTarget.cpp#L49)). 기획서의 "지역 볼륨 진입" 단계를 저작할 방법이 없다. | 대상이 AVolume이면 `EncompassesPoint`로 판정한다. |
| R42 | 상 | 원격 클라에서는 어빌리티 부여·제거 뒤 HUD 슬롯이 즉시 다시 매칭되지 않는다. 엔진이 `AbilitySpecDirtiedCallbacks`를 권위에서만 보낸다([WxViewModel_AbilitySystem.cpp:16](../../Source/WxGame/UI/MVVM/WxViewModel_AbilitySystem.cpp#L16)). 지금은 빙의할 때 한 번만 부여한다. | 지금은 주석만 사실대로 고친다. 런타임 스킬 교체를 도입할 때 재정의안을 검토하되, 엔진 의미론 이탈 지점으로 명시한다. |
| R43 | 중 | RandomChoice가 뒤쪽 자식의 Lower Priority·Both 데코레이터를 관찰자로 남긴다. 조건이 바뀌면 엔진 ensure가 뜨고, 실행 중인 패턴이 가중치를 무시하고 바뀐다([WxBTComposite_RandomChoice.cpp:67](../../Source/WxGame/AI/WxBTComposite_RandomChoice.cpp#L67)). | `UBTComposite_Sequence`처럼 `CanAbortLowerPriority()`를 false로 재정의한다. 헤더 27~28행도 고친다. |
| R44 | 중 | HUD 슬롯 버튼으로 콤보를 이으면 원격 클라와 서버의 콤보 단계가 어긋날 수 있다. 버튼 발동은 단계를 싣지 않는다([WxAbility_Combo.cpp:46](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L46)). 지금은 Game 입력 모드라 버튼을 누를 일이 드물다. | VM 발동도 ASC의 단계 동반 경로를 쓰게 하고, [WxAbility_Skill.h:13](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Skill.h#L13) 주석을 고친다. |
| R45 | 중 | MarkIndicator의 TargetLocation 자동 기록은 태스크를 직접 편집할 때만 동작하고, 실제 사용처(퀘스트 스텝의 파라미터 바인딩)에서는 동작하지 않는다. 값을 비우면 원거리에서 인디케이터가 월드 원점을 가리킨다([WxStateTreeTask_MarkIndicator.h:38](../../Source/WxGame/UI/IndicatorSystem/WxStateTreeTask_MarkIndicator.h#L38)). | 주석을 실제 동작에 맞춘다. 자동 기록이 필요하면 `FWxActorLocatorCustomization`에서 형제 값을 채우는 쪽을 검토한다. |

## 도구 결함

게임 런타임에는 영향이 없다. 확신도는 모두 상이다(코드와 엔진 API 동작으로 결과가 정해진다).

| ID | 위치 | 문제 | 수정 방향 |
|---|---|---|---|
| T1 | [Check-Redirects.ps1:386](../../.agents/scripts/Check-Redirects.ps1#L386) | 패키지 헤더만 읽어, 에셋 안에 문자열 경로(MVVM 변환 함수 인자, StateTree 파라미터 메타)가 남아 있어도 SAFE로 보고한다. 10-01 모듈 합치기 때 사람이 따로 찾은 함정이다. | 옛 Package/Class 경로는 파일 전체에서도 찾아 REVIEW로 분류 |
| T2 | [Check-Redirects.ps1:431](../../.agents/scripts/Check-Redirects.ps1#L431) | GameplayTag 참조를 따옴표 붙은 문자열로만 찾아 ini의 `ActionTag=...`를 놓친다. | ini는 토큰 경계로도 검색 |
| T3 | [DataTableRowReferenceUpdater.cpp:82](../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.cpp#L82) | 해제된 행의 주소를 비교해, 주소가 재사용되면 행 교체를 이름 변경으로 오인할 수 있다(드묾). | 이름이 유지된 행도 포인터 동일성 확인 |
| T4 | [Export-AbilitySystemLists.ps1:419](../../.agents/scripts/Export-AbilitySystemLists.ps1#L419) | float을 `[decimal]`로 바꿔, 아주 큰 값이나 NaN이 하나라도 있으면 목록 3개 생성이 모두 중단된다. | 범위 안 값만 decimal, 나머지는 문자열 |
| T5 | [Invoke-WikiLint.ps1:37](../../.agents/skills/wiki-lint/scripts/Invoke-WikiLint.ps1#L37) | 확장자 없는 에셋 출처 경로는 git log와 맞지 않아 낡음을 항상 0건으로 센다. | 실제 파일 경로로 git log |
| T6 | [WxMVVMToolset.cpp:112](../../Plugins/WxToolset/Source/WxToolset/Private/WxMVVMToolset.cpp#L112) | 변환 함수 설정이 실패하면 기존 변환과 소스 경로를 지운 채 false를 반환한다. | `IsValidConversionFunction` 사전 검사 |
| T7 | [WxStateTreeToolset.cpp:178](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L178) | 중복 이름·잘못된 경로면 모달이 떠 MCP 호출이 멈추고, 덮어쓰기를 누르면 기존 StateTree가 바뀐다. | 경로·존재 확인 후 오류 반환 |
| T8 | [WxStateTreeToolset.cpp:521](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L521) | 링크 상태가 아닌 곳에 오버라이드 해제를 부르면 파라미터 전체를 지우고 true를 반환한다. | Linked 상태인지 확인 |
| T9 | [WxStateTreeToolset.cpp:255](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L255) | 빈 값 타입·None·정리되는 이름으로 쓸 수 없는 파라미터를 만들거나 빈 ID를 오류 없이 반환한다. | 해당 입력 거부 |
| T10 | [WxLandscapeToolset.cpp:124](../../Plugins/WxToolset/Source/WxToolset/Private/WxLandscapeToolset.cpp#L124) | 새 LayerInfo의 BlendMethod가 None이라 페인트 레이어가 정규화되지 않는다. | `SetBlendMethod(FinalWeightBlending)` |
| T11 | [WxAnimMontageToolset.cpp:658](../../Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.cpp#L658) | 추상 노티파이 클래스를 받아 저장 시 사라질 노티파이를 만들고 true를 반환한다. | `CLASS_Abstract`면 오류 |
| T12 | [WxEditor.cpp:83](../../Source/WxEditor/WxEditor.cpp#L83) | Blueprint 썸네일 렌더러를 교체하면서 AnimBlueprint·WidgetBlueprint 전용 썸네일이 가려진다. | 교체 직후 두 항목을 다시 등록해 순서 복원 |

## 품질 개선

결함은 아니지만 근거가 확인된 정리 항목이다. 대부분 몇 줄 수정이다. ※는 검증자가 전제 일부를 직접 확인하지 못한 항목이다.

### 사용자 코드 규칙

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q1 | 아래 목록 | `.cpp`의 익명 namespace 12곳과 static 자유 함수 6개 | 상수는 지역 상수나 클래스 static 멤버로, 헬퍼는 호출부 인라인 또는 private 멤버로 |
| Q2 | [WxIndicator.cpp:110](../../Source/WxGame/UI/IndicatorSystem/WxIndicator.cpp#L110) | UIManager 서브시스템을 직접 꺼내 쓴다(UWxUILibrary 파사드 규칙) | UWxUILibrary에 `IsMenuLayerActive` 추가 |
| Q3 | [WxGameFlowSubsystem.cpp:41](../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L41) 외 | 프론트엔드 상태 문구 6개가 한국어다(인게임 문구는 영어) | 기본 문구를 영어로 |

Q1 위치는 다음과 같다.
- WxGame 익명 namespace: [WxAbility_Finisher.cpp:16](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L16), [WxHitStopComponent.cpp:11](../../Source/WxGame/AbilitySystem/WxHitStopComponent.cpp#L11), [WxAnimNotifyState_SnapToTarget.cpp:13](../../Source/WxGame/Animation/WxAnimNotifyState_SnapToTarget.cpp#L13), [WxAnimNotify_AbilityEvent.cpp:9](../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.cpp#L9), [WxSkillCutsceneComponent.cpp:26](../../Source/WxGame/Combat/WxSkillCutsceneComponent.cpp#L26), [WxDeviceStateTreeComponent.cpp:12](../../Source/WxGame/Device/WxDeviceStateTreeComponent.cpp#L12), [WxStateTreeTask_WaitForInteraction.cpp:11](../../Source/WxGame/Interaction/WxStateTreeTask_WaitForInteraction.cpp#L11), [WxStateTreeTask_WaitSpawnersKilled.cpp:11](../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L11)
- WxToolset 익명 namespace: [WxAnimMontageToolset.cpp:14](../../Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.cpp#L14), [WxBlueprintToolset.cpp:14](../../Plugins/WxToolset/Source/WxToolset/Private/WxBlueprintToolset.cpp#L14), [WxLandscapeToolset.cpp:21](../../Plugins/WxToolset/Source/WxToolset/Private/WxLandscapeToolset.cpp#L21), [WxStateTreeToolset.cpp:26](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L26)
- static 자유 함수: [WxEffect_Damage.cpp:60](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L60)부터 `GetDamageStatics`와 `Calculate*` 5개. `GetDamageStatics`는 GAS ExecCalc의 관용 패턴(Lyra와 같음)이라 예외로 둘지 결정이 필요하다.
- WaitForInteraction의 namespace에는 전역 대기 등록부와 핸들 카운터가 있어, 정적 상태의 소유자가 코드에 드러나지 않는다. 태스크 클래스의 private static 멤버로 옮긴다.

### 죽은 코드·무효 코드

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q4 | [WxEffect_Exceed.h:9](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Exceed.h#L9) | Exceed GE·Cue에 발행 경로가 없다(파생 에셋·호출부 없음) | 재사용 계획이 없으면 GE·Cue·태그를 삭제 목록으로 |
| Q5 | [WxViewModel_Ability.cpp:28](../../Source/WxGame/UI/MVVM/WxViewModel_Ability.cpp#L28) | ActionPhaseChanged 이벤트가 어떤 표시 값에도 영향이 없다. 어빌리티가 UI만을 위해 단계마다 이벤트를 낸다 | 구독·발행·태그 함께 제거 |
| Q6 | [WxInventoryComponent.h:177](../../Source/WxGame/Inventory/WxInventoryComponent.h#L177) | "UI 진입점"이라는 `RequestUseConsumable`과 `RemoveItemInstance`·`RemoveEntry`에 호출자가 없다 | 제거, 주석 정정 |
| Q7 | [WxAsyncAction_PushWidgetToLayer.h:35](../../Source/WxGame/UI/Foundation/WxAsyncAction_PushWidgetToLayer.h#L35) | `SetBeforePushCallback` 경로에 호출자가 없다 | 삭제(BP용 델리게이트는 유지) |
| Q8 | [WxEffect_HitStop.cpp:31](../../Source/WxGame/AbilitySystem/Effects/WxEffect_HitStop.cpp#L31) | 컨텍스트에 어빌리티를 넣지만 읽는 곳이 없다(예측 키 제거 잔재) | `MakeEffectContext()`만 남김 |
| Q9 | [WxAbilityTask_SlowTime.h:12](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_SlowTime.h#L12) | Duration 경로를 아무도 쓰지 않고, 주석이 현재 사용 방식과 다르다 | 분기 제거, 주석 정정 |
| Q10 | [WxStateTreeTask_SpawnNiagara.cpp:25](../../Source/WxGame/Device/WxStateTreeTask_SpawnNiagara.cpp#L25) | "살아 있으면 통과" 분기에 도달할 수 없어, 재선택 때 루프 FX를 지웠다가 다시 만든다(메모리의 재진입 설계와 다름) | ExitState에서 Sustained면 return |
| Q11 | [WxEnemyCharacter.cpp:40](../../Source/WxGame/Character/WxEnemyCharacter.cpp#L40) | `SetReplicationMode(Full)`은 엔진 기본값을 다시 쓸 뿐이다 ※BP에서 바꾸는지는 미확인 | 줄 삭제 |

### 같은 규칙의 중복

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q12 | [WxAbilitySystemComponent.cpp:301](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L301) | ASPD 하한 0.001이 AttributeSet 클램프와 두 곳에 있다 ※클램프 경로 재확인 안 함 | 이쪽 Max 제거 |
| Q13 | [WxAbilitySet.cpp:61](../../Source/WxGame/AbilitySystem/WxAbilitySet.cpp#L61) | 세트 부여 경로의 권위 검사가 한 호출 사슬에서 네 번 반복된다 | `GiveAbilitySets` 한 곳만 |
| Q14 | [WxAbility_Groggy.cpp:26](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp#L26) | Death 차단이 Death 쪽 `BlockAbilitiesWithTag`와 중복된다 | 삭제(거는 쪽이 막는 규약) |
| Q15 | [WxAnimNotifyState_Rush.h:19](../../Source/WxGame/Animation/WxAnimNotifyState_Rush.h#L19) | AbilityEvent 신호 규칙을 복제했다. 베이스를 고치면(R22 플래그 등) Rush만 빠진다 | AbilityEvent 파생으로(WeaponAttack 선례) |
| Q16 | [WxAbility_Interact.h:34](../../Source/WxGame/Interaction/WxAbility_Interact.h#L34) | 상호작용 반경이 스캐너·어빌리티 두 곳에 있다. 한쪽만 바꾸면 프롬프트는 뜨는데 서버가 거절한다 | 어빌리티가 서버 PC 스캐너 반경을 읽음 |
| Q17 | [WxViewModel_Subtitle.cpp:14](../../Source/WxGame/UI/MVVM/WxViewModel_Subtitle.cpp#L14) | `GetGlobalCollection`과 같은 조회를 다시 구현했다 | 유틸 호출로 교체 |

### 구조·계약

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q18 | [WxCharacterBase.cpp:166](../../Source/WxGame/Character/WxCharacterBase.cpp#L166) | `IsAlive()`가 HP만 본다. HP를 둔 채 죽이는 경로가 쓰이면 시체에 명판이 남는다 | `HP > 0 && !Ability.Death` |
| Q19 | [WxDamageEffectContext.cpp:34](../../Source/WxGame/AbilitySystem/WxDamageEffectContext.cpp#L34) | const 핸들을 const_cast해 컨텍스트를 수정한다 | const/비const 분리 |
| Q20 | [WxAbilityBase.h:91](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.h#L91) | Infinite 계약인 `ActivationOwnedEffects`에 Instant GE 2개가 있어, 실패한 발동에도 UP 지급이 남을 수 있다 ※에셋 구성은 보고 기준 | 계약 명시 또는 비용 커밋 경로로 |
| Q21 | [WxBTDecorator_ObserveAbility.cpp:35](../../Source/WxGame/AI/WxBTDecorator_ObserveAbility.cpp#L35) | "인스턴스에는 키 ID가 없다"는 주석이 엔진과 달라 매번 이름으로 조회한다 | `GetSelectedKeyID()` 사용 |
| Q22 | [WxBTDecorator_BeyondLeash.cpp:47](../../Source/WxGame/AI/WxBTDecorator_BeyondLeash.cpp#L47) | 설명에서 Super가 빠져 BT 그래프에 abort 모드·inversed가 안 보인다(AttributeRatio도 같음) | Super 결과를 앞에 붙임 |

### 코드와 어긋난 주석

| ID | 위치 | 실제 동작 |
|---|---|---|
| Q23 | [WxAbilityTargetData_Direction.h:9](../../Source/WxGame/AbilitySystem/WxAbilityTargetData_Direction.h#L9) | 서버→소유 클라 피격 방향 전달에도 쓴다 |
| Q24 | [WxCombatAttributeSet.h:107](../../Source/WxGame/AbilitySystem/Attributes/WxCombatAttributeSet.h#L107) | ASPD는 콤보 계열만 쓴다 |
| Q25 | [WxAbilitySystemComponent.cpp:199](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L199) | 엔진이 이미 사본을 만들어 "사본 회피" 근거가 성립하지 않는다 |
| Q26 | [WxAbility_Pattern.cpp:15](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Pattern.cpp#L15) | 패턴은 넉 계열 피격에도 끊긴다 |
| Q27 | [WxCombatLibrary.h:36](../../Source/WxGame/Combat/WxCombatLibrary.h#L36) | 소유 클라 예측 적용은 일어나지 않는다 |
| Q28 | [WxAIController.cpp:80](../../Source/WxGame/AI/WxAIController.cpp#L80) | Damage 센스에는 피아 필터가 없다 |
| Q29 | [WxEffect_HitStop.h:12](../../Source/WxGame/AbilitySystem/Effects/WxEffect_HitStop.h#L12) | "역경직"이 히트스톱과 패리 리액션을 함께 가리킨다(`bCanParry` 툴팁) |
| Q30 | [WxProjectileBase.h:56](../../Source/WxGame/Weapons/WxProjectileBase.h#L56) | 인터페이스 구획 안에 AActor 재정의가 있다 |
| Q31 | [WxMinionComponent.h:41](../../Source/WxGame/Minion/WxMinionComponent.h#L41) | 사망 어빌리티가 없으면 즉시 파괴한다(현재 유일한 사용처) |
| Q32 | [WxCheatManager.h:14](../../Source/WxGame/Development/WxCheatManager.h#L14) | EnableCheats로 클라에도 생긴다 ※엔진 코드는 보고 기준 |
| Q33 | [WxDialogueSessionComponent.cpp:361](../../Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp#L361) | 단일 노드 모드에도 AnimInstance가 있다 |
| Q34 | [WxItemFragment.h:31](../../Source/WxGame/Inventory/WxItemFragment.h#L31) | Grade·Pickup은 기능이 아닌 표시·외형 데이터다 |
| Q35 | [WxTargetingSorterTask_InputDirection.h:13](../../Source/WxGame/Targeting/WxTargetingSorterTask_InputDirection.h#L13) | 판정 일치는 서버와 소유 클라 사이에서만 맞다 |

### 데이터·설정 정리

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q36 | LV_OpenWorld·LV_DevCombat의 Spawner_BP_Boss | BP_Boss 삭제(`e0e3ecc51`) 뒤 대상 클래스가 빈 스포너 2개가 남았다. `Character.Boss` 캐릭터도 없어 보스 체력 바 경로가 쓰이지 않는다 | 보스 BP를 만들 때 연결, 그 전에는 레벨에서 삭제 |
| Q37 | [ST_Quest_Main1](../../Content/Quest/ST_Quest_Main1.uasset)·Main2 | 실패 전이가 등록되지 않은 태그 `Quest.Fail`을 기다려 발동할 수 없고, 로드마다 경고가 난다 | 실패 조건이 없다면 전이 제거 |
| Q38 | SiegeCannonEmplacement01 | 장치 6개가 폐기된 ST 파라미터 오버라이드를 들고 있다 ※바이너리 정밀 파싱 안 함 | 레벨 인스턴스를 열어 동기화 후 재저장 |
| Q39 | BP_Soldier_Projectile 외 2개 | 참조되지 않는 에셋 3개가 없는 피해 행을 가리킨다 ※DT 행 부재는 보고 기준 | 삭제 목록으로(사용자 확인 후) |
| Q40 | [DefaultEngine.ini:44](../../Config/DefaultEngine.ini#L44) | `WxCharacterMesh` 프로파일을 아무도 쓰지 않고, `WxAttack=Block`이 헤더 규칙(Overlap)과 반대다 | 프로파일 줄 삭제 |

## 기각·보류된 후보

검증 단계에서 결함이 아니라고 판정된 후보다. 같은 지적이 반복되지 않도록 사유를 남긴다.

| ID | 후보 | 기각 사유 |
|---|---|---|
| X1 | 청각 자극만으로 즉시 TargetActor를 지정한다 | `95a5a24a3`(07-17)에서 청각을 완전 획득으로 승격한 의도된 결정이다 |
| X2 | 리슨 서버에서는 사망 후 부활 경로가 없다 | 새 게임·체크포인트·부활 흐름 전체가 싱글플레이 범위로 설계됐다 |
| X3 | 보물상자·체크포인트 장치 상태가 셀 재로드 때 초기화된다 | `b3f982b0d`(WxSave 삭제)에서 감수하고 세이브 재도입 때 다시 설계하기로 했다. 1차 확정 뒤 2·3차가 기각 |
| X4 | 처치된 `bNeverRevive` 스포너가 셀 재로드 뒤 다시 스폰한다 | X3과 같은 결정이다. 해당 배치는 대상 클래스가 빈 스포너뿐이다. 1차 확정 뒤 2·3차가 기각 |
| X5 | LV_OpenWorld 보스 스포너의 `bNeverRevive`가 꺼져 있다 | 스폰 대상 클래스 자체가 없다(Q36) |
| X6 | 퀘스트 "NPC 대화" 단계가 상호작용 순간에 완료된다 | `e771a418b`(08-14) 대화 완주 게이트를 상호작용 대기로 바꾼 결정이다 |
| X7 | 퀘스트·대화가 호스트 전용이라 원격 클라는 퀘스트 NPC를 쓸 수 없다 | "v1 싱글/리슨 호스트 전제"가 코드에 명시된 범위 제한이다 |
| X8 | NPC 대사 포즈가 대화한 머신에서만 재생된다 | X7 전제 안의 연출 차이다. 복제 장치 신설은 원칙에 맞지 않는다 |
| X9 | 보물상자 골드가 0번 PC에게 들어간다 | 적 처치 보상과 같은 문서화된 정책이다 |
| X10 | `LinkedDevices` 빈 슬롯이 클라에서 열려 보인다 | 사용자가 확정한 "미로드 장치는 열어 두고 서버가 재검증" 정책 범위다 |
| X11 | TargetingPreset이 빈 이동 스냅은 범위 없이 워프한다 | 오래 유지된 명시적 의미이고, 몽타주 51개가 모두 프리셋을 지정한다 |
| X12 | Character VM이 자식 VM 구독을 해제하지 않는다 | 09-29 VM 베이스 제거 때 파괴 정리를 없앤 결정이다. 화면 영향이 없다 |
| X13 | Dodge 판정 캡슐을 고정 이름으로 만들어 덮어쓴다 | 엔진이 교체 전에 컴포넌트 등록을 정리해 누적되지 않는다 |
| X14 | 예측 클라가 상대의 Rush 태스크를 찾지 못한다 | 주장된 목적지 차이가 일어나지 않는다 |
| X15 | 디스크 체크포인트가 New Game에서만 지워진다 | 실행 간 유지가 의도다 |
| X16 | 콤보 입력이 `InternalTryActivateAbility`를 직접 부른다 | 이유가 주석에 있고, 가상의 정책 변경 대비 게이트는 엔진 판정 중복 금지 원칙과 충돌한다 |

**판정 보류(실행 없이 판단 불가)**
- [BoxComponentVisualizer.cpp:142](../../Plugins/BoxComponentVisualizer/Source/BoxComponentVisualizerEditor/Private/BoxComponentVisualizer.cpp#L142): BP 에디터에서 SCS 루트 UBoxComponent를 끌면 "반대쪽 면 고정"이 배치 인스턴스에 남지 않는다는 후보. 해당 구성의 BP가 프로젝트에 없다.
- [WxIndicator.cpp:109](../../Source/WxGame/UI/IndicatorSystem/WxIndicator.cpp#L109): "메뉴가 스크린 스페이스 위젯을 덮어 주지 못한다"는 주석이 엔진 레이어 순서와 다르다는 후보. 엔진 레이어 순서를 확인하지 못했다.

## 빌드 결과

Editor Development 증분 빌드는 경고 0건으로 성공했다(변경분 12개 파일 컴파일). 전체 클린 빌드는 하지 않았다.

## 부록: 검토 단위

| 단위 | 내용 | 파일 | 줄 |
|---|---|---:|---:|
| U01 | AbilitySystem 코어·Attributes·Cues | 25 | 1,978 |
| U02 | 어빌리티 A(베이스·공격·콤보·패턴·스킬·궁극기·스프린트·락온) | 20 | 2,108 |
| U03 | 어빌리티 B(사망·회피·처형·그로기·가드·피격반응) | 14 | 1,632 |
| U04 | GameplayEffect·GE 컴포넌트·ExecCalc | 58 | 1,994 |
| U05 | AbilityTask·Combat | 26 | 2,263 |
| U06 | AI 컨트롤러·컴포넌트·BT Task | 18 | 1,806 |
| U07 | BT Composite·Decorator·Service | 18 | 1,383 |
| U08 | Animation·Weapons | 42 | 2,193 |
| U09 | Character·Minion | 15 | 1,921 |
| U10 | 모듈 루트·Player·GameModes·Save·System·Development·FrontEnd·Input | 30 | 1,750 |
| U11 | Device | 32 | 2,273 |
| U12 | Dialogue·Quest·Interaction | 28 | 2,423 |
| U13 | Inventory | 22 | 2,186 |
| U14 | Spawner·Targeting | 33 | 1,881 |
| U15 | UI 코어·Foundation·Frontend·UIManager·IndicatorSystem | 31 | 2,746 |
| U16 | UI MVVM·Subtitle | 48 | 2,846 |
| U17 | WxEditor·타겟 정의·BoxComponentVisualizer | 31 | 2,061 |
| U18 | WxToolset | 16 | 2,419 |
| U19 | DataTableRowFixup·uproject·Config·BatchFiles·.agents 스크립트 | 31 | 3,696 |
| **합계** | C++·C# 512, 설정·스크립트 26 | **538** | **41,559** |
