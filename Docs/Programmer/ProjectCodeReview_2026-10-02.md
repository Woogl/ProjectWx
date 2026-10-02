# WX 프로젝트 전체 코드 리뷰 — 2026-10-02

기준 커밋 `af83fcefc`. 작업 트리의 소스·설정은 이 커밋과 같다.

## 결과 요약

리뷰 대상 **538개 파일(41,559줄)을 모두 읽었다**. 결함 후보 117건은 독립 검증자가 반증을 시도했고, 99건이 확정됐다. P0(크래시·세이브 손상·일반 진행 불가)에 해당하는 결함은 없었다.

| 구분 | 건수 | 의미 |
|---|---:|---|
| P1 | 1 | 주요 전투 흐름이 깨진다 |
| P2 | 10 | 구체적인 조건에서 기능 오류가 난다 |
| P3 | 34 | 드문 조건의 오류, 잠재 결함, 기획 수치와의 차이 |
| 도구 결함 | 12 | 에디터 확장·MCP 도구·운영 스크립트(게임 런타임 영향 없음) |
| 품질 개선 | 40 | 사용자 코드 규칙 위반, 죽은 코드, 규칙 중복, 코드와 어긋난 주석, 데이터 정리 |
| 기각·보류 | 16 + 2 | 의도된 설계로 확인됐거나 엔진 동작상 문제가 아님 / 실행 없이 판정 불가 |

AGENTS.md 코딩 규칙 1~3(Wx 접두사, 저작권 첫 줄, 인라인 함수 정의 금지)은 전수 검사에서 모두 통과했다. 사용자 규칙인 익명 namespace·static 자유 함수 금지는 13개 파일에서 어기고 있다([Q01](#q-품질-개선)).

### 먼저 볼 것

1. **[D01](#d01-p1-일반-피격을-받으면-진행-중인-다른-어빌리티의-몽타주-노티파이-구간이-사라진다)·[D02](#d02-p2-퍼펙트-가드-직후-guardreact-동안-퍼펙트-가드-판정이-계속-유지된다)**: 몽타주 신호 소유 판정의 조건 하나가 원인이다. 평타에 맞은 적의 패턴 공격 판정이 열리지 않거나 닫히지 않고, 퍼펙트 가드 구간도 늘어난다. 10-01 몽타주 이벤트 통합(`6a2dec191`)에서 생긴 회귀이며, 조건 하나를 지우면 둘 다 해결된다.
2. **[D03](#d03-p2-원격-클라이언트에서-선입력한-후딜-캔슬이-서버에서-거부된다)·[D04](#d04-p2-원격-클라이언트의-회피-반격-2단이-항상-서버에서-거부된다)**: 원격 클라이언트에서만 생긴다. 선입력한 후딜 캔슬(회피·스킬·가드·아이템)과 회피 반격 2단이 서버에서 거부된다.
3. **[D05](#d05-p2-회피-반격-1단의-후딜이-시작되면-2단-대신-약공격-1단이-나간다)·[D06](#d06-p2-hgtest-회피-반격-2단이-피해를-주지-않는다)**: 회피 반격 2단은 1단 후딜이 시작되면 약공격으로 바뀐다. HGTest는 2단의 피해 행이 없어 피해도 주지 않는다. D06은 에셋 값만 고치면 된다.
4. **[D07](#d07-p2-일반-피격으로는-아이템-사용이-끊기지-않는다)·[D08](#d08-p2-회복-전에-끊긴-포션이-소비되지-않는다)**: 포션 규칙이 기획과 반대로 동작한다. 함께 고쳐야 한다. D07만 고치면 "피격되면 포션이 줄지 않는" D08이 더 자주 드러난다.
5. **[D09](#d09-p2-분신이-가드를-따라-하면-가드-자세로-굳어-이후-동작을-따라-하지-않는다)**: 분신이 플레이어의 가드를 따라 하면 가드 자세로 굳는다. BT 에셋 값만 고치면 된다.
6. **[D10](#d10-p2-셀이-다시-로드되면-퀘스트-볼륨이-다시-발동해-퀘스트가-처음부터-재시작된다)·[D11](#d11-p2-처치-퀘스트가-스포너-셀이-다시-로드된-뒤-끝나지-않는다)**: Standalone에서 768 m 이상 멀어졌다가 돌아오면 퀘스트가 처음부터 다시 시작되거나 처치 단계가 멈춘다. 두 수정이 서로 영향을 주므로 함께 다룬다.

### 확인하지 않은 것

- **정적 리뷰 결과다.** PIE·멀티플레이 실행으로 재현하지는 않았다. 네트워크 결함(D03·D04 등)은 엔진 소스의 실행 순서를 따라가 판정했다.
- 성능 프로파일링, 패키징, 전체 클린 빌드는 하지 않았다. Editor Development 증분 빌드는 경고 0건으로 성공했다(변경분 12개 파일 컴파일).
- 블루프린트 그래프·StateTree·레벨 배치를 전수 조사하지는 않았다. 판정에 필요한 에셋만 uasset 문자열·바이너리를 파싱해 확인했다.
- 완결성 점검 2차 라운드(놓친 흐름을 찾아 추가 리뷰)는 사용량 한도 때문에 생략했다. 기획서 대비 점검을 따로 돌리지도 않았고, 단위 리뷰어가 담당 범위에서 함께 봤다.

## 검토 범위와 방법

### 범위

| 단위 | 내용 | 파일 | 줄 |
|---|---|---:|---:|
| U01 | AbilitySystem 코어(ASC·AbilitySet·글로벌·TargetData·EffectContext·HitStop)·Attributes·Cues | 25 | 1,978 |
| U02 | 어빌리티 A: WxAbilityBase·공격·콤보·패시브·패턴·스킬·궁극기·스프린트·락온 | 20 | 2,108 |
| U03 | 어빌리티 B: 사망·회피·처형·그로기·가드·가드반응·피격반응 | 14 | 1,632 |
| U04 | GameplayEffect·GE 컴포넌트·ExecCalc | 58 | 1,994 |
| U05 | AbilityTask·Combat | 26 | 2,263 |
| U06 | AI A: AIController·AIBehaviorComponent·PatrolComponent·BT Task | 18 | 1,806 |
| U07 | AI B: BT Composite·Decorator·Service | 18 | 1,383 |
| U08 | Animation·Weapons | 42 | 2,193 |
| U09 | Character·Minion | 15 | 1,921 |
| U10 | 모듈 루트·Player·GameModes·Save·System·Development·FrontEnd·Input | 30 | 1,750 |
| U11 | Device | 32 | 2,273 |
| U12 | Dialogue·Quest·Interaction | 28 | 2,423 |
| U13 | Inventory | 22 | 2,186 |
| U14 | Spawner·Targeting | 33 | 1,881 |
| U15 | UI 코어·Foundation·Frontend·UIManager·IndicatorSystem | 31 | 2,746 |
| U16 | UI MVVM·Subtitle | 48 | 2,846 |
| U17 | WxEditor·타겟 정의·BoxComponentVisualizer 플러그인 | 31 | 2,061 |
| U18 | WxToolset 플러그인 | 16 | 2,419 |
| U19 | DataTableRowFixup 플러그인·Wx.uproject·Config·BatchFiles·.agents 스크립트 | 31 | 3,696 |
| **합계** | C++·C# 512, 설정·스크립트 26 | **538** | **41,559** |

엔진 소스, 외부 플러그인, 자동 생성 코드, 원본 기획서는 대상이 아니다. 엔진 코드는 판정 근거가 필요한 부분만 대조했다.

### 방법

1. **파일 단위 리뷰**: 단위마다 리뷰어 1명이 담당 파일을 모두 읽고, 호출 관계를 범위 밖까지 추적했다. 538개 파일 모두 "끝까지 읽음"으로 보고됐다.
2. **횡단 관점 리뷰**: 5개 관점(네트워크·권위, 수명·델리게이트·타이머, GAS 종단 흐름, 저장·부활·월드 상태 복원, 태그·설정 의존과 프레임 비용)으로 모듈을 가로질러 흐름을 추적했다.
3. **코딩 규칙**: AGENTS.md 규칙 1~3과 익명 namespace·static 자유 함수는 스크립트로 전수 검사했다.
4. **검증**: 후보 128건 중 중복 4건을 병합하고, 규칙 위반 7건은 전수 검사 결과 하나로 묶었다. 남은 117건은 검증자가 코드를 직접 읽고 반증을 시도했다. 결함 후보는 Opus, 품질·도구 후보는 Sonnet이 맡았다. P1·P2로 보고된 20건은 앞선 판정을 모르는 2차 검증자가 다시 확인했다. 의견이 갈린 2건은 3차 판정자가 결론을 냈고, 둘 다 기각됐다.
5. **판단 근거**: 사용자 메모리에 기록된 설계 결정, Wiki, Docs 기획서, git 이력, 어빌리티·이펙트·캐릭터 목록(Saved/AbilitySystemLists, 리뷰 시작 시 Export-AbilitySystemLists.ps1로 재생성), 에셋 바이너리 파싱, UE 5.8 엔진 소스를 썼다. 메모리에 의도·결정으로 기록된 동작은 결함으로 세지 않았다.

문서의 ID(D·T·Q·X)는 이 문서에서만 쓰는 번호다. 아래 위치 링크는 리뷰 시점 줄 번호다.

## P1·P2 결함

### D01 [P1] 일반 피격을 받으면 진행 중인 다른 어빌리티의 몽타주 노티파이 구간이 사라진다

**발생 조건**: 적이 패턴 단계 몽타주(예: AM_Soldier_Pattern_1_2)를 재생하는 중에 플레이어 평타에 맞는다. 플레이어 평타 행은 모두 `HitReact.Normal`이라 일반 전투에서 자주 일어난다. 회피·아이템 사용 중의 일반 피격도 같다.

**영향**
- 그 단계의 WeaponAttack 판정이 아직 열리지 않았으면 끝내 열리지 않는다. 적 공격이 피해를 주지 않는다.
- 이미 열려 있었으면 노티파이 구간이 끝나도 닫히지 않는다. 몽타주가 블렌드아웃될 때까지 남아 후딜 중인 무기에도 맞는다.
- StartRecovery·ComboWindow 신호도 버려져 패턴·회피·아이템의 후딜 캔슬이 사라진다. 아이템 회복은 `WxAnimNotify_UseItem` 경로라 영향이 없다.

**원인**: `OwnsMontageSignal`은 몽타주 인스턴스 ID가 일치해도 `ASC->GetAnimatingAbility() == Ability`를 함께 요구한다([WxAbilityTask_MontageEvents.cpp:83](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L83)). 일반 피격 몽타주 AM_Shared_HitReact는 AdditiveHitReact 슬롯이라 DefaultSlot 몽타주를 끊지 않는다. 하지만 `ASC::PlayMontage`가 슬롯과 상관없이 AnimatingAbility를 HitReact로 덮어쓴다. HitReact가 끝나면 엔진은 이 값을 nullptr로 비울 뿐 원래 어빌리티로 되돌리지 않는다. 그동안 원래 어빌리티의 구간 시작·종료 신호([:285](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L285))가 모두 버려진다. 열린 구간은 태스크가 끝날 때 `ClearMontageWindows`에서야 회수된다. [UWxAbilityBase::IsPlayingMontageInstance](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L228)도 `ASC->GetCurrentMontage()`를 기준으로 삼아 같은 이유로 신호를 무시한다.

**경위**: `6a2dec191`(10-01, 몽타주 노티파이 실행을 어빌리티의 몽타주 이벤트 태스크로 통합) 이전의 WeaponAttack·ApplyGameplayEffect 노티파이는 AnimatingAbility와 상관없이 MontageInstanceID로 처리했다.

**수정 방향**: `OwnsMontageSignal`에서 AnimatingAbility 조건을 빼고, `OwnedMontageInstanceID`(전역 고유)와 메시 일치만으로 판정한다. 끊긴 인스턴스는 엔진이 `HandleEvents`에서 새 노티파이를 모으지 않으므로 정지 여부를 따로 검사할 필요는 없다. `IsPlayingMontageInstance`는 제거하거나 `AnimInstance->GetMontageInstanceForID(MontageInstanceID)`의 활성 여부로 바꾼다. 같은 `GetCurrentMontage()` 기준을 쓰는 [WxAbilityTask_Rush.cpp:97](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_Rush.cpp#L97)도 함께 점검한다.

**검증**: 1차·2차 검증자 모두 확정(P1). 엔진 `PlayMontageInternal`·`NotifyAbilityEnded`와 몽타주 슬롯을 대조했다.

### D02 [P2] 퍼펙트 가드 직후 GuardReact 동안 퍼펙트 가드 판정이 계속 유지된다

**발생 조건**: 퍼펙트 가드가 나면 매번 생긴다. GuardReact(PerfectGuard 섹션, SlowTime으로 실제 시간은 더 길다) 중에 막을 수 있는 공격이 한 번 더 들어오면 드러난다.

**영향**: 0.25초로 저작한 퍼펙트 가드 구간이 반응 몽타주 길이만큼 늘어난다. 연속 공격의 다음 1타가 입력 타이밍과 상관없이 퍼펙트 가드(반사 GP, 공격자 패리, 투사체 되돌림, 슬로모션)로 처리된다. 서버 판정이라 그대로 복제된다.

**원인**: D01과 같다. GuardReact가 DefaultSlot 몽타주로 가드 몽타주를 끊으면 AnimatingAbility가 GuardReact로 바뀐다. 끊긴 가드 몽타주의 ApplyGameplayEffect 구간 종료 신호가 다음 틱에 와도 Guard의 태스크가 버린다. Guard는 GuardReact 중에 끊기면 종료하지 않고 대기만 하므로([WxAbility_Guard.cpp:73](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L73)), Infinite GE인 `WxEffect_PerfectGuard`가 GuardReact가 끝날 때까지 남는다.

**수정 방향**: D01 수정으로 함께 해결된다. Guard 쪽에 우회 장치를 따로 넣지 않는다.

**검증**: 1차·2차 확정(P2). 다른 리뷰어 1명도 같은 결함을 독립적으로 보고했다.

### D03 [P2] 원격 클라이언트에서 선입력한 후딜 캔슬이 서버에서 거부된다

**발생 조건**: 리슨 서버에 접속한 원격 클라이언트가 루트모션 공격 몽타주의 본동작 중에 회피·스킬·가드·아이템·궁극기를 미리 입력해 둔다. 이 입력이 StartRecovery 시점에 버퍼에서 재생될 때 생긴다. 리슨 호스트와 Standalone, 같은 어빌리티의 콤보 재발동, StartRecovery 이후 직접 누른 입력은 해당하지 않는다.

**영향**: 클라에서 잠깐 나간 동작이 `ClientActivateAbilityFailed`로 되감긴다. 직전 공격의 후딜 모션도 이미 취소돼 캐릭터가 아무 동작 없이 선다. 입력은 성공으로 처리돼 버퍼에서 지워졌으므로 다시 나가지 않는다. 원격 플레이어에게만 핵심 조작이 자주 실패한다.

**원인**: 소유 클라에서 루트모션 몽타주의 노티파이는 CMC `PerformMovement`→`TickCharacterPose` 안에서 디스패치된다. StartRecovery가 같은 콜스택에서 `FlushBufferedInputs`를 부르고([WxAbilityBase.cpp:223](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L223)), 거기서 곧바로 예측 발동이 일어난다. 엔진은 발동 RPC 전에 보류 중인 무브만 보내므로, StartRecovery를 넘긴 이번 프레임 무브는 발동 RPC보다 늦게 서버에 도착한다. 서버의 원격 폰 몽타주는 클라 무브로만 진행되므로, 발동 RPC를 처리하는 순간 서버의 공격은 아직 Blocking 단계다. 회피·스킬 등은 공격을 `CancelAbilitiesWithTag`로 지목하지 않으므로 차단에 걸려 거부된다([:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)). 그런데 클라는 새 어빌리티 안에서 이미 공격을 취소해 `ServerCancelAbility`를 보냈으므로, 서버의 공격도 취소된다.

**수정 방향**: StartRecovery의 버퍼 재생만 `HandleAbilityEnded` 경로처럼 `SetTimerForNextTick`으로 미룬다([WxInputBufferComponent.cpp:144](../../Source/WxGame/Input/WxInputBufferComponent.cpp#L144)에 선례). 다음 틱 타이머는 CMC 틱 뒤에 돌아, 현재 무브가 먼저 전송된다. OpenComboWindow의 재생은 이 결함과 관계없다. 다만 D04를 서버 단계 기준으로 고친다면 함께 미뤄야 한다.

**검증**: 1차·2차 확정(P2). 엔진 CharacterMovementComponent·AbilitySystemComponent의 순서와 공격 몽타주 에셋(StartRecovery Queued, 루트모션)을 대조했다.

### D04 [P2] 원격 클라이언트의 회피 반격 2단이 항상 서버에서 거부된다

**발생 조건**: 원격 클라이언트가 회피 반격 1단의 콤보 창(0.348~0.421초) 안에서, 또는 그 전에 미리 약공격을 입력해 2단을 발동한다.

**영향**: 2단이 시작했다가 바로 끊긴다(예측 비용·쿨다운 롤백). 서버에서만 실행되는 2단의 분신 소환(SpawnMinion)과 피해가 일어나지 않는다. 호스트와 원격 플레이어의 회피 반격이 다르게 동작한다.

**원인**: 클라는 `Occupant == this && ComboWindow`라 소유자 발동 조건(`Ability.Action.Dodge`)을 면제받고 2단을 예측 발동한다([WxAbilityBase.cpp:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)). 엔진의 재발동은 기존 인스턴스를 먼저 끝내 `ServerEndAbility`를 보낸 뒤 발동 RPC를 보낸다. 두 RPC는 순서가 보장되는 reliable RPC다. 그래서 서버는 자기 인스턴스를 먼저 끝낸다. 서버가 `CanActivateAbility`를 판정할 때는 Occupant가 없어 면제가 성립하지 않는다. 회피는 1단 발동 때 취소돼 `Ability.Action.Dodge` 태그도 없으므로 발동이 거부된다([WxAbility_Attack.cpp:57](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Attack.cpp#L57)). 약공격 콤보는 요구 태그가 없어 같은 순서로도 통과하므로, 회피 반격만 끊긴다.

**수정 방향**: 서버 판정이 서버 쪽 ActionPhase에 기대지 않게 한다. 최소안은 둘 중 하나다. 새 태그나 전달 인자는 추가하지 않는다.
- 회피 반격의 "회피 중" 조건을 첫 단 진입에만 적용한다.
- 같은 인스턴스가 직전에 취소가 아닌 원격 종료로 끝났다는 사실만으로 재발동 면제를 인정한다.

서버 인스턴스가 "ComboWindow 단계에서" 끝났는지로 판정하는 안은 쓸 수 없다. 클라의 선입력 재생이 D03과 같은 순서로 일어나면, 서버 몽타주는 아직 콤보 창 전일 수 있다.

**검증**: 1차·2차 확정(P2).

### D05 [P2] 회피 반격 1단의 후딜이 시작되면 2단 대신 약공격 1단이 나간다

**발생 조건**: 모든 머신에서 생긴다. 회피 반격 1단을 발동한 뒤 0.421초(StartRecovery)~0.967초(콤보 창 끝, ASPD 1 기준) 사이에 약공격을 누른다. AM_HGTest_Attack_DodgeCounter1, AM_Template_DodgeCounter_1 모두 해당한다.

**영향**: 2단(HGTest는 분신 소환 포함)으로 들어갈 수 있는 구간이 약 73ms(0.348~0.421초)와 그 전 선입력뿐이다. 콤보 창의 대부분에서는 약공격 1단이 나간다.

**원인**: 콤보 재발동의 면제는 `ActionPhase == ComboWindow`일 때만 성립한다([WxAbilityBase.cpp:244](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp#L244)). StartRecovery가 창 도중에 오면 남은 창 구간은 Recovery가 되고 차단도 풀려 Occupant가 null이 된다. 회피 반격은 `Ability.Action.Dodge`를 요구하는데 회피는 이미 취소됐으므로 일반 검사에서 실패한다. 같은 IA_Attack_Light의 약공격은 AbilitySet 순서상 먼저 시도되고, 차단이 풀려 있어 통과한다([WxAbilitySystemComponent.cpp:181](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L181)). 약공격 콤보는 같은 겹침에서도 자기 조건이 늘 성립해 이어진다. 직전 어빌리티의 일시 태그에 기대는 회피 반격만 끊긴다.

**수정 방향**: 두 가지를 함께 바꿔야 한다.
- 면제는 "활성 인스턴스이고 창이 아직 닫히지 않음"으로 판정한다. 차단 1건 제외는 Occupant가 있을 때만 적용한다. 지금 구조에서 `bComboRetrigger`만 Recovery까지 넓히면 253~258행이 null Occupant를 역참조한다.
- 같은 입력에서는 창이 열린 활성 콤보 스펙을 먼저 시도한다.

데이터만으로 막으려면 DodgeCounter1 몽타주의 ComboWindow 끝을 StartRecovery 앞으로 당긴다. 이 경우 후딜 캔슬 시작이 늦어지므로 기획 확인이 필요하다. D04는 어느 쪽으로 고쳐도 남는다.

**검증**: 1차·2차 확정(P2). 몽타주 노티파이 시각은 에셋을 파싱해 확인했다.

### D06 [P2] HGTest 회피 반격 2단이 피해를 주지 않는다

**발생 조건**: 프론트엔드에서 BP_HGTest를 고르고, 회피 반격 2단(AM_HGTest_Attack_DodgeCounter2)을 적에게 맞힌다.

**영향**: 마무리 타격이 피해·피격 반응·히트스톱·큐를 모두 내지 않는다. 적중할 때마다 FindRow 경고만 남는다.

**원인**: [AM_HGTest_Attack_DodgeCounter2](../../Content/Character/HGTest/Abilities/Attack_DodgeCounter/AM_HGTest_Attack_DodgeCounter2.uasset)의 WeaponAttack 노티파이가 `DT_Damage.AM_Attack_LLLL`을 가리키는데, 이 행이 DT_Damage에 없다. `a9bfa4625`(09-15, 에셋 테이블에도 Template_ 붙임)에서 행 이름이 `AM_Template_Attack_LLLL`로 바뀌었는데 이 몽타주만 갱신되지 않았다. DataTableRowFixup 도입(09-23)보다 앞선 일이다. 행을 찾지 못하면 [UWxCombatLibrary::ApplyDamage](../../Source/WxGame/Combat/WxCombatLibrary.cpp#L71)가 GE 없이 false를 반환한다. 어빌리티·이펙트 목록 생성기도 이 참조를 "표에 없는 행을 가리키는 참조"로 표시하고 있다.

**수정 방향**: DamageDataRow를 있는 행으로 바꾼다. 원래 수치는 `AM_Template_Attack_LLLL`이고, HGTest 전용 수치가 필요하면 DT_Damage에 행을 추가한다. GA가 쓰지 않는 번호 없는 AM_HGTest_Attack_DodgeCounter도 같은 낡은 행을 가리킨다([Q39](#q-품질-개선)).

**검증**: 1차·2차 확정(P2). 2단에 실제로 도달한다는 것(콤보 재발동은 필수 태그 검사를 건너뜀)도 확인했다.

### D07 [P2] 일반 피격으로는 아이템 사용이 끊기지 않는다

**발생 조건**: 포션을 마시는 도중(회복 노티파이 0.276초 전)에 적 공격을 받는다. 현재 적 피해 행은 모두 `HitReact.Normal`이다. 무적이 끝난 회피 후딜(0.5~0.9초)에 맞아도 같다.

**영향**: 피격 중에도 포션 회복이 끝까지 적용된다. 위키·기획의 "아이템 사용: 피격 시 캔슬 O" 규칙이 동작하지 않는다.

**원인**: HitReact의 `CancelAbilitiesWithTag`에는 `Ability.Action.Attack`과 `Ability.Action.Skill`만 있다([WxAbility_HitReact.cpp:24](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L24)). 일반 피격 몽타주는 가산 슬롯 그룹이라 UpperBody의 UseItem, DefaultSlot의 Dodge 몽타주를 끊지 않는다. 그래서 끊기는 경로가 없다. 피격 몽타주가 같은 슬롯을 덮어 액션이 저절로 끝나던 시절(`2bbbea01a`)의 전제가 일반 피격의 가산 슬롯 이전으로 깨졌다.

**수정 방향**: `CancelAbilitiesWithTag`에 `Ability.Action.UseItem`을 추가한다. 무적 밖 회피도 끊는 것이 의도라면 `Ability.Action.Dodge`도 추가한다. 무적 구간에서는 `WxEffect_Invincible`이 HitReact 발동 자체를 막으므로 무적 프레임 예외는 유지된다. Pattern 제외는 그대로 둔다. **D08과 함께 고친다.**

**검증**: 1차·2차 확정(P2). 회피 캔슬은 위키 표기가 "△(무적 프레임)"라 의도를 확인해야 한다.

### D08 [P2] 회복 전에 끊긴 포션이 소비되지 않는다

**발생 조건**: 포션 사용 입력 뒤, 회복 노티파이 전에 어빌리티가 끊긴다. 지금은 넉 계열 피격(DefaultSlot 몽타주가 같은 그룹의 UseItem 몽타주를 멈춤), 사망, 그로기가 해당한다. D07을 고치면 일반 피격도 해당한다.

**영향**: 맞아도 포션이 줄지 않아, 기획 목적인 "사용 중 빈틈의 리스크"가 사라진다. 보스 앞에서 실패해도 손실 없이 다시 마실 수 있다.

**원인**: 발동 시에는 `BeginUseItem`으로 대기 플래그만 세운다([WxAbility_UseItem.cpp:59](../../Source/WxGame/Inventory/WxAbility_UseItem.cpp#L59)). 충전 차감과 회복 GE 적용은 모두 노티파이 시점의 `UseConsumable` 한 곳에서 일어난다([WxItemUseComponent.cpp:85](../../Source/WxGame/Inventory/WxItemUseComponent.cpp#L85)). 노티파이 전에 끝나면 `EndUseItem`이 플래그를 지워 아무것도 차감되지 않는다. 기획서(에스트병 기획서, 05-30)는 "GA 실행 시 1 감소, 회복 전 피격은 포션만 소비"이고, 구현(`43860c8aa`, 06-10)은 처음부터 노티파이 시점 차감이었다. 차이를 의도로 정한 기록은 없다.

**수정 방향**: `UseConsumable`을 차감과 적용으로 나눈다. 권위에서 커밋 직후 고른 인스턴스의 충전을 1 줄이고, 기존 `bUsePending` 플래그 자리에 그 인스턴스를 약참조로 둔다. 노티파이에서는 그 인스턴스의 GE만 적용한다. 차감 뒤 충전이 0이면 `FindConsumableInstance`가 그 인스턴스를 건너뛰므로, 노티파이에서 다시 찾지 말고 기억한 인스턴스를 써야 한다. 클라 예측 장치는 추가하지 않는다. 기획과의 차이가 의도라면 기획 확인 뒤 결정으로 기록한다.

**검증**: 1차·2차 확정(P2). 2차 검증자가 발생 조건을 "태그 캔슬"에서 "몽타주 슬롯 그룹"으로 바로잡았다.

### D09 [P2] 분신이 가드를 따라 하면 가드 자세로 굳어 이후 동작을 따라 하지 않는다

**발생 조건**: BP_HGTest가 궁극기 1로 분신(BP_Doppelganger)을 소환한 뒤 가드 키를 눌렀다 뗀다.

**영향**: 분신은 궁극기 2 등으로 거둘 때까지 가드 자세로 남는다. 그동안 공격·회피·스킬 따라 하기가 모두 거절되고, 위치 보정 텔레포트도 멈춘다(이동 추종은 계속된다). 궁극기 2 자체는 막히지 않는다.

**원인**: BT_Doppelganger의 MirrorAbility `ExcludedAbilities`에는 `Ability.Finisher`, `Ability.Action.Ultimate`만 있다([WxBTTask_MirrorAbility.cpp:85](../../Source/WxGame/AI/WxBTTask_MirrorAbility.cpp#L85)). 마스터의 가드가 커밋되면 분신도 같은 가드를 발동한다. 가드가 끝나는 정상 경로는 입력 해제뿐인데 AI 분신에는 입력이 오지 않는다. `IsInputHeld`도 AI에게는 항상 true다([WxAbility_Guard.cpp:110](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L110)). MirrorAbility는 마스터의 종료를 따라가지 않는다. 가드는 Recovery 단계가 없어 `Ability.Action` 차단을 계속 건다. 기획(현광)상 분신이 따라 하는 동작은 기본 공격·회피 반격·이동·대시이고 가드는 없다.

**수정 방향**: BT_Doppelganger의 `ExcludedAbilities`에 `Ability.Action.Guard`와 `Ability.GuardReact`를 추가한다(데이터 변경). GuardReact도 빼야 한다. 분신은 `IgnoreAbilityActivationTags`로 필수 태그를 건너뛰므로, 가드하지 않은 분신이 가드 피격 연출을 재생한다. 마스터 종료를 추적하는 동기화 장치는 만들지 않는다.

**검증**: 1차·2차 확정(P2). "궁극기 1→2 루프가 깨진다"는 최초 보고는 과장으로 정정됐다.

### D10 [P2] 셀이 다시 로드되면 퀘스트 볼륨이 다시 발동해 퀘스트가 처음부터 재시작된다

**발생 조건**: Standalone에서 LV_OpenWorld의 퀘스트 볼륨((732,-268) m)을 밟아 Main2를 받는다. 진행 중이든 완료했든 상관없다. 그 뒤 약 768 m 이상 떨어진 맵 반대편까지 갔다가 돌아와 볼륨에 다시 들어간다. 리슨 서버는 서버가 셀을 언로드하지 않아 해당하지 않는다. 체크포인트 부활 거리(약 570 m)로는 일어나지 않는다.

**영향**: 진행 중인 메인 퀘스트가 첫 단계로 돌아가고 대화가 다시 재생된다. 완료한 퀘스트도 다시 진행할 수 있게 되어, 끝까지 하면 보상(DT_Reward)을 다시 받는다.

**원인**: BP_QuestVolume은 겹침 시 `StartQuest`를 부른 뒤 `SetActorEnableCollision(false)`로 1회 발동을 구현한다. 이 상태는 액터 런타임 값뿐이다. 배치된 볼륨은 공간 로드 기본값이라, 셀이 다시 로드되면 콜리전이 켜진 새 인스턴스가 된다. [UWxQuestComponent::ActivateQuest](../../Source/WxGame/Quest/WxQuestComponent.cpp#L34)는 같은 에셋이 실행 중인지, 완료했는지 확인하지 않고 `StopLogic`→`StartLogic`한다. 퀘스트 트리에는 장치 컴포넌트가 없어 `IsRestoring`이 false이므로 GiveRewards도 매번 지급한다.

**WxSave 삭제 결정과의 관계**: `b3f982b0d`(09-01, WxSave 삭제)는 "셀 재진입 시 처치·장치 상태 유지"가 사라지는 것을 감수했다. 이 결함은 그 범위 밖이다. 진행 중인 메인 퀘스트 자체를 되돌린다.

**수정 방향**: BP_QuestVolume 클래스 기본값에서 Is Spatially Loaded를 끈다(엔진 순정 설정). 세션 동안 언로드되지 않아 1회 발동 상태가 유지된다. `ActivateQuest`에 "같은 에셋 Running이면 무시"만 넣으면 완료한 퀘스트의 재수주와 보상 재지급은 막지 못한다. 완료 기록의 영속화는 저장 범위를 정할 때 다룬다. **이 수정을 하면 D11의 우연한 복구 경로가 사라지므로 D11과 함께 고친다.**

**검증**: 1차·2차 확정(P2).

### D11 [P2] 처치 퀘스트가 스포너 셀이 다시 로드된 뒤 끝나지 않는다

**발생 조건**: Standalone에서 LV_OpenWorld Main2의 처치 단계 중, 적을 남긴 채 스포너 셀에서 약 768 m 이상 떨어졌다가 돌아온다. 적이 KillZ 아래로 떨어지는 경우도 같다([D35](#d35-p3-killz-아래로-떨어진-캐릭터가-사망-경로-없이-파괴된다)). 리슨 서버는 해당하지 않는다.

**영향**: 퀘스트 목표가 "적 처치"에서 멈추고, 처치할 적이 다시 나오지 않는다. 지금은 D10 때문에 퀘스트 볼륨을 다시 밟으면 Main2가 처음부터 재시작되어 우연히 풀린다.

**원인**: 셀이 다시 로드된 Manual 스포너는 `bIsKilled=false`인 새 인스턴스이고, BeginPlay에서 스폰하지 않는다([WxSpawner.cpp:111](../../Source/WxGame/Spawner/WxSpawner.cpp#L111)). 발동 태스크(TriggerSpawners)는 단계 진입 때 한 번만 실행된다. [WaitSpawnersKilled](../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L136)는 해석된 스포너의 `IsKilled()`만 보므로 영원히 Running에 머문다. 처치 기록이 셀과 함께 사라지는 것 자체는 WxSave 삭제 때 감수한 대가다([WxSpawner.h:67](../../Source/WxGame/Spawner/WxSpawner.h#L67) 주석). 하지만 그 결과 메인 퀘스트가 막히는 것은 별개 문제다.

**수정 방향**: 판단 주체는 처치 대기 태스크로 둔다. 주기 판정에서 "해석된 스포너가 처치되지 않았고 추적 중인 스폰 대상도 없음"이면 `Respawn()`을 부른다. 스포너에 스폰 대상 유효 여부를 묻는 조회 함수가 하나 필요하다. 재로드되면 처치 기록도 사라지므로, 이미 잡은 대상을 다시 스폰하지 않게 태스크 인스턴스 데이터에 "처치를 확인한 스포너"를 남긴다. 스포너를 상시 로드로 바꾸는 우회는 쓰지 않는다. 스폰 대상이 퍼시스턴트 레벨에 생겨 지형이 언로드되면 떨어질 수 있다. 저장 범위를 정할 때 함께 다시 볼 수도 있다.

**검증**: 1차·2차 확정. 최초 보고는 P1이었다. 현재 배치에서는 체크포인트 부활이나 단계 진입 시점의 미로드로는 생기지 않아 P2로 낮췄다.

## P3 결함

### 전투·어빌리티

#### D12 [P3] 가드를 다시 올릴 때마다 0.25초 퍼펙트 가드 구간이 입력 없이 다시 열린다
- **조건·영향**: 가드 키를 누른 채 연속 공격을 막다가 GuardReact 직후 약 0.25초 안에 다음 타격이 오면, 타이밍 입력 없이 퍼펙트 가드가 된다. D02와 겹치면 연쇄된다.
- **원인**: `HandleGuardReactEnded`가 섹션 지정 없이 `PlayMontage`를 불러 AM_Shared_Guard를 0초부터 다시 재생한다([WxAbility_Guard.cpp:103](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Guard.cpp#L103)).
- **수정**: 다시 올릴 때는 유지 구간 섹션부터 재생하고, 첫 재생만 Default부터 한다. 예전 자동 재발동 방식도 Default부터 재생했으므로 의도인지 기획 확인이 필요하다. 2차 검증에서 P2에서 P3으로 낮췄다.

#### D13 [P3] 같은 몽타주를 연속으로 재발동하면 섹션 시작의 구간 노티파이가 두 번째 재생에서 빠진다
- **원인**: `UWxAnimNotifyState_AbilityEvent`에 `NoMergeOnConcurrentPlay` 플래그가 없다([WxAnimNotify_AbilityEvent.h:24](../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.h#L24)). 엔진이 새 Queued 구간을 기존 활성 상태와 합쳐 새 인스턴스의 NotifyBegin을 보내지 않는다.
- **현재 예**: GuardReact PerfectGuard 섹션의 SlowTime 구간 안에서 퍼펙트 가드가 다시 나면, 두 번째 슬로모션이 걸리지 않는다. 섹션 시작에 무적·콤보 창을 두면 같은 식으로 조용히 사라진다.
- **수정**: 생성자에서 `NotifyStateBehaviorFlags`에 `EAnimNotifyStateBehaviorFlags::NoMergeOnConcurrentPlay`를 켠다(엔진 순정 플래그). 기존 에셋은 CDO 기본값을 따른다.

#### D14 [P3] 넉백 경직 중 약 1초 동안 이동 입력·AI 경로가 캐릭터를 움직인다
- **원인**: AM_Shared_HitReact_Knock의 DisableRootMotion은 KnockBack 섹션 전체(0~1.316초)를 덮는데, 상수 힘은 0.3초만 민다. 나머지 구간에는 애니 루트모션도 루트모션 소스도 없어, CMC가 입력·경로로 캐릭터를 움직인다([WxAbility_HitReact.cpp:136](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L136)). 락온하지 않은 캐릭터는 이동 방향으로 회전도 한다. 메모리에 기록된 "리액션 몽타주는 루트모션 보유" 전제가 이 구간에서는 성립하지 않는다.
- **현재 도달**: 플레이어 궁극기(`HitReact.KnockBack`)에 맞은 적이 경로 이동을 계속할 때. 적 피해 행에 KnockBack이 생기면 플레이어 피격과 GuardKnockback에도 생긴다.
- **수정**: 애니 루트모션을 끈 구간 동안 루트모션 소스를 유지한다. 예를 들어 ConstantForce의 Duration을 섹션 길이로 두고 StrengthOverTime 커브로 앞 0.3초만 민다. DisableRootMotion만 줄이면 애니에 구워진 변위가 이중으로 적용될 수 있어 에셋 확인이 필요하다. 2차 검증에서 P2에서 P3으로 낮췄다.

#### D15 [P3] 퍼펙트 가드의 반사 GP로 그로기에 든 공격자에게 패리 리액션도 보낸다
- **영향**: 그로기 시작 자세 대신 패리 넉이 먼저 재생된다. 그로기 몽타주가 Parry 길이만큼 늦게 시작해, 드레인이 끝날 때 꼬리가 잘린다.
- **원인**: 그로기 검사를 AddGP 전에 한 번만 한다. 반사 GP로 그로기가 동기 발동된 뒤에도 `Event.Hit.Parry`를 보낸다([WxEffectComponent_PerfectGuard.cpp:46](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_PerfectGuard.cpp#L46)).
- **수정**: 패리 이벤트 조건에 AddGP 뒤 시점의 `!SourceASC->HasMatchingGameplayTag(Ability_Groggy)` 한 줄을 추가한다. 2차 검증에서 P2에서 P3으로 낮췄다.

#### D16 [P3] 그로기 적을 처형하는 중 제3자의 일반 피격이 들어오면 처형 짝 몽타주가 그로기 자세로 덮인다
- **조건**: 처형 중 플레이어 미니언(`HitReact.Normal` 행)이나 다른 플레이어가 같은 적을 때린다.
- **원인**: Groggy의 몽타주 폴링이 "재생 중인 몽타주가 없다"를 `ASC->GetCurrentMontage()`로 판정한다([WxAbility_Groggy.cpp:115](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp#L115), :135). 가산 HitReact가 이 값을 가져갔다가 비우면, 비었다고 보고 그로기 몽타주를 다시 튼다. 그러면 같은 그룹의 처형 짝 몽타주가 끊긴다. D01과 같은 "ASC의 단일 몽타주 추적" 문제다.
- **수정**: 두 곳의 판정을 "그로기 몽타주와 같은 그룹에 활성 몽타주 인스턴스가 있는가"로 바꾼다([WxCharacterMovementComponent.cpp:110](../../Source/WxGame/Character/WxCharacterMovementComponent.cpp#L110)처럼 MontageInstances 순회). 폴링 구조는 유지한다.

#### D17 [P3] 처형 중에도 대상의 GP 드레인·누적이 계속돼 그로기가 도중에 풀리거나 다시 걸린다
- **조건**: 그로기가 끝나기 직전에 처형을 시작해 처형이 남은 드레인보다 길거나, 비교전 적을 뒤에서 처형한다.
- **원인**: Finisher는 대상에 `State.FinisherReserved`만 붙이고 DrainGP·GP 누적은 그대로 둔다([WxAbility_Finisher.cpp:62](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L62)). 처형 피해(AM_Shared_Finisher 행, 86)가 GP로 쌓인다.
- **수정**: GP 누적 조건([WxEffect_Damage.cpp:204](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L204)의 `!bIsGroggy`)에 "대상에 `State.FinisherReserved` 없음"을 함께 건다. 드레인 정지는 그로기 종료 판정과 함께 설계해야 하므로 기획 확인이 필요하다.

#### D18 [P3] 그로기를 일으킨 타격이 일반 피격 반응을 함께 재생한다
- **원인**: ExecCalc가 IncomingDamage 다음에 GP를 출력해 그로기가 먼저 발동한다. DamageReaction은 진입 타격도 "그로기 중"으로 보고 Normal 반응을 보낸다([WxEffectComponent_DamageReaction.cpp:59](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_DamageReaction.cpp#L59)). 기획은 반응 없음 또는 그로기 시작 리액션이다.
- **수정**: "이번 실행에 GP 기록 있음 + `Ability.Groggy` 보유"면 ReactionTag를 비운다. `Event.Hit`과 DamageDealt는 그대로 보낸다. 그로기 중 피격 반응은 위키에서 미결이므로 건드리지 않는다.

#### D19 [P3] 그로기 중 받는 피해 +30% 규칙이 피해 계산에 없다
- **원인**: ExecCalc는 `bIsGroggy`를 GP 누적을 건너뛰는 데만 쓴다([WxEffect_Damage.cpp:124](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L124)). 적 규격서·그로기 시스템·Elite_monster 기획서는 이를 공통 규칙으로 두고, 보류한 기록은 없다.
- **수정**: `UWxCombatDeveloperSettings`에 그로기 피해 배율(1.3)을 두고 `CalculateFinalDamage`에서 곱한다. 적용하면 앞잡 피해가 86에서 112로 올라 TemplateEnemy가 앞잡 한 번에 죽는다. 앞잡에도 적용할지 기획 확인이 필요하다.

#### D20 [P3] 강공격·스킬도 반응이 Normal이면 적 패턴을 끊지 못한다
- **원인**: HitReact는 피해 출처를 보지 않고 Pattern을 늘 취소 대상에서 뺀다([WxAbility_HitReact.cpp:22](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp#L22)). 어빌리티 규칙 브리핑은 "플레이어 평타만 패턴 캔슬 불가"다. `HitReact.Normal`인 AM_Template_Attack_H, AM_HGTest_Attack_Heavy, AM_HGTest_Skill_2·3 등이 해당한다.
- **수정**: 반응이 확정된 뒤 원인 어빌리티(`TriggerEventData->ContextHandle.GetAbility()`)가 `Ability.Action.Attack.Light`가 아니면 `Ability.Action.Pattern`을 취소한다. Pattern 주석([Q26](#q-품질-개선))도 함께 고친다.

#### D21 [P3] 회피 후딜에서 다음 회피로 후딜 캔슬이 되지 않는다
- **영향**: 2회 충전 회피를 연달아 쓸 때 두 번째가 약 0.4초 늦다. 위키 규칙은 "StartRecovery 이후 모든 액션으로 캔슬 가능"이다.
- **원인**: Dodge가 `bRetriggerInstancedAbility`를 켜지 않아, Recovery에서 차단이 풀려도 엔진이 활성 스펙의 재발동을 거부한다([WxAbility_Dodge.cpp:21](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Dodge.cpp#L21)).
- **수정**: 생성자에서 `bRetriggerInstancedAbility = true`(엔진 순정). Blocking 단계는 자기 차단이 막는다. 연속 회피 금지가 의도라면 주석으로 남긴다.

#### D22 [P3] 극한 회피 성공 신호가 클라 후딜 뒤에 도착하면 클라에서만 성공 섹션이 Recovery로 시작한다
- **조건**: 원격 클라에서 무적 창이 끝나기 전 약 RTT 안에 피격된다.
- **영향**: 클라에서는 성공 섹션을 다른 액션으로 캔슬할 수 있지만 서버에서는 막혀, 예측 발동이 거부·롤백된다.
- **원인**: `HandleDodgeSuccess`가 PlayMontage만 다시 부르고 ActionPhase를 Blocking으로 되돌리지 않는다([WxAbility_Dodge.cpp:202](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Dodge.cpp#L202)).
- **수정**: `SetActionPhase`를 protected로 옮기고 `HandleDodgeSuccess`에서 PlayMontage 전에 Blocking으로 설정한다.

#### D23 [P3] HUD 슬롯 버튼으로 콤보를 이으면 원격 클라와 서버의 콤보 단계가 어긋날 수 있다
- **원인**: VM의 발동은 이벤트 데이터 없이 `TryActivateAbility`를 불러 각 머신이 자기 `ComboIndex+1`을 쓴다. 키 입력 경로만 단계를 실어 보낸다([WxAbility_Combo.cpp:46](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Combo.cpp#L46)).
- **조건**: 원격 클라에서 커서·터치로 HUD 버튼을 쓸 때. 지금은 Game 입력 모드에서 마우스가 캡처돼 드물다.
- **수정**: VM 발동도 ASC의 단계 동반 경로를 쓰게 하고, [WxAbility_Skill.h:13](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Skill.h#L13)·Combo.cpp:46 주석을 실제 경로에 맞춘다.

#### D24 [P3] 클라 위치 보정 재실행 중 OnJumped가 다시 불려 진행 중인 후딜을 취소한다
- **원인**: 엔진 `CheckJumpInput`은 저장 무브를 재실행하는 중에도 점프 시작 무브면 OnJumped를 부른다. 그러면 [OnJumped_Implementation](../../Source/WxGame/Character/WxCharacterBase.cpp#L115)이 현재 후딜 액션을 취소한다.
- **수정**: `bClientUpdating`이면 후딜 취소를 건너뛴다(조건 한 줄).

#### D25 [P3] MaxSP가 0인 아바타의 질주가 움직이는 즉시 끝난다
- **원인**: `HandleSPChanged`가 `NewValue <= 0`이면 바로 종료한다([WxAbility_Sprint.cpp:130](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Sprint.cpp#L130)). 엔진은 값이 바뀌지 않아도 변경 델리게이트를 방송한다. 주석은 "제한 없이 달린다"이다.
- **현재 도달**: 마스터의 질주를 미러링하는 BP_Doppelganger.
- **수정**: "제한 없이"가 의도라면 실제로 감소했을 때만 종료하고, 해제 입력이 없는 분신을 위해 BT_Doppelganger `ExcludedAbilities`에 `Ability.Sprint`를 넣는다. 아니면 주석을 실제 동작에 맞춘다.

#### D26 [P3] 마지막 타격으로 죽은 대상에도 피해 행의 추가 효과 GE를 적용한다
- **원인**: Damage GE의 `TargetTagRequirements`는 적용 시점에만 검사한다. 마지막 컴포넌트인 AdditionalEffects는 퍼펙트 가드 여부만 본다([WxEffectComponent_AdditionalEffects.cpp:26](../../Source/WxGame/AbilitySystem/Effects/WxEffectComponent_AdditionalEffects.cpp#L26)). 현재 추가 효과를 쓰는 행이 없어 잠재 상태다.
- **수정**: 루프 앞에 대상이 `Ability.Death`면 return하는 조건 한 줄.

#### D27 [P3] Rush 돌진 시간 계산이 몽타주 RateScale을 빠뜨린다
- **원인**: 구간 길이를 `Instance->GetPlayRate()`로만 나눈다([WxAbilityTask_MontageEvents.cpp:341](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp#L341)). RateScale이 1보다 크면 이동이 느려져 구간 끝에서 잘리고 목표에 닿지 못한다. 1보다 작으면 이동이 먼저 끝난다. 현재 두 에셋은 RateScale 1이라 잠재 상태다.
- **수정**: `GetPlayRate() * Montage->RateScale`로 나눈다.

#### D28 [P3] BP_HGTest의 회피 잔상이 숨긴 구동 메시(SKM_Quinn_Simple) 모습으로 나온다
- **원인**: AWxGhostTrail이 `GetMesh()`의 에셋·포즈를 복사하는데, MetaHuman 캐릭터의 `GetMesh()`는 숨겨진 구동 메시다([WxCueNotify_GhostTrail.cpp:42](../../Source/WxGame/AbilitySystem/Cues/WxCueNotify_GhostTrail.cpp#L42)).
- **수정**: `UWxMetaHumanComponent`에 표시 바디 에셋 조회 함수를 두고, 있으면 바디 에셋으로 바꾼 뒤 포즈를 복사한다. 바디 실루엣만 맞고 Face·그룸·Outfit은 여전히 빠진다. BP_GhostTrail의 OverrideMaterials가 바디 메시 슬롯 수에 맞는지도 확인한다.

### AI

#### D29 [P3] 정찰 지점이 스폰 지점에서 30 m 밖이면 정찰과 복귀를 끝없이 반복한다
- **원인**: BT_Template·BT_Soldier 루트의 0번 자식 BeyondLeash(Lower Priority)에 TargetActor 조건이 없어, 전투가 아닌 정찰에도 리시가 걸린다([WxBTDecorator_BeyondLeash.cpp:72](../../Source/WxGame/AI/WxBTDecorator_BeyondLeash.cpp#L72), HomeLocation 기록 [WxAIController.cpp:105](../../Source/WxGame/AI/WxAIController.cpp#L105)). 기획의 리시는 추격 종료 조건이다.
- **현재 도달**: 없다. 유일한 정찰 스포너(LV_DevCombat)의 가장 먼 지점이 약 11.7 m다.
- **수정**: 최소안은 `UWxPatrolComponent` 클래스 주석에 "정찰 지점은 리시 반경 안"이라는 제약을 적는 것이다(검증기는 추가하지 않는다). BT로 막으려면 리시 브랜치에 TargetActor Is Set 조건을 Observer aborts **Lower Priority**로 둔다. None으로 두면, 30 m 밖에서 타겟을 잡은 정찰 적이 리시 없이 계속 싸우는 빈틈이 생긴다. 30 m 밖에서 발견하면 곧바로 귀환하게 되므로 기획 확인이 필요하다. 리뷰어 2명이 같은 결함을 따로 보고했다.

#### D30 [P3] RandomChoice가 뒤쪽 자식의 Lower Priority·Both 데코레이터를 관찰자로 남긴다
- **영향**: 조건이 바뀌면 개발 빌드에서 엔진 ensure가 뜨고, 실행 중인 패턴이 가중치·반복 회피를 무시하고 강제로 바뀐다. 현재 에셋에서는 드러나지 않는다.
- **원인**: 사전 필터가 인덱스와 상관없이 `NotifyDecoratorsOnFailedActivation`을 불러, 뽑힌 자식보다 뒤쪽 자식의 관찰자도 등록된다([WxBTComposite_RandomChoice.cpp:67](../../Source/WxGame/AI/WxBTComposite_RandomChoice.cpp#L67)).
- **수정**: `UBTComposite_Sequence`처럼 `CanAbortLowerPriority()`를 false로 재정의(WITH_EDITOR)해, 엔진이 자식 데코레이터의 abort 모드를 None·Self로 정리하게 한다. 헤더 27~28행 설명도 맞춘다.

#### D31 [P3] 순찰 속도 배율 기본값 0.5가 기획(기본 걷기의 0.75배)과 다르다
- [WxBTTask_Patrol.h:39](../../Source/WxGame/AI/WxBTTask_Patrol.h#L39). 이 노드를 쓰는 BT_Soldier·BT_Template 모두 값을 덮어쓰지 않는다.
- **수정**: 기본값을 0.75로 바꾸거나 두 BT에서 지정한다.

#### D32 [P3] 실제 청각 거리가 3 m로 기획(1 m)과 다르다
- **원인**: 리스너 HearingRadius 기본값이 1000 cm이고 Jog 노티파이 HearingDistance가 300 cm라, 짧은 쪽인 3 m가 된다([WxAIBehaviorComponent.h:71](../../Source/WxGame/AI/WxAIBehaviorComponent.h#L71)).
- **수정**: 적 BP(BP_Soldier, Enemy/BP_Template)에서 HearingRadius를 100으로 지정한다. C++ 기본값을 바꾸면 플레이어 팀 소환물(BP_Minion)의 청각도 1 m가 된다는 점을 함께 판단한다.

#### D33 [P3] 청각만 엔진 기본 피아 판정을 써서 Neutral(255) 소음을 적대로 듣는다
- **원인**: 엔진 청각은 `FGenericTeamId` 솔버를 쓰는데, 프로젝트가 솔버를 등록하지 않아 기본값(팀이 다르면 Hostile)이 적용된다([WxAIController.cpp:31](../../Source/WxGame/AI/WxAIController.cpp#L31)). 시야·피해 감지는 폰 규칙(255면 Neutral)을 쓴다. 현재 Neutral 캐릭터가 없어 잠재 상태다.
- **수정**: WxGame 모듈 시작 시 `FGenericTeamId::SetAttitudeSolver`로 AWxCharacterBase와 같은 규칙을 등록하고 종료 시 Reset한다. 그러면 `AWxCharacterBase::GetTeamAttitudeTowards`의 규칙 복제를 걷어 한 곳으로 모을 수 있다. 지금은 `FDefaultGameModuleImpl`이라 모듈 클래스가 필요하다.

#### D34 [P3] 죽은 적의 AI 퍼셉션이 계속 돌아 시체마다 시야 쿼리 비용이 남는다
- **원인**: `HandlePawnDeath`가 `StopLogic`만 한다([WxAIController.cpp:179](../../Source/WxGame/AI/WxAIController.cpp#L179)). CorpseLifeSpan이 0이고 스포너가 시체를 다음 Respawn까지 남기므로, 리스너가 유지된다. 트레이스 비용은 시체의 15 m·120° 원뿔 안에 플레이어 쪽 폰이 있을 때 생긴다.
- **수정**: `HandlePawnDeath`에서 퍼셉션 컴포넌트를 `UnregisterComponent()`한다. OnUnPossess의 중복 호출은 엔진이 로그만 남기므로, 필요하면 `IsRegistered`로 가린다.

### 플레이어·월드·퀘스트·인벤토리

#### D35 [P3] KillZ 아래로 떨어진 캐릭터가 사망 경로 없이 파괴된다
- **원인**: AWxCharacterBase가 `FellOutOfWorld`를 재정의하지 않아 엔진 기본 동작(Destroy)을 탄다.
- **영향**: 플레이어는 `Ability.Death` 없이 폰이 사라져 사망 화면이 뜨지 않고, `RequestRespawn`의 전제도 깨진다([WxRespawnLibrary.cpp:34](../../Source/WxGame/Player/WxRespawnLibrary.cpp#L34)). 파괴된 뒤에는 메뉴도 없어 강제 종료해야 한다. 적은 처치 통지 없이 사라져 Manual 스포너 처치 퀘스트가 끝나지 않는다([WxSpawner.cpp:191](../../Source/WxGame/Spawner/WxSpawner.cpp#L191), D11과 같은 결과).
- **조건**: 맵들이 KillZ를 재정의하지 않아 기본값(약 -10.5 km)까지 수 분간 떨어져야 한다. 지형 가장자리 밖이나 충돌 틈으로 떨어질 때만 생긴다.
- **수정**: Lyra의 `ALyraCharacter::FellOutOfWorld`처럼 재정의한다. Destroy 대신, 아직 `Ability.Death`가 없을 때만 정상 사망 경로(현재 HP만큼 피해)를 태운다. 맵의 KillZ를 지형에 맞게 올리는 것도 검토한다. 리뷰어 2명이 플레이어·적 쪽을 따로 보고했고, 2차 검증에서 P2에서 P3으로 낮췄다.

#### D36 [P3] 리슨 서버에서 한 사람이 체크포인트에서 휴식하면 월드의 모든 Auto 스포너가 리젠된다
- **원인**: [RespawnSpawners](../../Source/WxGame/Spawner/WxStateTreeTask_RespawnSpawners.cpp#L31)가 넷 모드를 확인하지 않고 `RespawnAll`을 부른다. 체크포인트 저장·부활은 Standalone 전용이다. 리슨 서버는 기본 설정에서 서버 스트리밍을 하지 않아 모든 셀이 로드돼 있다.
- **영향**: 다른 플레이어가 싸우던 적(HP를 깎아 둔 보스 포함)이 사라졌다가 만피로 다시 나온다.
- **수정**: `EnterState`에 SaveCheckpoint와 같은 Standalone 조건 한 줄을 둔다(공용 `RespawnAll`이 아니라 호출부에). 멀티플레이의 휴식 규칙은 기획 결정으로 받는다.

#### D37 [P3] 체크포인트 휴식의 포션 리필이 휴식한 플레이어가 아니라 0번 PC(리슨 호스트)에게 간다
- **원인**: [RefillItemCharges](../../Source/WxGame/Inventory/WxStateTreeTask_RefillItemCharges.cpp#L36)가 `GetPlayerController(Owner, 0)`을 쓴다. 같은 ST_CheckPoint의 HP 회복은 상호작용자에게 적용되어, 한 번의 휴식에서 대상이 갈린다. 0번 선택에는 "v1 싱글/리슨 호스트 전제" 주석이 있다. 협동 체크포인트는 원래 제한적이라 P3로 둔다(2차 검증에서 P2→P3).
- **수정**: Owner를 AWxDevice로 캐스트해 `GetInteractingCharacter()`의 컨트롤러 인벤토리를 쓴다. 또는 `Actor.InteractingCharacter`에 바인딩하는 입력 필드를 둔다. 헤더·cpp의 0번 전제 주석도 고친다.

#### D38 [P3] 퀘스트 트리의 GiveRewards가 픽업 보상을 월드 원점 근처에 스폰한다
- **원인**: 퀘스트 StateTree의 Owner는 AWxGameState(AInfo)라 트랜스폼이 원점이다([WxStateTreeTask_GiveRewards.cpp:37](../../Source/WxGame/Inventory/WxStateTreeTask_GiveRewards.cpp#L37)). 현재 DT_Reward에는 픽업 아이템이 없어 잠재 상태다.
- **수정**: 오너가 AWxDevice면 오너 트랜스폼을, 아니면 직접 지급 대상 PC의 폰 트랜스폼을 쓴다. "루트 컴포넌트가 있는가"로 판정하면 안 된다. AInfo는 PIE에서는 에디터 전용 Sprite 루트가 있고 쿠킹 빌드에서는 없어 두 환경이 갈린다.

#### D39 [P3] DA_Potion 최대 충전이 3회로 기획(기본·최대 4회)과 다르다
- C++ 기본값 3([WxItemFragment.h:69](../../Source/WxGame/Inventory/WxItemFragment.h#L69))을 DA_Potion이 덮어쓰지 않는다. 충전 아이콘도 3회 기준이다. 3회로 정한 결정 기록은 없다.
- **수정**: 기획에 확인한다. 4회가 맞으면 DA_Potion의 MaxCharges를 4로, ChargeIcons를 5장으로 늘린다. 3회가 의도라면 기획서 수정을 기획자에게 제안한다.

#### D40 [P3] "목적지 이동" 퀘스트 목표가 볼륨 진입을 판정하지 못한다
- **원인**: [WaitMoveToTarget](../../Source/WxGame/Quest/WxStateTreeTask_WaitMoveToTarget.cpp#L49)은 대상 액터 원점과의 3D 거리만 AcceptRadius와 비교한다. 기획서(Wx_Quest) 1·3단계의 "지역 볼륨 진입"을 저작할 방법이 없다. 현재 퀘스트 에셋에는 볼륨 목표가 없다.
- **수정**: 대상이 AVolume이면 `EncompassesPoint`로 판정하고, 그 밖에는 기존 반경을 유지한다. AcceptRadius 주석에 한 줄 덧붙인다.

#### D41 [P3] MarkIndicator의 TargetLocation 자동 기록이 실제 사용처에서는 동작하지 않는다
- **원인**: 자동 기록은 태스크 인스턴스 데이터를 직접 편집할 때만 걸린다([WxStateTreeTask_MarkIndicator.h:38](../../Source/WxGame/UI/IndicatorSystem/WxStateTreeTask_MarkIndicator.h#L38)). 퀘스트 스텝(ST_Quest_Main1/2)은 루트 파라미터에 바인딩해서 쓴다.
- **영향**: Target Location을 비워 두면, 대상 셀이 언로드된 원거리(마커가 가장 필요한 상황)에서 인디케이터가 월드 원점을 가리킨다. 대상을 옮기면 옛 좌표를 가리킨다.
- **수정**: 헤더(37~38행)와 EnterState(cpp 33행) 주석을 실제 동작에 맞춘다. 자동 기록이 꼭 필요하면 `FWxActorLocatorCustomization::SetLocatorToActor`에서 형제 TargetLocation을 채우는 쪽을 검토한다.

#### D42 [P3] 스포너 에디터 프리뷰의 자식 BP_Soldier가 레벨 패키지 2곳에 저장돼 있다
- **원인**: 프리뷰 ChildActorComponent를 `RF_Transient`만으로 만든다([WxSpawner.cpp:211](../../Source/WxGame/Spawner/WxSpawner.cpp#L211)). 엔진의 T3D 내보내기(복사·붙여넣기 등)는 `RF_Transient` 내부 객체를 거르지 않는다.
- **현재 오염**: 스포너 패키지 23개 중 Spawner_BP_Soldier9(LV_OpenWorld), Spawner_BP_Soldier11(SiegeCannonEmplacement01) 2개.
- **영향**: 에디터에서 프리뷰 병사가 겹친다. PIE에서는 저장된 편집기 전용 BP_Soldier가 로드되어, 스포너가 추적하지 않는 적이 하나 더 생길 수 있다(인게임 미확인). 쿠킹 빌드에서는 제거된다.
- **수정**: 생성 플래그를 `RF_Transient | RF_TextExportTransient | RF_DuplicateTransient`로 바꾼다(엔진의 자식 액터 처리와 같다). 오염된 두 패키지는 프리뷰 컴포넌트·자식 액터를 걷어내고 다시 저장한다. 2차 검증에서 P2에서 P3으로 낮췄다.

#### D43 [P3] WxTeleport 치트가 스트리밍 완료를 기다리지 않는다
- [WxCheatManager.cpp:24](../../Source/WxGame/Development/WxCheatManager.cpp#L24). World Partition 맵에서 미로딩 셀의 지면 근처로 이동하면 지형 아래로 떨어질 수 있다. 부활·맵 도착 경로는 `UpdateCamera` 뒤 `BlockTillLevelStreamingCompleted`로 기다린다.
- **수정**: 같은 순서로 기다린다. 비동기 스트리밍을 관찰하려는 의도라면 주석으로 남긴다.

### 네트워크·UI

#### D44 [P3] 원격 클라가 RTT 안에 락온 대상을 연달아 바꾸면 대상이 잠깐 되돌아간다
- **원인**: `LockOnTarget`이 조건 없이 소유 클라에게도 복제된다([WxLockOnComponent.cpp:17](../../Source/WxGame/Targeting/WxLockOnComponent.cpp#L17)). 대상을 고르는 주인은 소유 클라인데, 서버가 지난 요청 값을 돌려보내 로컬 값을 덮는다.
- **영향**: 카메라가 이전 대상 쪽으로 잠깐 꺾이고, 해제 직후 레티클이 잠깐 다시 나타난다. 그 사이 공격하면 SnapToTarget 워프가 클라와 서버에서 달라 위치 보정이 생길 수 있다.
- **수정**: `DOREPLIFETIME_CONDITION(..., COND_SkipOwner)` 한 줄로 바꾼다. cpp 39행과 헤더 18~22·47~48행 주석(소유자 복제 도착 통지)을 함께 고친다.

#### D45 [P3] 원격 클라에서는 어빌리티 부여·제거 뒤 HUD 슬롯이 즉시 다시 매칭되지 않는다
- **원인**: 엔진은 `AbilitySpecDirtiedCallbacks`를 권위 머신에서만 방송한다([WxViewModel_AbilitySystem.cpp:16](../../Source/WxGame/UI/MVVM/WxViewModel_AbilitySystem.cpp#L16)). 원격 클라는 다음 소유 태그 변경 때 맞춰진다. 현재 콘텐츠는 빙의할 때 한 번만 부여하므로 잠재 상태다.
- **수정**: 지금은 헤더 25행, cpp 207·226행 주석을 사실대로 고친다. 런타임 스킬 교체를 도입할 때 `OnGiveAbility`/`OnRemoveAbility` 재정의로 비권위에서도 방송하는 안을 검토한다. 엔진의 "권위 전용" 의미론을 벗어나므로, 그때 이탈 지점으로 명시한다.

## 도구 결함

게임 런타임에는 영향이 없다. 영향 칸은 그 도구를 쓰는 작업에서 생기는 결과다.

| ID | 위치 | 문제와 영향 | 수정 방향 |
|---|---|---|---|
| T01 | [Check-Redirects.ps1:386](../../.agents/scripts/Check-Redirects.ps1#L386) | 패키지 헤더만 읽어, export 데이터 안의 문자열 경로(MVVM 변환 함수 인자, StateTree 파라미터 메타)가 남아 있어도 SAFE로 보고한다. 지우면 다음 컴파일 때 바인딩이 조용히 풀린다. 10-01 모듈 합치기 때 사람이 따로 찾아 고친 그 함정이다. | Package/Class/Struct/Enum 종류의 옛 경로는 파일 전체 바이트에서도 찾아 REVIEW로 분류 |
| T02 | [Check-Redirects.ps1:431](../../.agents/scripts/Check-Redirects.ps1#L431) | GameplayTag 텍스트 참조를 따옴표 붙은 문자열로만 찾아, ini의 `ActionTag=...`를 놓친다. 현재 GameplayTagRedirects가 없어 잠재 상태. | ini는 토큰 경계 정규식으로도 검색 |
| T03 | [DataTableRowReferenceUpdater.cpp:82](../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.cpp#L82) | 해제된 행의 메모리 주소를 비교해, 할당기가 주소를 재사용하면 행 교체를 이름 변경으로 오인한다. 참조가 엉뚱한 행으로 바뀔 수 있다(드묾). | 이름이 유지된 행도 포인터 동일성 확인 |
| T04 | [Export-AbilitySystemLists.ps1:419](../../.agents/scripts/Export-AbilitySystemLists.ps1#L419) | float을 `[decimal]`로 바꿔, 7.9e28을 넘는 값이나 NaN이 하나라도 있으면 목록 3개 생성이 모두 중단된다. | 범위 안 유한값만 decimal, 나머지는 `ToString('R')` |
| T05 | [Invoke-WikiLint.ps1:37](../../.agents/skills/wiki-lint/scripts/Invoke-WikiLint.ps1#L37) | 확장자 없는 에셋 출처 경로는 git log 경로와 맞지 않아 낡음을 항상 0건으로 센다. 현재 위키에는 그런 출처 줄이 없다. | 실제 파일 경로(.uasset)로 git log 실행 |
| T06 | [WxMVVMToolset.cpp:112](../../Plugins/WxToolset/Source/WxToolset/Private/WxMVVMToolset.cpp#L112) | `SetBindingConversionFunction`이 반환 타입 불일치로 실패하면 기존 변환 함수와 소스 경로를 지운 채 false를 반환한다. 그대로 저장하면 UI 값이 갱신되지 않는다. | `IsValidConversionFunction`으로 사전 검사 |
| T07 | [WxStateTreeToolset.cpp:178](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L178) | `CreateStateTree`가 중복 이름·잘못된 경로를 받으면 AssetTools 모달을 띄워 MCP 호출이 멈춘다. 덮어쓰기를 누르면 기존 StateTree가 빈 에셋으로 바뀐다. | 경로 유효성·존재 여부를 확인하고 `RaiseScriptError` |
| T08 | [WxStateTreeToolset.cpp:521](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L521) | `ClearStateParameterOverride`를 링크 상태가 아닌 곳에 부르면 파라미터 전체를 지우고 true를 반환한다. | Linked/LinkedAsset과 대상이 있는지 확인 |
| T09 | [WxStateTreeToolset.cpp:255](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L255) | `AddRootParameter`가 빈 값 타입, Type=None, 정리되는 이름을 받으면 쓸 수 없는 파라미터를 만들거나 빈 ID를 오류 없이 반환한다. 재시도하면 중복이 생긴다. | 해당 입력을 거부 |
| T10 | [WxLandscapeToolset.cpp:124](../../Plugins/WxToolset/Source/WxToolset/Private/WxLandscapeToolset.cpp#L124) | 새로 만드는 LayerInfo의 BlendMethod가 None이라 페인트 레이어가 정규화되지 않는다. | 새로 만든 LayerInfo에 `SetBlendMethod(FinalWeightBlending)` |
| T11 | [WxAnimMontageToolset.cpp:658](../../Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.cpp#L658) | `AddNotify`가 추상 노티파이 클래스를 거르지 않아, 저장 시 null로 지워질 노티파이를 만들고 true를 반환한다. | `CLASS_Abstract`면 오류 |
| T12 | [WxEditor.cpp:83](../../Source/WxEditor/WxEditor.cpp#L83) | Blueprint 썸네일 렌더러를 교체하면서 항목이 목록 끝으로 가, AnimBlueprint·WidgetBlueprint 전용 썸네일이 가려진다. | 교체 직후 두 항목을 다시 등록해 순서 복원(UMGEditor 의존 추가), ShutdownModule 복원 순서도 맞춤 |

## Q. 품질 개선

결함은 아니지만 근거가 확인된 정리 항목이다. 대부분 몇 줄 수정이다.

### 사용자 코드 규칙

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q01 | 아래 목록 | `.cpp`의 익명 namespace 12곳과 static 자유 함수 6개. 메모리 규칙("어떤 경우에도 `.cpp` 최상단에 namespace 블록을 만들지 않는다")과 어긋난다. | 상수는 사용처 지역 상수나 클래스 static 멤버로, 한 곳에서 쓰는 헬퍼는 호출부에 인라인, 여러 곳에서 쓰면 private 멤버로 옮긴다. |
| Q02 | [WxIndicator.cpp:110](../../Source/WxGame/UI/IndicatorSystem/WxIndicator.cpp#L110) | UIManager 서브시스템을 직접 꺼내 `IsMenuLayerActive`를 부른다(UI 호출부는 UWxUILibrary 파사드 사용 규칙). | UWxUILibrary에 `IsMenuLayerActive` 파사드 추가 |
| Q03 | [WxGameFlowSubsystem.cpp:41](../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp#L41) 외 | 프론트엔드 StatusText 기본 문구 6개가 한국어다(인게임 문구는 영어 규칙). | LOCTEXT 키는 유지하고 기본 문구를 영어로 |

Q01 위치:
- 익명 namespace(WxGame): [WxAbility_Finisher.cpp:16](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Finisher.cpp#L16), [WxHitStopComponent.cpp:11](../../Source/WxGame/AbilitySystem/WxHitStopComponent.cpp#L11), [WxAnimNotifyState_SnapToTarget.cpp:13](../../Source/WxGame/Animation/WxAnimNotifyState_SnapToTarget.cpp#L13), [WxAnimNotify_AbilityEvent.cpp:9](../../Source/WxGame/Animation/WxAnimNotify_AbilityEvent.cpp#L9), [WxSkillCutsceneComponent.cpp:26](../../Source/WxGame/Combat/WxSkillCutsceneComponent.cpp#L26)(클래스 `FWxSkillCutsceneClock`), [WxDeviceStateTreeComponent.cpp:12](../../Source/WxGame/Device/WxDeviceStateTreeComponent.cpp#L12), [WxStateTreeTask_WaitForInteraction.cpp:11](../../Source/WxGame/Interaction/WxStateTreeTask_WaitForInteraction.cpp#L11), [WxStateTreeTask_WaitSpawnersKilled.cpp:11](../../Source/WxGame/Spawner/WxStateTreeTask_WaitSpawnersKilled.cpp#L11)
- 익명 namespace(WxToolset): [WxAnimMontageToolset.cpp:14](../../Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.cpp#L14), [WxBlueprintToolset.cpp:14](../../Plugins/WxToolset/Source/WxToolset/Private/WxBlueprintToolset.cpp#L14), [WxLandscapeToolset.cpp:21](../../Plugins/WxToolset/Source/WxToolset/Private/WxLandscapeToolset.cpp#L21), [WxStateTreeToolset.cpp:26](../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp#L26)
- static 자유 함수: [WxEffect_Damage.cpp:60~92](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp#L60)(`GetDamageStatics`와 `Calculate*` 5개). `GetDamageStatics`는 GAS ExecCalc의 관용 패턴(Lyra 동일)이라 예외로 둘지 결정이 필요하다.
- WaitForInteraction의 namespace에는 전역 대기 등록부와 핸들 카운터가 들어 있어, 정적 상태의 소유자가 코드에 드러나지 않는다. 태스크 클래스의 private static 멤버로 옮기는 것을 제안한다.

### 죽은 코드·무효 코드

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q04 | [WxEffect_Exceed.h:9](../../Source/WxGame/AbilitySystem/Effects/WxEffect_Exceed.h#L9), [WxCueNotify_Exceed.cpp:16](../../Source/WxGame/AbilitySystem/Cues/WxCueNotify_Exceed.cpp#L16) | Exceed GE는 Abstract이고 파생 에셋·호출부가 없다. GC_Exceed도 이 GE로만 발행된다. | 재사용 계획이 없으면 UWxEffect_Exceed·AWxCueNotify_Exceed·GC_Exceed·`GameplayCue.Exceed`를 삭제 목록으로 정리 |
| Q05 | [WxViewModel_Ability.cpp:28](../../Source/WxGame/UI/MVVM/WxViewModel_Ability.cpp#L28) | ActionPhaseChanged 이벤트 구독·발행이 어떤 표시 값에도 영향을 주지 않는다(배타 점유 판정 제거 뒤 잔재). 어빌리티 베이스가 UI만을 위해 단계마다 이벤트를 낸다. | 구독, `SetActionPhase`의 발행, `Event.Ability.ActionPhaseChanged` 태그를 함께 제거 |
| Q06 | [WxInventoryComponent.h:177](../../Source/WxGame/Inventory/WxInventoryComponent.h#L177) | "UI 진입점"이라는 `RequestUseConsumable`에 호출자가 없다. `RemoveItemInstance`·`RemoveEntry`도 없다. | 제거하고 UseItem 헤더 주석을 현재 경로에 맞춤 |
| Q07 | [WxAsyncAction_PushWidgetToLayer.h:35](../../Source/WxGame/UI/Foundation/WxAsyncAction_PushWidgetToLayer.h#L35) | `SetBeforePushCallback`·`BeforePushCallback` 경로에 호출자가 없다. | 삭제(BP용 BeforePush 델리게이트는 유지) |
| Q08 | [WxEffect_HitStop.cpp:31](../../Source/WxGame/AbilitySystem/Effects/WxEffect_HitStop.cpp#L31) | `GetAnimatingAbility`·`Context.SetAbility` 결과를 읽는 곳이 없다(예측 키 제거 뒤 잔재). | `MakeEffectContext()`만 남기고 include 정리 |
| Q09 | [WxAbilityTask_SlowTime.h:12](../../Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_SlowTime.h#L12) | Duration 인자·타이머 분기를 아무도 쓰지 않고, 클래스 주석이 현재 사용 방식(구간 종료 시 EndTask)과 다르다. | 분기 제거, 주석 정정 |
| Q10 | [WxStateTreeTask_SpawnNiagara.cpp:25](../../Source/WxGame/Device/WxStateTreeTask_SpawnNiagara.cpp#L25) | "살아 있으면 통과" 분기에 도달할 수 없다. 재선택 때 ExitState가 먼저 컴포넌트를 파괴해, 루프 FX를 지웠다가 다시 만든다. 메모리의 재진입 설계와 어긋난다. | ExitState 첫 줄에 Sustained면 return(설계 복원). 재생성이 의도라면 분기를 지우고 주석 정정 |
| Q11 | [WxEnemyCharacter.cpp:40](../../Source/WxGame/Character/WxEnemyCharacter.cpp#L40) | `SetReplicationMode(Full)`은 엔진 기본값을 다시 쓸 뿐이다. | 줄 삭제 |

### 같은 규칙의 중복

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q12 | [WxAbilitySystemComponent.cpp:301](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L301) | ASPD 하한 0.001이 AttributeSet 클램프와 `GetMontagePlayRate` 두 곳에 있다. | `GetMontagePlayRate`의 Max 제거 |
| Q13 | [WxAbilitySet.cpp:61](../../Source/WxGame/AbilitySystem/WxAbilitySet.cpp#L61) | 어빌리티 세트 부여 경로의 권위 검사가 한 호출 사슬에서 네 번 반복된다. | `GiveAbilitySets` 한 곳만 남김 |
| Q14 | [WxAbility_Groggy.cpp:26](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp#L26) | `ActivationBlockedTags(Ability.Death)`가 Death의 `BlockAbilitiesWithTag(Ability)`와 같은 차단이다. | 삭제(거는 쪽이 막는 HitReact 규약과 맞춤) |
| Q15 | [WxAnimNotifyState_Rush.h:19](../../Source/WxGame/Animation/WxAnimNotifyState_Rush.h#L19) | AbilityEvent 신호 규칙을 `SendSignal`로 복제했다. 베이스를 고치면(D13 플래그 등) Rush만 빠진다. | AbilityEvent 파생으로 변경(WeaponAttack 선례) |
| Q16 | [WxAbility_Interact.h:34](../../Source/WxGame/Interaction/WxAbility_Interact.h#L34) | 상호작용 반경(150)이 스캐너와 어빌리티 두 곳에 있다. "변조 방지" 근거 주석은 성립하지 않는다(서버 PC에도 스캐너 기본값 사본이 있음). 한쪽만 바꾸면 프롬프트는 뜨는데 서버가 거절한다. | 어빌리티가 서버 PC 스캐너 반경을 읽음 |
| Q17 | [WxViewModel_Subtitle.cpp:14](../../Source/WxGame/UI/MVVM/WxViewModel_Subtitle.cpp#L14) | `GetOrCreate`가 `WxViewModel::GetGlobalCollection`과 같은 조회를 다시 구현했다. | 유틸 호출 한 줄로 교체 |

### 구조·계약

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q18 | [WxCharacterBase.cpp:166](../../Source/WxGame/Character/WxCharacterBase.cpp#L166) | `IsAlive()`가 HP만 본다. `DespawnMinions`는 HP를 둔 채 `Event.Death`로 죽이므로, 그 경로가 쓰이면 시체에 명판이 남는다(현재 도달 없음). | `HP > 0 && !Ability.Death` |
| Q19 | [WxDamageEffectContext.cpp:34](../../Source/WxGame/AbilitySystem/WxDamageEffectContext.cpp#L34) | `Get`이 const 핸들을 const_cast해 가변 포인터를 돌려주고, `MakeDamageSpec`이 const 참조로 컨텍스트를 수정한다. | const/비const 오버로드 분리 |
| Q20 | [WxAbilityBase.h:91](../../Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.h#L91) | `ActivationOwnedEffects`(Infinite 효과 계약)에 Instant GE(GE_HGTest_Attack_Heavy_2·Skill_2)가 들어 있다. 커밋 전에 적용돼, 실패한 발동에도 UP 지급이 남을 수 있다. | Instant 사용을 계약에 명시하거나, 지급을 비용 커밋 경로로 옮김 |
| Q21 | [WxBTDecorator_ObserveAbility.cpp:35](../../Source/WxGame/AI/WxBTDecorator_ObserveAbility.cpp#L35) | "노드 인스턴스에는 키 ID가 없다"는 주석이 엔진 동작과 달라, 키를 매번 이름으로 조회한다. MirrorMovement도 같다. | `GetSelectedKeyID()`로 읽기 |
| Q22 | [WxBTDecorator_BeyondLeash.cpp:47](../../Source/WxGame/AI/WxBTDecorator_BeyondLeash.cpp#L47) | BeyondLeash·AttributeRatio의 `GetStaticDescription`이 Super를 빼, BT 그래프에 abort 모드·inversed 표시가 안 나온다. | ObserveAbility처럼 Super 결과를 앞에 붙임 |

### 코드와 어긋난 주석

| ID | 위치 | 어긋난 내용 |
|---|---|---|
| Q23 | [WxAbilityTargetData_Direction.h:9](../../Source/WxGame/AbilitySystem/WxAbilityTargetData_Direction.h#L9) | "클라 입력 방향을 서버로 전송"뿐 아니라 서버→소유 클라 피격 방향 전달에도 쓴다 |
| Q24 | [WxCombatAttributeSet.h:107](../../Source/WxGame/AbilitySystem/Attributes/WxCombatAttributeSet.h#L107) | ASPD는 어빌리티 몽타주 전반이 아니라 UWxAbility_Combo 계열만 쓴다 |
| Q25 | [WxAbilitySystemComponent.cpp:199](../../Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp#L199) | "사본을 피한다"는 근거가 바로 앞 엔진 호출(이미 사본 생성) 때문에 성립하지 않는다 |
| Q26 | [WxAbility_Pattern.cpp:15](../../Source/WxGame/AbilitySystem/Abilities/WxAbility_Pattern.cpp#L15) | 패턴은 그로기·사망뿐 아니라 넉 계열 피격에도 끊긴다 |
| Q27 | [WxCombatLibrary.h:36](../../Source/WxGame/Combat/WxCombatLibrary.h#L36) | `ApplyEffect`의 소유 클라 예측 적용은 일어나지 않는다(서버 권위 적용 후 복제) |
| Q28 | [WxAIController.cpp:80](../../Source/WxGame/AI/WxAIController.cpp#L80) | Damage 센스에는 피아 필터가 없다(적대 판정은 `HandlePawnHit`가 담당) |
| Q29 | [WxEffect_HitStop.h:12](../../Source/WxGame/AbilitySystem/Effects/WxEffect_HitStop.h#L12) | "역경직"이 히트스톱과 패리 리액션을 함께 가리킨다. `bCanParry`=false여도 공격자 히트스톱은 걸리므로 툴팁 정정 |
| Q30 | [WxProjectileBase.h:56](../../Source/WxGame/Weapons/WxProjectileBase.h#L56) | IGenericTeamAgentInterface 구획 주석 안에 AActor 재정의 `PostNetReceiveVelocity`가 있다 |
| Q31 | [WxMinionComponent.h:41](../../Source/WxGame/Minion/WxMinionComponent.h#L41) | 사망 어빌리티가 없으면 즉시 파괴한다는 설명이 빠졌다(현재 유일한 사용처가 이 경로) |
| Q32 | [WxCheatManager.h:14](../../Source/WxGame/Development/WxCheatManager.h#L14) | "존재하면 곧 권위 측"이 EnableCheats에서 깨진다(클라에도 생기며 그때 치트가 조용히 실패) |
| Q33 | [WxDialogueSessionComponent.cpp:361](../../Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp#L361) | 단일 노드 모드에도 AnimInstance가 있다(null 조건은 대상·메시·애님 클래스 없음) |
| Q34 | [WxItemFragment.h:31](../../Source/WxGame/Inventory/WxItemFragment.h#L31) | Fragment는 기능 축만이 아니다. Grade·Pickup은 표시·외형 데이터다(아이템 속성은 Fragment 컴포지션 원칙) |
| Q35 | [WxTargetingSorterTask_InputDirection.h:13](../../Source/WxGame/Targeting/WxTargetingSorterTask_InputDirection.h#L13) | "머신 간 판정 일치"는 서버와 소유 클라 사이에서만 맞다(시뮬레이티드 프록시는 속도 방향) |

### 데이터·설정 정리

| ID | 위치 | 내용 | 제안 |
|---|---|---|---|
| Q36 | LV_OpenWorld·LV_DevCombat의 Spawner_BP_Boss | `e0e3ecc51`(09-09, 캐릭터 폴더 정리)에서 BP_Boss가 삭제된 뒤 SpawnableActorClass가 빈 스포너 2개가 남았다. `Character.Boss`를 가진 캐릭터도 없어 보스 체력 바 경로가 쓰이지 않는다(보스 콘텐츠 부재이지 코드 결함은 아님). | 보스 BP를 다시 만들 때 IdentityTags에 `Character.Boss`를 넣고 연결. 그 전까지는 빈 스포너를 레벨에서 삭제 |
| Q37 | [ST_Quest_Main1](../../Content/Quest/ST_Quest_Main1.uasset)·Main2 | 실패 전이가 등록되지 않은 태그 `Quest.Fail`을 기다린다. 발동할 수 없고, 로드할 때마다 잘못된 태그 경고가 난다. | 실패 조건이 없다면 전이 제거 |
| Q38 | SiegeCannonEmplacement01 레벨 인스턴스 | 장치 6개가 폐기된 ST 파라미터 오버라이드 `TriggerEvent=Event.Device.Triggered`를 들고 있다. 동작 영향은 없고 경고만 쌓인다. | 레벨 인스턴스를 열어 장치 ST 파라미터 동기화 후 재저장 |
| Q39 | BP_Soldier_Projectile, AM_Minion_Attack_Heavy, AM_HGTest_Attack_DodgeCounter | 어디서도 참조되지 않는 에셋 3개가 DT_Damage에 없는 행을 가리킨다. 되살려 쓰면 피해가 없는 공격이 된다. | 삭제 목록으로 정리(삭제는 사용자 확인 후) |
| Q40 | [DefaultEngine.ini:44](../../Config/DefaultEngine.ini#L44) | `WxCharacterMesh` 콜리전 프로파일을 아무도 쓰지 않는다. `WxAttack=Block`도 [WxCollisionChannels.h](../../Source/WxGame/WxCollisionChannels.h)의 규칙(캐릭터 메시는 Overlap)과 반대다. | 프로파일 줄 삭제 |

## X. 기각·보류된 후보

검증 단계에서 결함이 아니라고 판정된 후보다. 같은 지적이 반복되지 않도록 사유를 남긴다.

| ID | 후보 | 기각 사유 |
|---|---|---|
| X01 | 청각 자극만으로 즉시 TargetActor를 지정한다(기획: 청각은 즉시 인식 안 함) | `95a5a24a3`(07-17)에서 청각을 완전 획득 센스로 승격한 의도된 결정이다. 기획서보다 나중 결정이다. |
| X02 | 리슨 서버에서는 사망 후 부활 경로가 없다 | 새 게임·체크포인트·부활 흐름 전체가 싱글플레이 범위로 설계됐다. 결함이 아니라 미구현 범위다. |
| X03 | 보물상자·체크포인트 장치 상태가 셀 재로드 때 초기화된다 | `b3f982b0d`(WxSave 삭제)에서 "셀 재진입 시 처치·장치 상태 유지"를 버리고 세이브 재도입 때 다시 설계하기로 했다. 1차 확정 뒤 2차·3차가 기각했다. |
| X04 | 처치된 `bNeverRevive` 스포너가 셀 재로드 뒤 다시 스폰한다 | X03과 같은 결정이다. 현재 해당 배치는 대상 클래스가 빈 스포너뿐이다. 1차 확정 뒤 2차·3차가 기각했다. |
| X05 | LV_OpenWorld 보스 스포너의 `bNeverRevive`가 꺼져 있다 | 그 스포너에는 스폰 대상 클래스 자체가 없다. 정리 항목은 Q36에서 다룬다. |
| X06 | 퀘스트 "NPC 대화" 단계가 대화 종료가 아니라 상호작용 순간에 완료된다 | `e771a418b`(08-14, 대화 완주 게이트를 상호작용 대기 게이트로 교체)의 의도된 결정이다. |
| X07 | 퀘스트·대화가 호스트 전용이라 원격 클라는 퀘스트 NPC를 쓸 수 없다 | "v1 싱글/리슨 호스트 전제"가 코드에 명시된 범위 제한이다. |
| X08 | NPC 대사 포즈가 대화한 머신에서만 재생된다 | X07 전제 안의 부수 증상이다. 게임 상태가 아닌 연출이고, 복제 장치 신설은 사용자 원칙에 맞지 않는다. |
| X09 | 보물상자의 골드가 0번 PC에게 들어간다 | 적 처치 보상과 같은, 문서화된 보상 정책이다. 리필(D37)과 달리 플레이어별 전투 자원이 아니다. |
| X10 | `LinkedDevices`에 빈 슬롯이 있으면 클라에서 버튼이 열려 보인다 | 사용자가 확정한 정책("클라에 미로드된 연결 장치는 열어 두고 서버가 다시 검증")의 범위 안이다. |
| X11 | TargetingPreset이 빈 이동 스냅은 범위 판정 없이 워프한다 | 이전부터 유지된 명시적 의미다. SnapToTarget을 쓰는 몽타주 51개가 모두 프리셋을 지정해 도달하지 않는다. |
| X12 | Character VM이 자식 VM 구독을 해제하지 않는다 | 09-29 VM 베이스 제거 때 약참조·UObject 타이머 전제로 파괴 정리를 없앤 결정이다. 화면 영향이 없고 누적되지 않는다. |
| X13 | Dodge 판정 캡슐을 고정 이름으로 만들어 재부여 시 덮어쓴다 | 엔진이 같은 이름 객체를 교체하기 전에 컴포넌트 등록을 정리한다. 누적되지 않는다. |
| X14 | 예측 클라가 상대의 시뮬레이티드 Rush 태스크를 찾지 못한다 | 주장된 목적지 차이가 일어나지 않는다. 제안된 수정도 효과가 없다(필요한 필드가 복제되지 않음). |
| X15 | 디스크 체크포인트가 New Game에서만 지워진다 | PIE·일반 플레이가 슬롯을 공유하고 실행 간 유지되는 것이 의도다. |
| X16 | 콤보 입력이 `InternalTryActivateAbility`를 직접 부른다 | 이유가 주석에 있다. 가상의 정책 변경에 대비한 방어 게이트는 엔진 판정 중복 금지 원칙과 충돌한다. |

**판정 보류(실행 없이 판단 불가)**
- [BoxComponentVisualizer.cpp:142](../../Plugins/BoxComponentVisualizer/Source/BoxComponentVisualizerEditor/Private/BoxComponentVisualizer.cpp#L142): BP 에디터에서 SCS 루트 UBoxComponent의 모서리를 끌면 "반대쪽 면 고정"이 배치 인스턴스에 남지 않는다는 후보. 프로젝트에 해당 구성의 BP가 확인되지 않는다.
- [WxIndicator.cpp:109](../../Source/WxGame/UI/IndicatorSystem/WxIndicator.cpp#L109): "메뉴가 스크린 스페이스 위젯을 덮어 주지 못한다"는 주석이 엔진 레이어 순서와 다르다는 후보. 엔진 레이어 순서를 확인하지 못했다.
