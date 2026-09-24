# 어빌리티 테이블 구동 전환

상태: 구현 중. 1-1단계는 구현했고 사용자가 플레이로 확인했다(2026-09-24). 1-2단계(몽타주 섹션 모델)를 진행한다.

- 날짜: 2026-09-24
- 요청: GA_ 에셋을 하나하나 만드는 방식을 폐기하고 어빌리티를 테이블로 완전 구동한다. 사용자 초안 컬럼: 어빌리티 이름, 어빌리티 타입, 발동 타입, 애니메이션 몽타주, 키입력, 코스트 종류, 코스트 양, 쿨타임, 쿨타임 스택, 아이콘, 표기명, 설명문.
- 기준: HEAD `142fab5d6`. 조사 방법은 에디터 MCP 읽기(GA_ 39개, `DT_Ability` 21행, ABS_ 9개, 몽타주 45개)와 UE 5.8 GAS·애니메이션 소스 대조다. 빌드·플레이 검증은 하지 않았다.

## 원칙

- 타입(C++ 클래스)은 시스템 규칙이다. 어빌리티 태그, 취소·차단 관계, 발동 그룹, 입력 방식(홀드·토글), 캐릭터 공통 발동 조건, 네트워크 정책이 여기에 속한다.
- 행은 콘텐츠다. 몽타주, 수치, 표시, 입력 IA, 캐릭터 상태 조건, 쿨다운 그룹이 여기에 속한다.
- 이 구분은 `Docs/Meeting/어빌리티 규칙 브리핑.md` 2-2("어빌리티 태그는 시스템 규칙, 기획 커스터마이징 불허")와 같다. GAS가 태그·관계·재발동을 CDO에서만 읽는다는 제약과도 맞다.

## 확정한 결정

### 테이블 구성

- 액션 어빌리티 테이블은 행 구조체가 하나다. 에셋은 공용 1장(`DT_Ability_Shared`)과 캐릭터별 테이블(`DT_Ability_<캐릭터>`, 소환물 행 포함)로 나눈다. 몽타주를 하드 참조하므로 테이블 단위가 곧 로드 단위다.
  - 플레이어/적 분리를 하지 않는 이유는 세 가지다.
    - `GA_Shared_Death`·`GA_Shared_HitReact`를 공용 플레이어 세트와 공용 적 세트가 같이 부여한다.
    - 분신·도플갱어는 HGTest 몽타주가 하드 참조로 소환한다.
    - 적 테이블을 한 장으로 두면 적 하나만 등장해도 모든 적의 패턴이 로드된다.
- 패시브는 별도 테이블 `DT_Passive` 한 장에 둔다(아래 패시브 테이블).
- 부여는 지금처럼 세트(ABS_)가 한다. 세트에는 액션 어빌리티 행 목록과 패시브 행 목록을 따로 둔다. 같은 입력에 후보가 여럿이면 세트 순서대로 시도하고 처음 성공한 것을 쓴다.
- 행 ID는 두 테이블을 통틀어 전역 고유다. 소유 접두사를 붙인다(예: `AbilityId.Shared.Dodge`, `AbilityId.HGTest.Skill1`).
- 편집은 UE 에디터에서 한다.
- 타입 데이터 구조체 칸은 두지 않는다. 특정 타입만 쓰는 컬럼도 두지 않는다.

### 액션 어빌리티 컬럼

| 컬럼 | 형식 | 비고 |
|---|---|---|
| ID | 행 이름(= 행 ID 태그) | 영문이며 바꾸지 않는다고 전제한다. `GA_` 접두사는 없앤다 |
| 타입 | 코드가 정한 목록 | 아래 타입 목록 |
| 입력 | InputAction | 물리 키는 IMC가 정한다. AI·이벤트 전용이면 비운다 |
| 발동 조건 | `FGameplayTagRequirements` | 캐릭터 상태 범주(`Master.*` 등)만 허용한다 |
| 코스트 자원 / 양 | 기존 | Custom은 테이블 비용이 없다는 뜻이다. 콤보는 단마다 소모한다 |
| 쿨다운 시간 | 초 | 0이면 쿨다운이 없다 |
| 충전 수 | 정수 | 한 칸씩 직렬로 회복한다 |
| 쿨다운 그룹 | Dodge, Skill_1~4, Ultimate | 그룹마다 GE 클래스가 하나다(엔진이 스택을 GE 클래스 단위로 합친다) |
| 활성 중 효과 | GE 목록 | 어빌리티가 끝나면 걷는다 |
| 몽타주 | 1개 | 변형은 섹션으로 나눈다 |
| 아이콘 / 표기명 / 설명 | 기존 | 영어 원문, FText 현지화 |

- 초안의 "발동 타입"은 뺐다. 발동 그룹이든 입력 방식이든 타입이 정하는 값이기 때문이다.
- 피해 수치는 이 테이블에 두지 않는다. 기존처럼 노티파이·투사체가 `DT_Damage` 행을 고른다.
- 행 단위 트리거와 발동 시 효과는 이 테이블에 없다.
  - 반응 트리거는 클래스에 있다.
  - 액션 어빌리티 사이의 이벤트 연계는 금지다(브리핑 2-5).
  - 순간 효과는 시점형 노티파이로 건다.

### 패시브 테이블

| 컬럼 | 형식 | 비고 |
|---|---|---|
| ID | 행 이름(= 행 ID 태그) | 액션 어빌리티와 같은 전역 고유 규칙 |
| 트리거 이벤트 | 태그 목록 | 서로 조상 관계인 태그는 함께 넣지 못한다(한 이벤트에 두 번 발동) |
| 발동 조건 | `FGameplayTagRequirements` | 예: 도플갱어가 있을 때만 |
| 적용 효과 | GE 목록 | 어빌리티가 끝나도 남는다 |

- 행 하나가 `UWxAbility_Passive` 스펙 하나다. 트리거는 스펙별 `DynamicAbilityTriggers`로 등록한다.
- "공격 1회당 한 번만 지급" 같은 중복 방지 규칙은 지금처럼 Passive 클래스가 맡는다.
- 패시브 행을 읽는 곳은 부여와 Passive 클래스뿐이다. 무거운 에셋 참조가 없으므로 캐릭터별로 나누지 않고 한 장에 둔다.
- 이벤트 없이 늘 붙어 있는 스탯 보정은 행으로 만들지 않는다. 기존처럼 세트의 GrantedEffects에 둔다.
- 패시브에 쿨다운이나 UI 표시가 필요해지면 그때 이 테이블에 컬럼을 추가한다. 액션 테이블의 쿨다운 처리와는 별개다.

### 타입

- 공격은 규칙이 다른 만큼 나눈다.
  - Attack_Light
  - Attack_Heavy: Light를 취소한다
  - Attack_Air: `Movement.InAir`가 필요하다
  - Attack_DodgeCounter: `Ability.Dodge`가 필요하다
  - 캐릭터 공통 조건(공중·회피 중 금지 등)은 행이 아니라 타입 규칙이다. Light에 달린 `Ability.Death` 금지는 사망 어빌리티가 이미 `Ability` 전체를 막으므로 중복이다.
- 나머지 액션 타입은 기존 클래스와 1:1이다. Skill, Ultimate, Pattern, Dodge, Guard, GuardReact, HitReact, Death, Groggy, Finisher, LockOn, Sprint, Interact, UseItem.
- Passive는 패시브 테이블에서 부여한다.
- `PlayMontageOnce`는 행 없이 시스템이 부여한다.
- 분신 공격 3개(Minion Attack_Heavy·Skill_1·Skill_2)는 Override에서 Exclusive로 바꾼다. 플레이에서 확인할 점은 세 가지다.
  - 앞 동작의 본동작 중에는 발동이 거절된다.
  - 사망·BT 중단이 진행 중인 공격을 끊는다.
  - BT 중단 경고가 사라진다.
- 재발동: Attack과 Skill 모두 허용한다. 콤보 창에서 누르면 다음 단이다. 콤보 창이 닫힌 뒤 다시 누르면 후딜이든 아니든 1단부터 시작한다(사용자 확정). 창이 닫힐 때 단계를 초기화하는 방식으로 구현한다.
  - 평타도 바뀐다. 지금은 재발동이 단계를 유지해서 창이 닫힌 뒤 후딜에 눌러도 다음 단으로 이어진다.
  - HGTest 스킬 3개에만 걸려 있던 재발동 금지는 없앤다. 쿨다운이 없는 HGTest Skill_2는 후딜 중에 다시 누르면 재시작된다.

### 몽타주 섹션 규칙

- **콤보·패턴**: 몽타주 하나에 단계 섹션 `1`, `2`, …를 둔다.
  - 섹션 사이 링크는 끊는다.
  - 단계 전환은 같은 몽타주를 다음 섹션부터 새로 재생해서 교차 블렌드를 유지한다. 콤보는 재발동으로, 패턴은 블렌드아웃 때 다음 섹션을 재생한다.
  - 재생 중인 인스턴스 안에서 `JumpToSection`으로 단계를 넘기지 않는다.
  - 번호 섹션이 없는 몽타주는 처음부터 한 단계로 본다. 그래서 공중 공격(`Default`→`Loop`, `Grounded`)은 그대로 쓴다.
  - 블렌드 값은 몽타주마다 한 벌이다. 단계별로 다르게 두지 않는다(사용자 확인).
- **회피**: 기존 8방향 섹션 + `Backstep` + 극한 회피 `Success<방향>` 8개를 한 몽타주에 둔다. 이동 입력이 없으면 `Backstep`을 쓴다. 락온 중 회전은 현재 방식을 유지한다. 블렌드 인은 0.05초다.
- **가드 반응**: `GuardHit`, `GuardKnockback`, `GuardBreak`, `PerfectGuard`를 한 몽타주에 둔다. 블렌드 인은 0.05초다.
- **피격 반응**: 같은 HitReact 타입의 행 3개로 나눈다.
  - 넉 계열: `KnockBack`, `KnockDown`, `Parry`
  - 넉업: `KnockUp`(지금의 `Default`)→`Loop` 반복, `Grounded`
  - 일반 피격: 가산 슬롯, `Normal`
  - 반응 태그 이름과 같은 섹션이 자기 몽타주에 있는 행만 반응한다(`ShouldAbilityRespondToEvent`).
  - 일반 피격을 넉 계열과 합칠 수 없는 이유: 가산 슬롯 `AdditiveHitReact`는 다른 슬롯 그룹이고, 엔진은 한 몽타주의 슬롯이 모두 같은 그룹일 때만 재생한다.
  - 그로기 중 넉 계열을 일반 피격으로 낮추는 판단은 `UWxEffectComponent_DamageReaction`에서 한다.
  - 수용한 동작 변화: 일반 피격이 넉 계열을 끊지 않고 그 위에 겹친다. Normal 폴백이 사라진다.
- **착지**: `Grounded` 이름 방식을 유지한다. 넉업을 다른 넉 반응과 합치지 않는 이유가 이것이다. `UWxCharacterMovementComponent::JumpToLandingSection`은 가장 최근 몽타주만 보지 말고 재생 중인 몽타주 전부에서 `Grounded`를 찾는다. 일반 피격이 넉업 위에 겹친 채 착지하면 점프를 놓쳐 `Loop`에 갇히기 때문이다.

### 노티파이(시점형)

| 값 | 방식 |
|---|---|
| 처형 피해 행 | `UWxAnimNotify_FinisherDamage`가 행을 담고, 이벤트에 자기 자신을 실어 보낸다(`UWxAnimNotify_UseItem` 선례) |
| 짝 몽타주 | 공격자 몽타주 노티파이가 불리는 시점에 처형 어빌리티(서버)가 피해자 몽타주를 재생한다. 0초에 두면 피해자가 한 틱 늦게 시작한다 |
| 컷신 | 몽타주를 먼저 재생하고, 궁극기 몽타주 노티파이 시점에 컷신 시작을 시도한다. 전역 시간 정지라 몽타주도 멈춘다. 시작에 실패해도 따로 처리하지 않으며, 그러면 컷신 없이 몽타주가 그대로 진행된다. 실패는 두 플레이어가 한 틱 안에 궁극기를 누를 때만 생긴다. 몽타주 없이 컷신만 있는 궁극기는 만들 수 없다 |
| 사용 아이템 | 소비는 기존 꿀꺽 노티파이 이벤트로 한다. 어떤 아이템을 쓸지는 인벤토리가 정한다. 인벤토리는 이미 에스트병 하나만 있다는 전제(`RequestUseConsumable`)로 짜여 있다. 재생 전 보유 확인(빈 병 모션 방지)과 꿀꺽 시점 소비 모두 인벤토리에 묻는다. 아이템은 행에도 몽타주에도 두지 않는다. 소비 아이템 종류가 늘면 퀵슬롯 선택이 필요하다 |

- 스킬 컷신이 재생되는 동안 모든 플레이어의 입력형 어빌리티 발동을 막는다(사용자 제안).
  - 이동·시점 입력은 이미 시퀀스 재생 설정(`bDisableMovementInput`, `bDisableLookAtInput`)으로 막혀 있다.
  - 궁극기는 컷신 중 `IsBusy`로 발동이 막힌다.
  - 그 밖의 어빌리티 입력(공격·회피·질주·락온 등)은 막혀 있지 않다. 그래서 멈춘 세계에서 발동하고 비용을 치른 뒤, 컷신이 끝나면 이어서 진행된다.
  - 막는 방법은 설계 단계에서 정한다. 예: 컷신 컴포넌트가 무적 GE를 걸듯 차단 GE(BlockAbilityTags)를 건다.
  - 피격·사망 같은 반응 어빌리티는 막지 않는다.
- 처형 앞잡·뒤잡은 한 벌로 합친다. 변형 구조체가 없어진다. `AM_Shared_BackstabFinisher`는 참조처가 없다.
- 처형 상호작용 문구는 컬럼 없이 코드 기본값 "Finisher"를 쓴다.

### 값 이동

- 시체 파괴 지연은 `AWxCharacterBase`로 옮기고 `HandleDeath`에서 수명을 건다. 분신 BP만 0.1초다.
- 락온 수치, 질주 배율, 상호작용 사거리, 넉업 속도는 코드 기본값으로 둔다. 에셋 참조(락온 타게팅 프리셋)는 `UWxCombatDeveloperSettings`에 둔다.

### 범위 제외

- 스킬 레벨·강화: 계획은 있으나 지금 고려하지 않는다.
- Q-003(현광 자원 예외 공통화): 지금 신경 쓰지 않는다.

## 설계 인계 사항

- **스펙에 행 싣기**: `SourceObject`에 테이블, `DynamicSpecSourceTags`에 행 ID 태그를 넣는다. 둘 다 스펙과 함께 복제되며, Lyra가 입력 태그를 같은 통로로 싣는다.
  - 행 ID는 엔진의 태그 API(취소·차단·태그 발동) 대상이 될 수 없다. 이런 관계는 타입 태그로만 맺는다.
- **CDO 대신 스펙·인스턴스로 읽도록 바꿀 곳**:
  - 비용·쿨다운 MMC: `GetAbility()`는 CDO이므로 `GetAbilityInstance_NotReplicated()`로 바꾼다.
  - ASC 입력 라우팅, 입력 버퍼, 어빌리티 VM(슬롯 매칭·조건·표시·비용)
  - `UWxAbilitySet`: 액션 행 목록과 패시브 행 목록을 둔다.
- **행 ID로 고르도록 바꿀 곳**:
  - `UWxBTTask_ActivateAbility`, `UWxBTDecorator_ObserveAbility`
  - `UWxBTService_MirrorMovement`: 소유 태그 대신 활성 스펙의 ID를 본다.
  - `UWxBTTask_MirrorAbility`: 클래스와 함께 ID도 복사한다.
- **행별 동작**:
  - 발동 조건: `DoesAbilitySatisfyTagRequirements` 오버라이드(Lyra 선례)
  - 쿨다운 그룹: `GetCooldownGameplayEffect`
  - 패시브 트리거: `DynamicAbilityTriggers`
- **한 클래스에 스펙이 여럿 생긴다**: 클래스 기반 API(`FindAbilitySpecFromClass` 등)는 쓰지 않는다. 로그에는 행 ID를 남긴다.
- **궁극기 흐름 변경**: 비용 → 몽타주 → 노티파이로 컷신 시작 순서가 된다. 클라이언트 종료 보류 로직은 다시 검증한다.
- **콘텐츠 병합**:
  - 콤보 9개(약공 3, 반격 2, 패턴 4)
  - 회피 3개, 가드 반응 4개, 넉 계열 3개
  - 넉업·일반 피격 몽타주의 섹션 이름 변경
  - `WxAnimMontageToolset`(구조 복제·섹션 노티파이 복제)을 쓸 수 있다.
- **데이터 옮기기**: GA_ 39개는 모두 데이터 전용(그래프 로직 없음)이라 값만 변환하면 된다. 패시브 GA_ 2개는 `DT_Passive`로 옮긴다. `DT_Ability` 행 이름 변경은 DataTableRowFixup이 참조를 갱신한다.
- **검증기가 잡을 것**:
  - 타입별 섹션 규칙: 번호 연속, 예약 이름, 링크
  - 쿨다운 시간은 있는데 그룹이 없는 행
  - 같은 그룹 행 사이의 값 불일치
  - 같은 입력 후보 간 조건 중복
  - ID 태그 미등록, 두 테이블 사이 ID 중복
  - 필수 노티파이 누락
  - HitReact 섹션 누락
  - 패시브 트리거의 조상 관계

## 구현 단계

사용자가 "구현 진행"을 지시했다(2026-09-24). 1단계에서는 GA_를 유지한 채 규칙과 몽타주를 바꾼다. 2단계는 테이블 구동 기반(코드), 3단계는 데이터 이전과 GA_ 삭제, 4단계는 검증기다.

### 1-1단계 설계: 코드만으로 되는 규칙 변경

| 변경 | 위치 | 방법 |
|---|---|---|
| 콤보 창이 닫히면 1단부터 | `UWxAbilityBase`, `UWxAbility_Attack`·`_Skill` | `CloseComboWindow`가 가상 훅 `OnComboWindowClosed`를 부르고, 두 클래스가 단계 인덱스를 초기화한다 |
| 늦게 도착한 노티파이 거르기 | `UWxAnimNotifyState_ComboWindow`, `UWxAnimNotify_StartRecovery`, `UWxAbilityBase` | 노티파이의 몽타주 인스턴스 ID를 넘기고, 어빌리티가 지금 재생 중인 인스턴스와 다르면 무시한다. 큐 경로와 브랜칭 포인트 경로를 모두 처리한다(`UWxAnimNotifyState_ApplyGameplayEffect` 방식) |
| 착지 점프 | `UWxCharacterMovementComponent::JumpToLandingSection` | 가장 최근 몽타주 대신 재생 중인 몽타주 인스턴스 전부에서 `Grounded`를 찾는다 |
| 그로기 강등 | `UWxEffectComponent_DamageReaction`, `UWxAbility_HitReact` | 대상이 그로기면 넉 계열 반응 태그를 `HitReact.Normal`로 바꿔 보낸다. HitReact의 강등 코드는 지운다 |
| 시체 수명 | `AWxCharacterBase`, `UWxAbility_Death` | `CorpseLifeSpan`을 캐릭터에 두고 `HandleDeath`에서 수명을 건다. Death의 `PendingDestroyTime`은 지운다. `BP_Minion` = 0.1 |
| 컷신 중 입력 차단 | `UWxSkillCutsceneComponent`, 새 `UWxEffect_SkillCutscene`, 태그 `Effect.SkillCutscene` | 세션 시작 때 모든 플레이어 ASC에 입력형 어빌리티(Attack·Skill·Ultimate·Dodge·Guard·UseItem·Sprint·LockOn·Interact)를 막는 GE를 걸고, 종료 때 스택 하나씩 뺀다 |
| 소비 아이템을 인벤토리가 선택 | `UWxInventoryComponent`, `UWxItemUseComponent`, `UWxAbility_UseItem` | `CanUseItemByDef`/`UseItemByDef`를 `CanUseConsumable`/`UseConsumable`로 바꾼다(아이템 지목 없음). 어빌리티의 `ConsumableDef`는 지운다 |

에셋 변경:
- `GA_Minion_Attack_Heavy`·`Skill_1`·`Skill_2`의 ActivationGroup을 기본값(Exclusive)으로 되돌린다.
- `GA_HGTest_Skill_1`~`3`의 재발동을 기본값(허용)으로 되돌린다.
- `BP_Minion`의 시체 수명을 0.1로 둔다.

검증: 빌드. 가능한 범위는 임시 자동화 테스트로 확인하고, 테스트는 제출 전에 지운다. 콤보 재시작·착지·컷신 입력 차단은 플레이로 확인한다.

### 1-1단계 결과

- 설계 표대로 구현했다. 표에 없던 것은 두 가지다.
  - 노티파이 두 개는 브랜칭 포인트 경로(`BranchingPointNotify*`)를 오버라이드한다. 엔진 기본 구현이 빈 이벤트 참조를 넘겨 몽타주 인스턴스를 잃기 때문이다.
  - `UWxInventoryComponent`의 선택 규칙: Usable 조각이 있고, Charges 조각이 있으면 남은 횟수가 있는 첫 인스턴스를 쓴다.
- 에셋 6개(분신 공격 3개의 발동 그룹, HGTest 스킬 3개의 재발동)는 MCP로 저장했다.
- 사용자가 새 빌드(DebugGame)로 에디터를 다시 열어 `BP_Minion` 시체 수명을 0.1로 저장했다. 지운 프로퍼티 값이 남아 있던 `GA_Minion_Death`·`GA_Shared_UseItem`도 다시 저장했다. 저장된 파일에서 `CorpseLifeSpan`은 0.1이고, `PendingDestroyTime`·`ConsumableDef`는 남아 있지 않음을 확인했다.
- 사용자 확인(2026-09-24): "잘 작동되네요". 항목별 확인 범위는 따로 받지 않았다. 설계에서 플레이 확인 대상으로 든 것은 세 가지다.
  - 콤보 재시작
  - 착지
  - 컷신 입력 차단
- 자동화 테스트는 만들지 않았다.

## 위험과 대응(설계에서 반영)

**오류 없이 조용히 틀어지는 곳**
- **CDO 읽기**: 한 클래스를 여러 행이 공유하므로 `Spec.Ability`나 GE 컨텍스트의 `GetAbility()`로 읽으면 기본값이 나온다. 대응은 두 가지다.
  - 행 데이터를 클래스 프로퍼티로 두지 않는다.
  - 행 조회는 인스턴스에서만 허용한다(ensure).
- **클래스로 찾는 API**: `FindAbilitySpecFromClass`, `TryActivateAbilityByClass`, BP의 By Class 노드는 첫 스펙만 본다. 규칙으로 금지한다. 분신 따라하기의 클래스 부여는 ID 복사로 바꾼다.
- **이름 규약 오타**: 반응·콤보·회피 섹션 이름, `Grounded` 이름이 틀리면 반응이 나오지 않거나(Normal 폴백 없음) 기본 방향으로 간다. 대응은 두 가지다.
  - 몽타주·테이블 저장 시 검증기를 돌린다.
  - 실행 중 누락 섹션은 경고를 한 번 남긴다.

**GA_ 방식 대비 잃는 것**
- **디버그 표시**: `showdebug abilitysystem`, 게임플레이 디버거, 엔진 로그가 클래스 이름으로만 표시된다. 행 ID를 찍는 Wx 로그와 디버그 명령으로 보완한다.
- **다른 머신에서 행 식별 불가**: 스펙은 소유 클라에만 복제되고(`COND_ReplayOrOwner`, `AbilitySystemComponent.cpp:1862`), GE 컨텍스트로 복제되는 것은 공유 CDO다. 그래서 다른 머신은 어느 행에서 왔는지 알 수 없다. 지금 이것을 쓰는 코드는 없다. 필요해지면 행 ID를 따로 싣는다.
- **편집 충돌 단위**: GA_ 파일 단위에서 캐릭터 테이블 단위로 커진다(바이너리 잠금). 필요하면 더 잘게 나눈다.
- **정보 분산**: 테이블, 몽타주 노티파이(컷신·짝 몽타주·피해 행), 타입 규칙, 세트, BT에 나뉜다. 검증기로 보완하고, 행에서 노티파이 값을 보여 주는 에디터 표시는 후순위로 둔다.

**운영 규칙**
- **행 이름과 ID 태그 이중 관리**: 행을 추가하면 태그를 등록하고, 이름을 바꾸면 태그 리디렉트와 BT 참조를 함께 바꾼다.
- **플레이 중 테이블 수정**:
  - 부여 시점 값(입력 바인딩, 패시브 트리거)은 캐릭터를 다시 스폰해야 반영된다.
  - 행 포인터는 캐시하지 않는다(에디터가 행 메모리를 재배치한다).
- **로드 단위**: 캐릭터 전용 행을 공용 테이블에 넣지 않는다.

**전환 작업 자체**
- 몽타주 병합은 노티파이를 수작업으로 옮기므로 실수할 수 있다.
- 블렌드 인 0.05초 통일은 조작감을 바꾼다. 플레이 확인이 필요하다.
- 브리핑 문서의 GA_·ABS_ 설명과 스킬 콤보 설명은 기획 담당자에게 수정을 제안한다(직접 수정 금지).

## 확인한 근거

- **GAS**:
  - `SetAssetTags`는 생성자 전용 ensure다(`GameplayAbility.cpp:1306`).
  - `CancelAbilities`·`TryActivateAbilitiesByTag`는 CDO 태그로 비교한다(`AbilitySystemComponent_Abilities.cpp:1339`, `:1549`). 재발동도 CDO 값을 읽는다(`:1836`).
  - 스펙별 트리거 등록(`:578`). 인스턴스가 있으면 발동 판정을 인스턴스에서 한다(`:1797`).
  - 소유 클라에서도 스펙 복제 때 인스턴스를 만들고 `OnGiveAbility`를 부른다(`GameplayAbilityTypes.cpp:295`).
  - `FGameplayEffectContext::GetAbility()`는 CDO를 돌려준다(`GameplayEffectTypes.cpp:223`).
  - 복제된 GE는 클라에서 지속시간을 재계산하지 않는다(`PostReplicatedAdd`).
  - `PlayMontage`마다 `PlayInstanceId`를 올리고 시작 섹션을 복제한다(`:3073`, `:3083`).
- **애니메이션**:
  - `Montage_Play`는 같은 에셋이어도 새 인스턴스를 만들고 앞 인스턴스를 새 몽타주의 BlendIn으로 멈춘다(`AnimInstance.cpp:2770`, `:2800`).
  - 링크 없는 섹션은 섹션 끝을 기준으로 자동 블렌드아웃한다(`AnimMontage.cpp:2626`).
  - 활성 목록은 인스턴스 단위로 정리한다(`AnimInstance.cpp:3706`).
  - 한 몽타주의 슬롯은 모두 같은 그룹이어야 재생된다(`AnimMontage.cpp:1019`, `AnimInstance.cpp:2762`).
  - 0초 노티파이도 첫 틱에 불린다(`AnimSequenceBase.cpp:477`).
  - 궁극기 컷신은 `SetGlobalTimeDilation`으로 시간을 멈춘다.
- **데이터**:
  - 콤보·패턴 몽타주 29개는 블렌드 설정이 모두 같다(0.25초 HermiteCubic, `DefaultSlot`).
  - 회피 0.05 / 백스텝 0.25, 가드 반응 0 / 0.25 / 0.25 / 0.05초로 블렌드 인이 다르다.
  - 극한 회피 몽타주에 슬로모션 구간이 있다.
  - 같은 IA에 여러 GA_가 붙어 태그 조건으로 갈린다(HGTest 약공 4개 등).
  - 행 단위 트리거를 설정한 GA_는 Passive 2개뿐이다.
- **기존 데이터 결함**:
  - `GA_HGTest_Attack_Heavy_2`: 쿨다운 GE가 None이라 5초 쿨다운이 적용되지 않는다.
  - `GA_Shared_Sprint`: 행이 없어 진입 SP 비용이 0이다.
  - `DT_Ability`: Title이 전부 비어 있고 Description은 한국어다.

## 기각한 대안

- **타입 데이터 구조체 칸**: 사용자가 반대했다.
- **콤보를 어빌리티 여러 개로 분리**: 브리핑 2-5(서로 다른 어빌리티 간 연계 금지)와 충돌한다. 배타 규칙 예외와 연계 장치가 필요하다.
- **몽타주 메타데이터(`UAnimMetaData`)**: 사용자가 노티파이를 택했다.
- **넉업 착지 ANS**: 사용자 제안에 따라 `Grounded` 이름 방식을 유지한다.
- **`트리거 이벤트`·`발동 시 효과`를 액션 테이블 공통 컬럼으로 두는 안**: 프로젝트 규칙상 패시브만 쓰는 컬럼이라 패시브 테이블 분리로 대체했다.
- **순정 이탈이 없는 대안(테이블에서 GA_ 자동 생성)**: 방향이 GA_ 폐기라서 택하지 않았다.

## 미결

없음. 마지막 두 건은 2026-09-24 사용자 동의로 확정했다.
- 소비 아이템은 인벤토리가 고른다.
- 컷신 시작 실패는 따로 처리하지 않는다.

컷신 중 입력 차단은 이후 사용자 제안으로 추가했다.
