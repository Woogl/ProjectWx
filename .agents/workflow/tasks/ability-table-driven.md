# 어빌리티 테이블 구동 전환

상태: 완료 · 체크리스트 7/7 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

이전 상태: 구현 중. 1-1·1-2단계는 구현했고 사용자가 플레이로 확인했다(2026-09-24). 2단계 설계는 사용자가 확정했다(2026-09-24: Q-단계 A, Q-태그 B, 기존 `Ability` 태그를 행 태그로 쓰고 행 이름은 자유, BT는 처음 성공, 디버그 치트 제외). 2단계를 구현했고(코드·데이터 전환·GA_ 삭제) 사용자가 플레이로 확인했다(2026-09-24). 사용자 지시로 제출은 보류했다(미커밋). 4단계(검증기) 설계를 사용자가 확정했고(2026-09-25: Q-범위 A, Q-회피 A) 구현과 자체 검증을 마쳤다. 사용자가 에디터로 확인한 뒤 후속 수정(스킬 3 조건, 중복 메시지, 속성 행 검사, 솔저 속성 행 재연결)까지 반영했다(미커밋). 이어 행 찾기를 다시 설계했다(2026-09-25): 타입 칸 부활, 행의 동적 태그, 세트 = 테이블 목록. 구현과 네트워크 PIE 검증을 마쳤다(미커밋, 아래 "세트 = 테이블 목록"). 이어 사용자가 GA_ 에셋으로 돌아가기로 했다(2026-09-25, 미결의 Q-GA_ 복귀). 설계는 아래 "GA_ 복귀 설계"에 있고 사용자가 확정했다. 구현과 자체 검증(빌드 2종, 데이터 검증, 단독·네트워크 PIE)을 마쳤고, DT_Effect 제거·락온 프리셋 복귀·몽타주 검증 제거까지 사용자 지시로 제출했다(2026-09-25, `5153da936` 회피 몽타주, `0473e201b` 슬롯 VM·스캐너, `e305161ee` GA_ 데이터, `7c52ce0ce` DT_Effect). Wiki는 `5c66bfaf9`로 반영했다(원자료 2개, 기사 12개). 인게임 플레이 확인이 남았다(아래 "GA_ 복귀 결과"). 이 문서의 테이블 관련 절(행 컬럼, 세트 = 테이블 목록, 위험과 대응의 행·CDO 항목)은 이력으로만 남는다.

- 날짜: 2026-09-24
- 요청: GA_ 에셋을 하나하나 만드는 방식을 폐기하고 어빌리티를 테이블로 완전 구동한다. 사용자 초안 컬럼: 어빌리티 이름, 어빌리티 타입, 발동 타입, 애니메이션 몽타주, 키입력, 코스트 종류, 코스트 양, 쿨타임, 쿨타임 스택, 아이콘, 표기명, 설명문.
- 기준: HEAD `142fab5d6`. 조사 방법은 에디터 MCP 읽기(GA_ 39개, `DT_Ability` 21행, ABS_ 9개, 몽타주 45개)와 UE 5.8 GAS·애니메이션 소스 대조다. 빌드·플레이 검증은 하지 않았다.

## 요청

- 추가 요청 · 이우성 2026-09-25

> 이 작업은 테이블화를 안하는 쪽으로 마무리되었습니다. 기능은 전부 정상 작동하고 있습니다.

- 추가 요청 · 이우성 2026-09-25

> 이 작업은 테이블화 안하는 쪽으로 결론났습니다. 테스트 했을 때에도 모두 이상 없습니다.

- 추가 요청 · 이우성 2026-09-25

> 이 작업은 테이블 전환을 하지 않는 것으로 마무리되었습니다. 테스트했을 때에도 문제 없었습니다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| HGTest 어빌리티 | HGTest가 있는 맵: HGTest의 어빌리티가 발동하고 연출된다 | 사람 | 통과 | 이우성 2026-09-25 |
| 분신 | 분신이 있는 맵: 분신 어빌리티가 발동하고 연출된다 | 사람 | 통과 | 이우성 2026-09-25 |
| 도플갱어 | 도플갱어가 있는 맵: 도플갱어 어빌리티가 발동하고 연출된다 | 사람 | 통과 | 이우성 2026-09-25 |
| 조작감 | 게임: 블렌드 인 0.05초 통일 뒤 전반 조작감이 괜찮다 | 사람 | 통과 | 이우성 2026-09-25 |
| 가드 경감·버프 아이콘 | 게임: 가드 중 피해가 절반으로 줄고 방패 버프 아이콘이 보인다 | 사람 | 통과 | 이우성 2026-09-25 |
| 템플릿 패시브 UP | 게임: 템플릿 패시브가 UP 5를 준다 | 사람 | 통과 | 이우성 2026-09-25 |
| 락온 | 게임: 락온이 어빌리티 데이터의 타게팅 프리셋으로 대상을 잡는다 | 사람 | 통과 | 이우성 2026-09-25 |

## 원칙

- 타입(C++ 클래스)은 시스템 규칙이다. 어빌리티 태그, 취소·차단 관계, 발동 그룹, 입력 방식(홀드·토글), 캐릭터 공통 발동 조건, 네트워크 정책이 여기에 속한다.
- 행은 콘텐츠다. 몽타주, 수치, 표시, 입력 IA, 캐릭터 상태 조건, 쿨다운 그룹이 여기에 속한다.
- 이 구분은 `Docs/Meeting/어빌리티 규칙 브리핑.md` 2-2("어빌리티 태그는 시스템 규칙, 기획 커스터마이징 불허")와 같다. GAS가 태그·관계·재발동을 CDO에서만 읽는다는 제약과도 맞다.

## 확정한 결정

### 테이블 구성

- 액션 어빌리티 테이블은 행 구조체가 하나다. 몽타주를 하드 참조하므로 테이블 단위가 곧 로드 단위다. 적 테이블을 한 장으로 두면 적 하나만 등장해도 모든 적의 패턴이 로드된다.
- 세트가 테이블을 통째로 부여하므로(2026-09-25 세트 = 테이블 목록) 테이블은 부여 단위로 나눈다.
  - 공용 3장: `DT_Ability_Shared`(플레이어·적 공용: 피격, 사망), `DT_Ability_SharedPlayer`, `DT_Ability_SharedEnemy`
  - 캐릭터별: `DT_Ability_<캐릭터>`. 소환물도 자기 테이블을 가진다(`DT_Ability_Minion`).
  - 처음에는 공용 1장에 소환물 행을 캐릭터 테이블에 두었다. 세트가 행을 골라 담던 때의 구성이다.
- 패시브도 캐릭터별 테이블(`DT_Passive_<캐릭터>`)에 둔다(아래 패시브 테이블).
- 부여는 지금처럼 세트(ABS_)가 한다. 세트에는 액션 테이블 목록과 패시브 테이블 목록을 따로 두고, 테이블의 모든 행을 부여한다. 같은 입력에 후보가 여럿이면 테이블 순서, 그 안의 행 순서대로 시도하고 처음 성공한 것을 쓴다.
- 행 이름은 자유다. 행은 타입(클래스)과 동적 태그를 가진다. 스펙은 테이블 안에서 타입과 동적 태그로 자기 행을 찾으므로, 한 테이블 안에서 타입이 같은 행은 동적 태그가 달라야 한다. 변천: "행 ID = 새 `AbilityId` 태그, 전역 고유"(초안) → 기존 `Ability` 태그 하나로 타입과 행을 함께 정함(2단계 설계) → 타입 칸과 동적 태그(2026-09-25).
- 편집은 UE 에디터에서 한다.
- 타입 데이터 구조체 칸은 두지 않는다. 특정 타입만 쓰는 컬럼도 두지 않는다.

### 액션 어빌리티 컬럼

| 컬럼 | 형식 | 비고 |
|---|---|---|
| 이름 | 행 이름 | 자유(2단계 설계에서 변경). 로그에 쓰인다 |
| 타입 | 코드가 정한 목록 | 아래 타입 목록 |
| 동적 태그 | `Ability` 태그 목록 | 스펙의 `DynamicSpecSourceTags`로 실린다. 한 테이블 안 같은 타입 행을 가르고, BT·분신이 이 태그로 행을 고른다(2026-09-25) |
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
| 이름 | 행 이름 | 자유(2단계 설계에서 변경) |
| 동적 태그 | `Ability` 태그 목록 | 클래스가 하나라 한 테이블에 행이 여럿이면 행마다 달라야 한다(2026-09-25) |
| 트리거 이벤트 | 태그 목록 | 서로 조상 관계인 태그는 함께 넣지 못한다(한 이벤트에 두 번 발동) |
| 발동 조건 | `FGameplayTagRequirements` | 예: 도플갱어가 있을 때만 |
| 적용 효과 | GE 목록 | 어빌리티가 끝나도 남는다 |

- 행 하나가 `UWxAbility_Passive` 스펙 하나다. 트리거는 스펙별 `DynamicAbilityTriggers`로 등록한다.
- "공격 1회당 한 번만 지급" 같은 중복 방지 규칙은 지금처럼 Passive 클래스가 맡는다.
- 패시브 행을 읽는 곳은 부여와 Passive 클래스뿐이다. 처음에는 무거운 에셋 참조가 없어 한 장(`DT_Passive`)에 두었으나, 세트가 테이블을 통째로 부여하게 되면서 캐릭터별로 나눴다(2026-09-25).
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
| 컷신 | 시점형이 아니다(Q-궁극기). 궁극기 몽타주의 `UWxAnimNotify_SkillCutscene`이 시퀀스를 담는 표식이고, 불려도 하는 일이 없다. 어빌리티가 재생 전에 읽어 지금 흐름(컷신 → 몽타주)을 그대로 쓴다. 표식이 없으면 몽타주만 재생한다. 몽타주 없이 컷신만 있는 궁극기는 만들 수 없다 |
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

- **스펙에 행 싣기**: `SourceObject`에 세트, `DynamicSpecSourceTags`에 행 태그를 넣는다(2단계 설계에서 "테이블·행 ID 태그"를 바꿨다). 둘 다 스펙과 함께 복제되며, Lyra가 입력 태그를 같은 통로로 싣는다.
  - 행 태그는 엔진의 태그 API(취소·차단·태그 발동) 대상이 될 수 없다. 엔진은 클래스의 에셋 태그만 본다. 이런 관계는 타입 태그로만 맺는다.
- **CDO 대신 스펙·인스턴스로 읽도록 바꿀 곳**:
  - 비용·쿨다운 MMC: `GetAbility()`는 CDO이므로 `GetAbilityInstance_NotReplicated()`로 바꾼다.
  - ASC 입력 라우팅, 입력 버퍼, 어빌리티 VM(슬롯 매칭·조건·표시·비용)
  - `UWxAbilitySet`: 액션 행 목록과 패시브 행 목록을 둔다.
- **행 태그로도 고르게 할 곳**(2단계 설계에서 "행 ID로 고르도록"을 구체화):
  - `UWxBTTask_ActivateAbility`, `UWxBTDecorator_ObserveAbility`
  - `UWxBTService_MirrorMovement`: 소유 태그 대신 활성 스펙의 태그를 본다.
  - `UWxBTTask_MirrorAbility`: 클래스와 함께 세트·행 태그도 복사한다.
- **행별 동작**:
  - 발동 조건: `DoesAbilitySatisfyTagRequirements` 오버라이드(Lyra 선례)
  - 쿨다운 그룹: `GetCooldownGameplayEffect`
  - 패시브 트리거: `DynamicAbilityTriggers`
- **한 클래스에 스펙이 여럿 생긴다**: 클래스 기반 API(`FindAbilitySpecFromClass` 등)는 쓰지 않는다. 로그에는 행 태그를 남긴다.
- **궁극기**: 흐름(컷신 → 비용 → 몽타주)과 클라이언트 종료 보류 로직은 바뀌지 않는다. 시퀀스를 읽는 곳만 어빌리티 프로퍼티에서 몽타주 표식 노티파이로 옮긴다.
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
  - 행 태그가 비었거나 한 세트 안에서 겹치는 행(2단계 설계에서 "ID 태그 미등록, 두 테이블 사이 ID 중복"을 바꿨다)
  - 필수 노티파이 누락
  - HitReact 섹션 누락
  - 패시브 트리거의 조상 관계
  - 자기 단계 섹션 끝을 넘는 노티파이 구간(다음 단 인스턴스가 이어받는다)
  - 단계 섹션 시작에 정확히 맞춘 단발 노티파이(앞 단 끝에서 불린다)

## 구현 단계

사용자가 "구현 진행"을 지시했다(2026-09-24). 1단계에서는 GA_를 유지한 채 규칙과 몽타주를 바꾼다. 2단계는 테이블 구동 기반(코드), 3단계는 데이터 이전과 GA_ 삭제, 4단계는 검증기다.
- 2단계 설계 중 사용자가 2·3단계를 합치기로 했다(Q-단계 A, 2026-09-24). 2단계가 코드·데이터 전환·GA_ 삭제를 함께 하고, 다음은 4단계(검증기)다.

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

### 1-2단계 설계: 몽타주 섹션 모델

GA_는 유지한다. 몽타주 프로퍼티를 하나로 줄이고, 변형은 섹션 이름으로 고른다.

| 대상 | 코드 | 에셋 |
|---|---|---|
| 공통 | `UWxAbilityBase::PlayMontage`는 없는 시작 섹션이면 경고하고 실패한다. 단계 섹션 규칙(`1`, `2`, …, 번호 섹션이 없으면 처음부터 한 단계)은 `UWxAbilityBase`의 정적 함수 두 개(단계 수, 단계 섹션)가 한 곳에서 정한다 | — |
| 공격·스킬·패턴 | `ComboMontages` 배열 → `ComboMontage` 하나. 단계 인덱스로 섹션을 고른다. 패턴은 블렌드아웃 때 다음 섹션을 새로 재생하는 방식을 유지한다 | 9개 그룹을 번호 섹션 몽타주로 합친다. 새 이름은 GA_ 이름의 접두사만 `AM_`로 바꾼 것이다(예: `AM_HGTest_Attack_Light_1`). `GA_Soldier_Pattern_3`의 세 번째 칸(None)은 버린다 |
| 회피 | `BackstepMontage`·`PerfectDodgeMontage`를 지운다. 이동 입력이 없으면 `Backstep`, 극한 회피는 `Success<방향>` 섹션이다. `Backstep`이 없으면 지금처럼 `Back`으로 간다 | `AM_Shared_Dodge` 끝에 백스텝과 극한 회피를 붙인다. 블렌드 인은 이미 0.05초다 |
| 가드 반응 | 몽타주 4개 → `GuardReactMontage` 하나. 넉 계열이면 `GuardKnockback`, 아니면 `GuardHit`. 넉 계열 몽타주가 없을 때 가드 피격으로 가던 폴백은 없앤다 | `AM_Shared_GuardReact`(섹션 `GuardHit`, `GuardKnockback`, `GuardBreak`, `PerfectGuard`), 블렌드 인 0.05초 |
| 피격 반응 | 몽타주 5개 → `HitReactMontage` 하나. 반응 태그(패리는 `Event.Hit.Parry`)의 끝 이름이 섹션 이름이다. 그 섹션이 자기 몽타주에 있는 GA_만 이벤트에 반응한다 | `AM_Shared_HitReact_Knock`(`KnockBack`, `KnockDown`, `Parry`). 넉업은 `Default`를 `KnockUp`으로, 일반 피격은 `Default`를 `Normal`로 이름만 바꾼다. `GA_Shared_HitReact_KnockUp`·`GA_Shared_HitReact_Normal`을 추가하고 피격 GA_를 주는 세트 3개(`ABS_Shared_Player`·`ABS_Shared_Enemy`·`ABS_Sandbag`)에 넣는다 |
| 처형 | 변형 구조체와 앞잡·뒤잡 구분(`bBackstab`)을 지우고 `FinisherMontage` 하나로 둔다. 짝 몽타주는 새 노티파이 `UWxAnimNotify_FinisherVictim`이, 피해 행은 `UWxAnimNotify_FinisherDamage`가 담는다. 두 노티파이 모두 이벤트에 자기 자신을 싣고, 서버의 처형 어빌리티가 받아 처리한다. 새 태그는 `Event.PlayFinisherVictimMontage`다. 적의 `OnInteracted`가 채우던 `TargetTags`는 읽는 곳이 없어지므로 지운다 | `AM_Shared_Finisher` 0초에 짝 몽타주 노티파이를 둔다. 피해 노티파이에 `DT_Damage` 행(`AM_Shared_Finisher`)을 넣는다 |
| 에디터 도구 | `WxAnimMontageToolset`에 두 가지를 추가한다. `AppendMontage`는 원본 몽타주의 세그먼트·섹션·노티파이를 대상 끝에 이어 붙인다. `RenameSection`은 섹션 이름과 그 이름을 가리키는 링크를 함께 바꾼다. `DescribeMontage`는 노티파이 오브젝트 경로도 돌려준다 | — |

- 합친 뒤에는 참조가 없어진 원래 몽타주를 지운다. 참조가 원래 없던 `AM_Shared_BackstabFinisher`는 지우지 않는다.
- 섹션 끝에 다음 섹션 링크가 없으면 엔진은 위치를 그 섹션 끝에 고정하고 재생을 멈춘다(`AnimMontage.cpp:2737`). 그래서 앞 단 인스턴스가 블렌드아웃하는 동안에도 다음 섹션의 노티파이는 불리지 않는다.
- 검증은 두 가지다. 빌드와 MCP로 섹션·노티파이 배치를 확인한다. 콤보·패턴·회피·가드·피격·처형은 플레이로 확인한다.

### 1-2단계 결과

- 설계 표대로 코드와 에셋을 바꿨다. 빌드는 성공했다(Development).
- 병합 도구를 쓰다가 엔진 규칙 두 가지를 확인했고, 도구가 둘 다 피하도록 고쳤다.
  - 몽타주는 0초가 아닌 섹션 시작에 놓인 노티파이를 앞 섹션 끝에서 불리게 잡는다(`UAnimMontage::CalculateOffsetFromSections`, `AnimMontage.cpp:887`). 트리거 오프셋을 다시 계산하면 단 시작의 노티파이가 앞 단 끝으로 넘어간다. 그래서 도구는 원본의 트리거 오프셋을 그대로 둔다.
  - 몽타주 길이를 늘리면 엔진이 끝에 프레임을 끼운 것으로 보고, 끝에 닿은 노티파이 구간을 늘리거나 민다(`AnimSequenceBase.cpp:1101`). 그래서 도구는 대상의 기존 노티파이 시각을 길이 변경 전후로 보존한다.
- 에셋 결과: 몽타주 4종을 새로 만들었다.
  - 번호 섹션 몽타주 9개
  - `AM_Shared_GuardReact`
  - `AM_Shared_HitReact_Knock`
  - `AM_Shared_Dodge` 확장(섹션 17개)
- 그 밖의 에셋 변경:
  - 넉업·일반 피격 섹션 이름을 바꿨다.
  - 처형 몽타주에 짝 몽타주 노티파이를 넣었다.
  - GA_ 28개와 세트 3개를 저장했다. 이 중 GA_ 2개(`GA_Shared_HitReact_KnockUp`·`_Normal`)는 새로 만들었다.
  - 원래 몽타주 38개를 지웠다. 지우기 전에 참조가 없음을 확인했다.
- MCP 검증 결과:
  - 병합한 몽타주마다 섹션 시작·길이·링크와 노티파이(종류·시각·길이·트랙)가 원본을 이어 붙인 값과 일치한다.
  - 모든 GA_가 코드가 요구하는 섹션을 가진 몽타주를 가리킨다.
- 발견한 기존 데이터 결함(수정하지 않음): `AM_Shared_Dodge` 방향 섹션의 무적·슬로모션 구간 일부가 섹션 시작에 놓여 있다. 위 엔진 규칙 때문에 이 구간들은 앞 방향 섹션 끝에서 시작된다(예: `ForwardRight` 무적이 0.8999초). 구간 노티파이라 자기 섹션 재생에서도 불리므로 동작 영향은 앞 방향 회피 끝의 한 프레임 정도다.
- 병합 몽타주를 편집할 때 주의할 점이 있다. 단계 섹션 시작에 정확히 맞춰 둔 단발 노티파이는 그 단을 재생할 때 불리지 않는다. 섹션 시작보다 조금 뒤에 둬야 한다. 4단계 검증기가 잡을 항목에 넣는다.
- 궁극기(Q-궁극기 결정 B): `CutsceneSequence`를 지우고, 새 `UWxAnimNotify_SkillCutscene`을 궁극기 몽타주 3개의 0초에 넣었다. 시퀀스를 넣은 뒤 다시 읽어 확인했다. 컷신만으로 끝나던 분기(몽타주 없음)는 없앴다.
- 저장한 GA_ 파일에 지운 프로퍼티 이름(`CutsceneSequence`, `ComboMontages`, `BackstepMontage` 등)이 남아 있지 않음을 확인했다.
- 사용자 플레이에서 버그 1건이 나왔다: 평타 2타 중에 누르면 1타가 나갔다. 원인과 수정은 아래와 같다.
  - 원인: 1타 콤보 창의 끝이 부동소수 오차로 2타 섹션 시작보다 6e-8초 뒤에 있었다. 이 오차 때문에 2타를 트는 새 인스턴스가 첫 틱에 1타 창을 수집했다. 엔진은 같은 노티파이 구간을 다른 인스턴스가 수집하면 새 구간으로 보지 않고, 계속 활성인 구간의 참조를 그 인스턴스 것으로 바꾼다(`UAnimInstance::TriggerAnimNotifies`, `AnimInstance.cpp:1890`). 그래서 1타 창이 2타 인스턴스 ID로 닫혔고, 늦은 노티파이 거르기를 통과해 단계가 초기화됐다. 단마다 몽타주가 따로였을 때는 에셋이 달라 생기지 않던 일이다.
  - 진단: 임시 로그로 흐름을 확인했다. 재발동(`bRetriggerInstancedAbility`)은 정상이었다(같은 프레임에 종료 → 다음 단 발동). 진단 로그는 지웠다.
  - 수정: 병합 도구에 `SnapNotifyEndsToSections`를 추가했다. 섹션 경계에서 2ms 안쪽으로 끝나는 노티파이 구간의 끝을 경계에 정확히 맞추고, 경계 직전에 끝나게 한다(OffsetBefore). 엔진은 섹션 시각과 정확히 같은 끝만 경계 직전으로 잡고, 조금이라도 다르면 오프셋을 0으로 둔다. `AppendMontage`는 이 처리를 자동으로 한다.
  - 이미 합친 몽타주에 적용했다: 콤보·패턴 8개에서 구간 20개를 맞췄다. 가드 반응·넉 계열·템플릿 패턴 1에는 맞출 구간이 없었다. 맞춘 뒤 모든 구간이 자기 섹션 끝보다 먼저 끝나고, 노티파이 시각은 그대로다.
  - 사용자 재확인(2026-09-24): "이제 해결되었네요". 로그에서도 1→2→3→4단 뒤 1단으로 순환하는 흐름을 확인했다.
- 4단계 검증기 항목에 두 가지를 더했다(설계 인계 사항).

### 2단계 설계: 테이블 구동 전환

기준은 HEAD `8652e585e`다. 조사 방법은 코드 전수 읽기, 에디터 MCP 읽기(GA_ 41개, 세트 9개, BT 4개, `DT_Ability` 21행, HUD 슬롯 리졸버), UE 5.8 GAS 소스 대조다. 설계를 확정하기까지 내린 결정은 맨 아래 결정 기록에 있다.

**행과 스펙 연결**
- 행 이름은 자유다. 행마다 행 태그(`Tag`) 하나를 기존 `Ability` 태그 트리에서 고른다.
- 부여: `UWxAbilitySet`이 행마다 `FGameplayAbilitySpec(Row.Type, 1, INDEX_NONE, 세트)`를 만들고, 스펙의 `DynamicSpecSourceTags`에 행 태그를 넣는다.
- 패시브 행은 `UWxAbility_Passive` 스펙으로 만들고, 행의 트리거 이벤트를 스펙의 `DynamicAbilityTriggers`에 넣는다. 엔진이 부여할 때 클래스 트리거와 함께 등록한다(`AbilitySystemComponent_Abilities.cpp:578`).
- 읽기: `UWxAbilitySet::FindAbilityRow(Spec)`·`FindPassiveRow(Spec)`가 스펙의 세트(SourceObject)에서, 행 태그가 스펙에 실린 태그와 같은 행을 찾는다. 싣는 곳과 읽는 곳을 한 클래스에 둔다.
- 규칙은 하나다: 한 세트 안에서 같은 행 태그를 두 번 쓰지 않는다.
  - 스펙과 함께 복제되는 칸은 클래스·레벨·InputID·SourceObject·태그뿐이라(5.8 `FGameplayAbilitySpec`), 행 이름을 실을 칸이 없다. 한 세트에서 태그가 같으면 두 스펙이 같은 행을 읽는다.
  - 다른 세트끼리, 다른 캐릭터끼리는 겹쳐도 된다.
  - 어기면 부여할 때 오류를 남기고 뒤 행을 부여하지 않는다.
- 여러 행을 한 번에 고를 때는 부모 태그를 쓴다. `Ability.Attack.Light`는 `.1`·`.2` 행을 모두 가리킨다.
- 어빌리티는 `UWxAbilityBase::GetAbilityRow()`로 자기 스펙의 행을 읽는다. 행 포인터는 캐시하지 않는다.
- CDO에는 스펙이 없으므로 `GetAbilityRow()`는 nullptr(행 값 없음)을 돌려준다. 위험과 대응의 처음 계획("행 조회는 인스턴스에서만 허용한다(ensure)")을 이렇게 바꿨다.
  - 엔진이 CDO에 대고 오버라이드를 부르는 정상 경로가 있어 ensure가 터진다. 태그로 발동할 때 후보 거르기(`GetActivatableGameplayAbilitySpecsByAllMatchingTags`, `AbilitySystemComponent_Abilities.cpp:1554`), 원격 발동 사전 판정(`:1665`), `HasActivatableTriggeredAbility`(`:2487`), 디버그 표시(`AbilitySystemComponent.cpp:3024`)다.
  - 행 값이 클래스 프로퍼티에 없으므로, CDO로 잘못 읽으면 다른 행의 값이 아니라 빈 값이 나온다.
  - 프로젝트 코드가 CDO로 행 값을 읽던 곳은 모두 인스턴스로 바꾼다(아래 표).
- 엔진이 함께 싣는 것: 어빌리티가 만드는 GE의 컨텍스트에 SourceObject(세트)가 실리고(`GameplayAbility.cpp:1922`), 소스 태그에 행 태그가 실린다(`:1390`). 행 태그가 GE 소스 태그에 실리는 것은 GA_가 에셋 태그를 덮어쓰던 때와 같다. 프로젝트 C++와 에셋에서 컨텍스트의 SourceObject를 읽는 곳은 없다.

**행 구조체**

`FWxAbilityTableRow`에 컬럼을 더한다. 기존 쿨다운·비용·표시 컬럼은 그대로 둔다.

| 필드 | 형식 | 비고 |
|---|---|---|
| `Type` | `TSubclassOf<UWxAbilityBase>` | 추상 클래스는 목록에 나오지 않는다. `UWxAbility_PlayMontageOnce`는 `DisallowedClasses`로 뺀다 |
| `Tag` | `FGameplayTag`(`Ability`) | 한 세트 안에서 고유 |
| `InputAction` | `UInputAction` | |
| `ActivationRequirements` | `FGameplayTagRequirements` | |
| `Montage` | `UAnimMontage` 하드 참조 | |
| `ActivationOwnedEffects` | GE 클래스 목록 | 타입이 생성자에서 정한 효과에 더해 건다 |
| `CooldownGroup` | `TSubclassOf<UWxEffect_Cooldown>` | 기존 `UWxEffect_Cooldown_*` 6개가 그룹 목록이다. 그룹을 따로 열거하지 않아 매핑 코드가 없다 |

`FWxPassiveTableRow`는 새 파일 `WxPassiveTableRow.h`에 둔다. 컬럼은 `Tag`, `TriggerEvents`(태그 컨테이너), `ActivationRequirements`, `Effects`(GE 클래스 목록)다.

**타입 클래스**
- `UWxAbilityBase`를 `NotBlueprintable`로 바꿔 GA_를 다시 만들 수 없게 한다. 타입은 C++ 클래스뿐이다.
- 지우는 프로퍼티:
  - `AbilityDataRow`, `ActivationInputAction`
  - 몽타주 프로퍼티 전부: `ComboMontage`, `UltimateMontage`, `DodgeMontage`, `GuardMontage`, `GuardReactMontage`, `HitReactMontage`, `DeathMontage`, `GroggyMontage`, `FinisherMontage`, `UseMontage`
  - `UWxAbility_Passive::TriggeredEffects`, `UWxAbility_LockOn::TargetingPreset`
- 몽타주는 기반의 `GetMontage()`(행의 `Montage`)로 읽는다.
- 쿨다운 GE 클래스 기본값(Skill의 `Skill_1`, Dodge, Ultimate)을 지운다. 그룹은 행이 정한다.
- 타입 규칙 값(`ActivationGroup`, 클래스의 `ActivationOwnedEffects`)은 생성자에서만 정하므로 편집 지정자를 뗀다.
- 공격을 넷으로 나눈다. `UWxAbility_Attack`은 콤보 로직을 가진 추상 기반으로 두고, 같은 파일에 구체 클래스 넷을 둔다(`WxEffect_Cooldown.h` 선례).

| 클래스 | 에셋·소유 태그 | 규칙 |
|---|---|---|
| `UWxAbility_Attack_Light` | `Ability.Attack.Light` | 금지 `Movement.InAir`, `Ability.Dodge` |
| `UWxAbility_Attack_Heavy` | `Ability.Attack.Heavy` | `Ability.Attack.Light` 취소, 금지 `Movement.InAir`, `Ability.Dodge` |
| `UWxAbility_Attack_Air` | `Ability.Attack.Air` | 필요 `Movement.InAir` |
| `UWxAbility_Attack_DodgeCounter` | `Ability.Attack.DodgeCounter` | 필요 `Ability.Dodge`, 금지 `Movement.InAir` |

- 위 규칙은 지금 GA_ 값에서 `Master.*`를 뺀 것과 같다. `Master.*`는 행의 발동 조건으로 옮긴다.
- `UWxAbility_Skill`의 에셋·소유 태그를 `Ability.Skill.1`에서 `Ability.Skill`로 바꾼다. 번호는 행 태그가 맡는다. HUD 스킬 슬롯(`WBP_PlayerSkills`)은 이미 `Ability.Skill`로 지목한다(MCP로 확인).
- `Abstract`를 떼는 클래스: Attack 구체 넷, Skill, Pattern, Dodge, Guard, GuardReact, HitReact, Finisher, Interact, UseItem. Death·Groggy·LockOn·Sprint·Ultimate는 이미 구체 클래스다.
- `UWxAbility_Passive`의 부모를 `UGameplayAbility`로 바꾼다.
  - 기반의 액션 행 조회(비용·쿨다운·발동 조건·표시·입력)가 패시브 스펙에서 돌 이유가 없다. 패시브가 기반에서 쓰던 것은 인스턴싱 정책뿐이라 생성자에서 직접 정한다.
  - 발동 조건은 `DoesAbilitySatisfyTagRequirements`에서 패시브 행을 본다. 효과는 행의 `Effects`를 쓰고, 공격 1회당 한 번 지급하는 규칙은 그대로 둔다.
- 락온 타게팅 프리셋은 `UWxCombatDeveloperSettings::LockOnTargetingPreset`(소프트 참조)로 옮기고, 쓸 때 `LoadSynchronous`한다(`UWxUIDeveloperSettings` 선례). `TP_LockOn`은 `/Game/Character` 아래라 `DirectoriesToAlwaysCook`으로 쿠킹된다.
- `UWxAbility_Ultimate` 주석을 고친다. 관전 머신이 컷신을 로드 대기 없이 트는 근거가 GA_ 클래스에서 "캐릭터 → 세트 → 테이블 → 몽타주" 하드 참조로 바뀐다.

**행 값을 읽는 곳**

| 곳 | 바꾸는 것 |
|---|---|
| `UWxAbilityBase` | 표시(`IWxUIData`)·충전·쿨다운 시간·쿨다운 GE(행의 그룹, 시간이 0이면 nullptr)·충전 판정·활성 중 효과(타입 효과 + 행 효과)를 행에서 읽는다. `DoesAbilitySatisfyTagRequirements`는 타입 조건 뒤에 행 조건을 보고, `Effect.IgnoreAbilityTags`와 콤보 창 우회를 행 조건에도 적용한다. `OnGiveAbility`는 쿨다운 시간은 있는데 그룹이 없는 행을 행 태그와 함께 오류로 남긴다 |
| `UWxMMC_Cost`, `UWxMMC_CooldownDuration` | 컨텍스트의 `GetAbility()`(CDO) 대신 `GetAbilityInstance_NotReplicated()` |
| `UWxAbilitySystemComponent` 입력 라우팅 3곳 | CDO의 IA 대신 `FindAbilityRow(Spec)`의 IA. 입력 바인딩 목록(`GetAbilityInputActions`)은 세트의 행에서 모은다 |
| `UWxInputBufferComponent` | IA는 행에서, 발동 그룹은 CDO(타입 규칙)에서 읽는다 |
| `UWxViewModel_Ability`(WxUI) | 물고 있는 대상을 CDO에서 스펙의 기본 인스턴스로 바꾼다. 후보 조건·표시·쿨다운 태그·비용·발동 판정·발동을 모두 인스턴스로 한다. 슬롯 매칭은 지금처럼 타입 태그다 |
| `UWxInteractionScannerComponent`(WxWorld) | `CanActivateAbility`를 CDO 대신 기본 인스턴스로 부른다. 설계 인계 목록에 없던 곳이다 |
| `AWxEnemyCharacter` 처형 문구 | 그대로 둔다. 코드 기본값인 타입 값을 CDO에서 읽는다 |

**세트**
- `GrantedAbilities`를 `Abilities`(행 핸들 목록, `RowType`=`FWxAbilityTableRow`)와 `Passives`(`RowType`=`FWxPassiveTableRow`)로 바꾼다. 기존 `WxDataTableRowHandleCustomization`이 배열 원소에도 행 미리보기를 붙인다.
- 부여 순서가 세트 순서다. 입력 라우팅은 그 순서로 시도해 처음 성공한 것을 쓴다.
- `Type`이나 `Tag`가 없는 행, 같은 세트에 이미 있는 태그의 행은 오류를 남기고 부여하지 않는다.

**BT(WxAI는 계속 WxCombat에 의존하지 않는다)**
- 스펙을 고르는 조건을 "CDO의 에셋 태그"에서 "CDO의 에셋 태그 또는 스펙의 `DynamicSpecSourceTags`"로 넓힌다. 한 필드에서 타입 태그와 행 태그를 모두 고른다.
- 발동은 지금처럼 처음 성공한 하나만 한다(사용자 결정).
- BT 데이터는 바꾸지 않는다. 지금 값(`Ability.Skill.1~3`, `Ability.Pattern.1~3`, `Ability.Death` 등)이 행 태그나 타입 태그로 그대로 행을 가리킨다.

| 노드 | 변경 |
|---|---|
| `UWxBTTask_ActivateAbility` | 매칭에 `DynamicSpecSourceTags`를 더한다 |
| `UWxBTDecorator_ObserveAbility` | 같다. 발동 콜백은 인스턴스의 스펙을, 종료 콜백은 핸들로 찾은 스펙을 본다 |
| `UWxBTService_MirrorMovement` | 발동 매칭에 행 태그를 더한다. 유지 판정은 소유 태그 대신 활성 스펙의 태그(에셋·행)를 본다 |
| `UWxBTTask_MirrorAbility` | 따라 부여할 때 클래스·레벨과 함께 `SourceObject`(세트)·`DynamicSpecSourceTags`를 복사해 같은 행이 되게 한다. 제외 매칭에 행 태그를 더한다 |

**디버그**
- 몽타주 섹션 경고, 부여 오류, 쿨다운 그룹 오류, 분신 따라하기 로그에 행 태그를 남긴다.
- 디버그 치트는 두지 않는다(사용자 결정). 엔진 `AbilityListGranted`는 클래스 이름만 찍으므로 같은 타입의 행은 로그로 가른다.

**태그**
- 새 태그 8개를 `WxGameplayTags`에 네이티브로 추가한다(Q-태그 B). 한 세트 안에 같은 타입 행이 여럿인 곳에만 필요하다.
  - HGTest 변형: `Ability.Attack.Light.1`·`.2`, `Ability.Attack.Heavy.1`·`.2`, `Ability.Ultimate.1`·`.2`
  - 공용 피격 3종: 넉 계열은 `Ability.HitReact`를 쓰고 `Ability.HitReact.KnockUp`·`.Normal`을 추가
- `Ability.Skill.1~4`, `Ability.Pattern.1~3`은 그대로 행 태그로 쓴다. 지우는 태그는 없다.

**데이터 전환(Q-단계 A에 따라 2단계에 넣는다)**

행 이름은 GA_ 이름에서 접두사 `GA_`를 뗀 것으로 옮긴다. 이후에는 자유롭게 바꿔도 된다.

| 테이블(`/Game/DesignerTables/`) | 행 이름 → 행 태그 |
|---|---|
| `DT_Ability_Shared` | Shared_Dodge → `Ability.Dodge`, Shared_Guard → `Ability.Guard`, Shared_GuardReact → `Ability.GuardReact`, Shared_HitReact → `Ability.HitReact`, Shared_HitReact_KnockUp → `Ability.HitReact.KnockUp`, Shared_HitReact_Normal → `Ability.HitReact.Normal`, Shared_Death → `Ability.Death`, Shared_Groggy → `Ability.Groggy`, Shared_Finisher → `Ability.Finisher`, Shared_LockOn → `Ability.LockOn`, Shared_Sprint → `Ability.Sprint`, Shared_Interact → `Ability.Interact`, Shared_UseItem → `Ability.UseItem` |
| `DT_Ability_HGTest` | HGTest_Attack_Light_1·2 → `Ability.Attack.Light.1`·`.2`, HGTest_Attack_Heavy_1·2 → `Ability.Attack.Heavy.1`·`.2`, HGTest_Attack_Air → `Ability.Attack.Air`, HGTest_Attack_DodgeCounter → `Ability.Attack.DodgeCounter`, HGTest_Skill_1~3 → `Ability.Skill.1~3`, HGTest_Ultimate_1·2 → `Ability.Ultimate.1`·`.2`, Minion_Attack_Heavy → `Ability.Attack.Heavy`, Minion_Skill_1·2 → `Ability.Skill.1`·`.2` |
| `DT_Ability_TemplatePlayer` | Template_Attack_Light → `Ability.Attack.Light`, Template_Attack_Heavy → `Ability.Attack.Heavy`, Template_Attack_Air → `Ability.Attack.Air`, Template_Attack_DodgeCounter → `Ability.Attack.DodgeCounter`, Template_Skill → `Ability.Skill.1`, Template_Ultimate → `Ability.Ultimate` |
| `DT_Ability_TemplateEnemy` | Template_Pattern_1·2 → `Ability.Pattern.1`·`.2` |
| `DT_Ability_Soldier` | Soldier_Pattern_1~3 → `Ability.Pattern.1~3` |
| `DT_Passive` | HGTest_Passive, Template_Passive → 둘 다 `Ability.Passive`(세트가 달라 겹쳐도 된다) |

- 값은 이번에 읽은 GA_ 41개와 `DT_Ability` 21행을 그대로 옮긴다. 행 발동 조건은 GA_의 `Master.*` 조건이다(HGTest 9행).
- 분신 행은 HGTest 행과 태그가 겹치지만(`Ability.Skill.1` 등) 세트가 달라 같은 테이블에 둔다.
- 사망 셋(`GA_Shared_Death`, `GA_Minion_Death`, `ABS_Sandbag`의 네이티브 `UWxAbility_Death`)은 값이 같아(몽타주 없음) Shared_Death 하나로 모은다. `BP_Minion`은 `ABS_Minion`과 `ABS_Shared_Enemy`가 사망을 두 번 주고 있으므로 `ABS_Minion`에서 뺀다.
- 세트 9개는 순서를 지켜 행 목록으로 바꾼다. `ABS_Doppelganger`는 지금처럼 비워 둔다(따라하기가 부여한다).
- 지우는 에셋: GA_ 41개(세트 밖 참조 없음을 확인했다), `DT_Ability`. GA_만 있던 폴더 5개(사망 2, 상호작용, 락온, 질주)는 함께 정리한다. 나머지 폴더에는 행이 가리키는 몽타주·효과·컷신이 남는다.
- 기존 결함은 지금 동작대로 옮기고 기획 확인 대상으로 남긴다.
  - HGTest_Attack_Heavy_2: 행의 5초 쿨다운이 GE가 없어 적용되지 않고 있다. 그룹 목록에 공격용 그룹이 없으므로 0초로 옮긴다.
  - Shared_Sprint: 행이 없어 진입 비용 0이던 것을 그대로 둔다.
  - `Title`이 비어 있고 `Description`이 한국어인 상태를 그대로 옮긴다.
- 범위 밖으로 그대로 두는 것: `AM_Minion_Attack_Heavy`는 참조처가 없다. 분신 강공은 `626c94d61`부터 HGTest 강공 몽타주를 쓴다.

**동작 변화**
- 분신 강공(Minion_Attack_Heavy)에 Heavy 타입 규칙(공중·회피 중 금지)이 새로 걸린다. 지금 GA_에는 금지 태그가 없다.
- 새 순정 이탈은 없다. 행 태그가 엔진의 태그 API 대상이 아니라는 기존 제약만 남는다.

**검증**
- 빌드: Development, DebugGame.
- MCP로 확인한다: 모든 행의 `Type`과 `Tag`, 세트의 행 핸들 해석과 세트 안 태그 중복, BT 값이 가리키는 행, GA_를 가리키는 참조가 남지 않았는지.
- 부여 로그로 캐릭터별 스펙의 행 태그를 확인한다.
- 플레이 확인 대상:
  - 플레이어: 평타 콤보, 강공, 공중 공격, 회피 반격, 스킬 세 변형(분신·도플갱어 유무), 궁극기 두 변형(컷신), 회피 충전, 가드·가드 반응, 피격 세 종류, 처형, 락온, 질주, 상호작용, 아이템
  - HUD: 아이콘, 쿨다운, 비용, 변형 전환
  - 적: 솔저·템플릿 패턴, 그로기, 사망, 샌드백
  - 분신: 주인의 스킬 1·2, 강공, 평타 따라 쓰기
  - 도플갱어: 따라 쓰기, 스킬 3 마주보기

**결정 기록**(2026-09-24, 모두 사용자 결정)
- **Q-단계(2·3단계 경계): A.** 2단계 코드는 GA_의 몽타주·IA·행 프로퍼티와 세트의 `GrantedAbilities`를 지우므로, 코드만 올리면 플레이 확인을 할 수 없다. 그래서 2단계에 데이터 전환과 GA_ 삭제까지 넣는다. 커밋은 코드·데이터·삭제로 나누고 확인 뒤 한 번에 푸시한다. 3단계는 없어지고 4단계(검증기)가 남는다. 기각: 이중 경로(B, 3단계에 지울 임시 분기), 코드만 먼저(C, 확인 시점만 늦음).
- **Q-태그(새 태그 등록 위치): B, C++ 네이티브.** 행을 추가하거나 태그를 바꿀 때 코드 수정과 빌드가 필요하다. 에디터는 ini에서 온 태그만 이름 바꾸기·지우기를 허용한다(`SGameplayTagPicker::CanModifyTag`). 리디렉트는 태그 정의 위치와 무관하게 ini에서 읽힌다(`GameplayTagRedirectors.cpp`). 처음 설명에서 "B는 리디렉트를 쓸 수 없다"고 한 것은 틀려서 고쳤다. 기각: A(`DefaultGameplayTags.ini`, 추천했던 안).
- **행 식별 방식.** 처음 설계는 새 루트 `AbilityId`의 행 ID 태그(행 이름 = 태그, 두 테이블을 통틀어 전역 고유)였다. 사용자가 새 태그 대신 기존 `Ability` 태그를 쓰자고 했다. 이어 행 이름은 자유롭게 하고 태그 중복을 허용하자고 했다. 스펙이 자기 행을 찾을 열쇠가 태그뿐이라 "한 세트 안에서만 고유"로 합의했다. 태그를 완전히 겹치게 허용하려면 스펙과 행을 잇는 표를 따로 복제해야 해서 기각했다. 에셋 태그를 곧 ID로 쓰는 안은 기각한 대안에 적었다.
- **BT 발동: 처음 성공한 하나.** "맞는 것 모두 실행"은 BT 태스크에 여러 어빌리티의 종료 규칙이 필요하고, 지금 데이터에서는 결과가 같다.
- **디버그 치트 제외.** 별도 장치를 최소화한다.

### 2단계 결과

- 설계대로 코드와 데이터를 바꿨다. 빌드는 Development와 DebugGame 모두 성공했다.
- 설계에 없던 것은 두 가지다.
  - 타입의 튜닝 값(락온 7개, 질주 2개, 넉업 속도, 상호작용 사거리, 처형 문구)에서도 편집 지정자를 뗐다. BP 파생이 막혀 편집할 곳이 없기 때문이다. 지운 GA_가 이 값들을 덮어쓴 적이 없음을 HEAD의 uasset에서 확인했다. 그래서 코드 기본값이 곧 이전 값이다.
  - 정상 부여는 로그로 남기지 않는다(오류만 남긴다). 부여 결과는 PIE에서 ASC의 스펙 목록을 MCP로 읽어 확인했다.
- 데이터:
  - 테이블 6장을 만들었다: `DT_Ability_Shared` 13행, `DT_Ability_HGTest` 14행, `DT_Ability_TemplatePlayer` 6행, `DT_Ability_TemplateEnemy` 2행, `DT_Ability_Soldier` 3행, `DT_Passive` 2행. 쿨다운·비용·표시 값은 `DT_Ability`에서 복사했다.
  - 세트 8개를 행 목록으로 바꿔 저장했다. `ABS_Doppelganger`는 이미 비어 있어 바뀐 것이 없다.
  - 락온 타게팅 프리셋은 `DefaultGame.ini`에 넣었다.
  - GA_ 41개와 `DT_Ability`를 지웠다. 지우기 전에 참조가 세트뿐임을 확인했다. 지운 뒤에는 Content의 uasset·umap 바이너리에 두 이름이 남지 않았음을 확인했다. GA_만 있던 폴더 5개도 함께 없어졌다.
  - 에디터의 Git 연동이 새 테이블을 add, 지운 에셋을 rm으로 인덱스에 올려 두었다. 커밋을 나눌 때 인덱스를 다시 맞춘다.
- MCP 검증 결과:
  - 모든 행의 타입·행 태그·발동 조건(HGTest 9행의 `Master.*`)·몽타주·효과·쿨다운 그룹이 GA_에서 읽은 값과 같다.
  - 세트 9개의 행 핸들이 모두 해석되고, 세트 안에서 행 태그가 겹치지 않는다.
  - BT 4개의 어빌리티 태그 값 20개(발동 13, 관찰 5, 따라 움직이기 1, 따라 쓰기 1)가 전환 전과 같은 스펙을 가리킨다. `BT_Minion`의 `Ability.Attack.Light`는 전환 전에도 분신에 그 어빌리티가 없어 발동하지 않았다.
  - PIE(`LV_DevCombat`): 플레이어(템플릿), 솔저, 템플릿 적 2, 샌드백의 스펙이 모두 올바른 세트·행 태그와 인스턴스를 가졌다. 패시브 스펙에는 트리거가 붙었다. 부여 오류는 없었다. HUD에 스킬·궁극기·회피 충전·아이템 아이콘이 행 값대로 나왔다.
  - PIE의 경고 3종은 이전 세션 로그에도 있던 것이다: `ABS_Soldier`의 속성 행 `Enemy`가 `DT_CharacterAttribute`에 없음, 보스 명판 VM 초기화 실패, 획득 목록 null 항목.
  - HGTest·분신·도플갱어는 이 맵에 없어 PIE로 확인하지 못했다. 플레이 확인 대상에 남는다.
- 발견한 기존 데이터 결함(수정하지 않음): `ABS_Soldier`가 가리키는 속성 행 `Enemy`가 `DT_CharacterAttribute`에 없어 솔저는 세트의 속성 초기화를 건너뛴다.
- 사용자 확인(2026-09-24): "잘 되네요". 항목별 확인 범위는 따로 받지 않았다. 사용자가 제출(커밋·푸시)은 보류하고 4단계로 이어가라고 했다. 2단계 변경은 미커밋으로 남아 있다.

### 4단계 설계: 검증기

기준은 2단계 작업 트리(미커밋)다. 조사 방법은 세 가지다: 엔진 DataValidation 경로 대조, 행이 가리키는 몽타주 31개를 MCP로 읽기, 타입 코드 읽기. 규칙 번호는 설계 인계 사항의 "검증기가 잡을 것" 순서다.

**검증이 도는 때**
- 순정 DataValidation 플러그인(켜져 있다)이 에셋을 저장할 때와 콘텐츠 브라우저의 데이터 검증 메뉴에서 돈다. 저장 시 검증은 기본으로 켜져 있다(`UDataValidationSettings::bValidateOnSave`). 오류가 있어도 저장은 막지 않고 메시지 로그에 남긴다.
- 테이블: 엔진이 행마다 `FTableRowBase::IsDataValid`를 부르고 메시지에 행 이름을 붙인다(`DataTable.cpp:418`). `FWxAbilityTableRow`와 `FWxPassiveTableRow`가 오버라이드한다.
- 세트: `UWxAbilitySet::IsDataValid`를 오버라이드한다.
- 몽타주: 엔진 클래스라 오버라이드할 수 없다. 그래서 WxEditor에 `UEditorValidatorBase` 파생 하나를 둔다. 이 검증기는 그 몽타주를 가리키는 어빌리티 테이블 행을 에셋 레지스트리로 찾아, 행 타입의 몽타주 규칙을 돌린다.
- 규칙 코드는 모두 WxCombat의 `WITH_EDITOR` 구간에 둔다. WxEditor 검증기는 행을 찾아 부르기만 한다.

**규칙과 위치**

| 번호 | 규칙 | 위치 | 심각도 |
|---|---|---|---|
| 5 | 타입이나 행 태그가 빈 행 | 행 | 오류 |
| 2 | 쿨다운 시간은 있는데 그룹이 없는 행 | 행 | 오류 |
| 8 | 행 태그가 빈 패시브 행, 트리거끼리 조상 관계인 패시브 행 | 패시브 행 | 오류 |
| 5 | 풀리지 않는 행 핸들, 한 세트 안에서 겹치는 행 태그 | 세트 | 오류 |
| 4 | 발동 조건이 배타적이지 않은 같은 입력의 행 | 세트 | 경고 |
| 3 | 쿨다운 시간이나 충전 수가 다른 같은 그룹의 행 | 세트 | 경고 |
| 7 | 세트의 피격 행 몽타주들이 다 덮지 못한 반응 섹션 | 세트 | 경고 |
| 1·6·7·9·10 | 타입별 몽타주 규칙(아래) | 행 → 타입 CDO, 몽타주 검증기 | 아래 |

- 배타성 판정: 한쪽이 요구하는 태그를 다른 쪽이 막으면 배타적이다. 부모 태그로 막는 경우도 포함한다.
  - 요구하는 태그는 타입의 `ActivationRequiredTags`와 행의 `RequireTags`다. 막는 태그는 타입의 `ActivationBlockedTags`와 행의 `IgnoreTags`다.
  - 행의 `TagQuery`는 보지 않는다. 지금 쓰는 행이 없다.
  - 엔진이 두 태그 목록을 protected로 두므로 판정은 `UWxAbilityBase` 멤버가 한다.
- 경고로 둔 셋은 의도일 수 있다. 입력이 겹치면 세트 순서가 결과를 정한다. 그룹 값 차이와 반응 누락(예: 넉다운 반응이 없는 보스)은 기획 선택일 수 있다.

**타입별 몽타주 규칙**

공통 규칙은 몽타주가 있으면 모든 타입에 적용한다.
- 독립 섹션: 타입이 재생을 직접 시작하는 섹션이다. 기본은 첫 섹션이고, 타입이 바꾼다.
- 독립 섹션이 링크로 다른 독립 섹션에 이어지면 오류다. 한 번 재생에 두 섹션이 나간다.
- 어디에서도 닿지 않는 섹션은 경고다. 닿는 섹션은 독립 섹션, 거기서 링크로 이어지는 섹션, 착지 섹션(`Grounded`)이다. 예약 이름의 오타를 잡는다(예: 회피 방향 오타는 지금 조용히 Forward로 폴백한다).
- 노티파이 경계 규칙(인계 항목 9·10을 단계 섹션에서 모든 독립 섹션으로 넓혔다):
  - 독립 섹션의 시작 경계(0초 제외)를 넘는 노티파이 구간은 오류다. 그 섹션을 새로 트는 인스턴스가 첫 틱에 이 구간을 이어받는다.
  - 그 경계에 정확히 놓인 단발 노티파이도 오류다. 엔진은 섹션 시작에 놓인 노티파이를 앞 섹션 끝으로 잡는다(`AnimMontage.cpp:887`).

| 타입 | 몽타주 | 독립 섹션 | 추가 규칙(모두 오류) |
|---|---|---|---|
| Attack 넷·Skill | 필수 | 번호 섹션. 없으면 첫 섹션 | 번호는 1부터 빈틈 없이 잇는다. 마지막 단 전까지 단마다 콤보 창이 있어야 한다 |
| Pattern | 필수 | 같다 | 번호는 1부터 빈틈 없이 잇는다 |
| Attack_Air | 필수 | 같다 | 루프 섹션(자기 자신으로 링크)이 있으면 `Grounded`가 있어야 한다 |
| Dodge | 필수 | 방향 8개, `Backstep`, `Success`+방향 8개 | 방향 섹션이 있으면 `Forward`, `Success` 섹션이 있으면 `SuccessForward`가 있어야 한다(폴백 대상) |
| GuardReact | 필수 | 섹션 4개 | 4개 모두 있어야 한다 |
| HitReact | 필수 | 반응 섹션: `HitReact` 태그 자식의 끝 이름과 `Parry` | 반응 섹션이 하나는 있어야 한다. `KnockUp`이 있으면 `Grounded`가 있어야 한다 |
| Guard·Groggy·Ultimate | 필수 | 첫 섹션 | — |
| Finisher | 필수 | 첫 섹션 | `FinisherVictim`·`FinisherDamage` 노티파이가 있어야 한다 |
| UseItem | 필수 | 첫 섹션 | `UseItem` 노티파이가 있어야 한다 |
| Death | 선택 | 첫 섹션 | — |
| LockOn·Sprint·Interact | 없음 | — | — |

- "필수"는 몽타주가 없으면 어빌리티가 곧바로 끝나는 타입이다. 몽타주가 비면 오류다.
- 반응 섹션 이름은 태그 트리에서 읽으므로 반응 태그가 늘면 규칙도 따라간다.

**코드 구조**
- `UWxAbilityBase`(모두 `WITH_EDITOR`):
  - `IsMontageDataValid(Montage, Context)` 가상 함수. 행 검증과 몽타주 검증기가 타입 CDO에 대고 부른다. 기본은 몽타주가 있을 때 첫 섹션으로 공통 규칙만 본다. 위 표의 타입이 오버라이드한다.
  - 공통 규칙과 단계 규칙은 protected 정적 도우미로 둔다. 단계 규칙은 기존 `GetComboStageCount`·`GetComboStageSection` 옆에 둔다.
  - 입력 배타성 판정 멤버 함수.
- 섹션 이름을 실행 코드와 검증이 함께 쓰도록 한 곳에 모은다.
  - 가드 반응 섹션 4개와 회피의 `Backstep`·`Success` 접두사를 각 타입의 정적 상수로 옮긴다. 지금은 코드 곳곳의 문자열이다.
  - 착지 섹션 `Grounded`는 `UWxCharacterMovementComponent`의 익명 namespace 상수를 `UWxAbilityBase`로 옮기고, 이동 컴포넌트가 그 상수를 쓴다. 착지 섹션은 어빌리티 몽타주 규약이고, 이동 컴포넌트는 점프만 한다.
- 행 구조체 두 개는 지금 헤더뿐이다. `IsDataValid` 구현을 위해 .cpp를 더한다(인라인 정의 금지).
- `UWxAbility_HitReact`: 반응 섹션 이름 목록 정적 함수. HitReact 자신의 검증과 세트의 반응 누락 검사가 함께 쓴다.
- WxEditor: `UWxAbilityMontageValidator`를 새로 만들고, `WxEditor.Build.cs`에 `DataValidation`·`AssetRegistry`·`WxCombat` 의존을 더한다.

**지금 데이터에서 예상되는 결과**(MCP로 읽은 몽타주 31개와 행 38개를 손으로 대조했다)
- 오류 9개: `AM_Shared_Dodge`의 구간 9개가 독립 섹션 시작 경계를 넘는다. 1-2단계 결과에 적은 결함이다. 방향 섹션 무적 7개와 극한 회피 슬로모션 2개가 자기 섹션보다 0.0001~0.0002초 앞에서 시작한다.
- 경고 1개: `ABS_HGTest`의 `HGTest_Skill_2`(분신 필요)와 `HGTest_Skill_3`(도플갱어 필요)은 같은 입력이다. 둘이 함께 있으면 두 행 모두 성립하고, 지금은 세트 순서대로 Skill_2가 나간다.
  - 분신과 도플갱어는 함께 있을 수 있다. 분신은 약공 1의 4타, 회피 반격 2타, 스킬 1이 부르고, 도플갱어는 궁극기 1이 부른다(MCP로 노티파이 값 확인).
  - 기획 확인 대상이다.
  - 정정(2026-09-25): 위 판단은 틀렸다. 사용자가 "분신과 도플갱어가 함께 있을 수 없습니다"라고 했고, `BP_Minion`의 `WxMinionComponent`로 확인했다. 소환 조건이 `Master.Doppelganger`를 막고, 취소 조건이 `Master.Doppelganger`에서 분신을 없앤다. 소환 노티파이만 보고 소환물 쪽 정책을 보지 않았던 것이 원인이다. 처리는 4단계 결과에 있다.
- 나머지는 통과할 것으로 본다.

**검증**
- 빌드: Development, DebugGame.
- 에디터에서 테이블 6장, 세트 9개, 몽타주 31개를 데이터 검증으로 돌려 위 예상과 대조한다.
- 규칙마다 일부러 어긋나게 만든 임시 사본으로 잡히는지 확인하고, 확인한 뒤 지운다.

**결정 기록**(2026-09-25, 사용자 결정 "둘 다 A로 해주세요")
- **Q-범위: A, 세트 단위.** 입력 겹침·쿨다운 그룹 값 차이·피격 반응 누락은 세트를 저장할 때 세트 안에서만 본다. 코드가 한 곳이다. 세트 사이의 겹침(예: 공용 세트의 회피와 캐릭터 세트의 특수 회피가 같은 입력)은 잡지 못한다. 지금 데이터에는 그런 경우가 없다. 기각: B(캐릭터 BP의 ASC에서 세트 전체를 보기. 세트만 저장하면 돌지 않고 캐릭터마다 같은 메시지가 반복된다).
- **Q-회피: A, 이번에 고친다.** `AM_Shared_Dodge`의 구간 9개의 시작을 자기 섹션 시작 + 0.0001초로 옮긴다. 몽타주 도구에 시작 맞춤을 추가한다. 앞 방향 회피 끝에 붙던 여분 무적·슬로모션이 없어진다. 기각: B(경계 규칙을 단계 섹션에만 적용해 회피를 검사 밖에 두기).
- 나머지 설계는 제시한 대로 확정했다.

### 4단계 결과

- 설계대로 구현했다. 코드 위치는 다음과 같다.
  - 규칙: `UWxAbilityBase`의 `IsMontageDataValid`와 도우미, 타입 12개의 `ValidateMontage` 오버라이드
  - 행: `FWxAbilityTableRow`·`FWxPassiveTableRow`의 `IsDataValid`(.cpp 추가)
  - 세트: `UWxAbilitySet::IsDataValid`
  - 몽타주 검증기: WxEditor `UWxAbilityMontageValidator`
  - 몽타주 도구: `SnapNotifyStartsToSections`
- 설계에서 바꾼 것: 단발 노티파이의 경계 판정과 콤보 창의 단 판정을 링크 시각이 아니라 실제 트리거 시각으로 한다.
  - 병합 도구가 옮긴 노티파이 상당수는 링크 시각이 섹션 시작과 정확히 같고, 원본에서 가져온 "뒤로" 트리거 오프셋(+0.0001초, `UE_KINDA_SMALL_NUMBER`)을 가진다. 그래서 실제로는 자기 섹션에서 불린다.
  - 링크 시각만 보면 이런 정상 노티파이를 오류로 잡는다. 몽타주 설명 도구의 `time`도 트리거 시각이다.
- 회피 몽타주 수정(Q-회피 A):
  - `AM_Shared_Dodge`의 구간 9개의 시작 트리거를 자기 섹션 시작 + 0.0001초로 옮겼다. 끝 트리거 시각은 그대로다. 나머지 노티파이 40개는 바뀌지 않았다.
  - 저장할 때 몽타주 검증기가 돌았고 메시지가 없었다.
  - 첫 시도에서는 도구가 링크 시각으로 대상을 골라 정상 구간 8개의 끝까지 0.0001초 당겼다. 저장하지 않은 채 에디터를 닫아 버리고, 도구를 트리거 시각 기준으로 고친 뒤 다시 했다.
- 빌드: Development와 DebugGame 모두 성공했다.
- 지금 데이터 검증: DataValidation 커맨드릿(`-run=DataValidation -AssetType=<클래스 경로>`)을 저장 없이 돌렸다.
  - 어빌리티 테이블 5장과 `DT_Passive`가 통과했다.
  - 세트는 `ABS_HGTest`의 경고 1개(`HGTest_Skill_2`·`HGTest_Skill_3` 입력 겹침)뿐이다. 설계에서 예상한 결과와 같다.
  - 몽타주 45개가 몽타주 검증기를 거쳐 메시지 없이 통과했다.
  - 커맨드릿 종료 코드는 1이었다. 원인은 오류 로그 두 개다: 에디터가 쓰는 8000 포트의 바인딩 실패, 기존 `DT_Dialogue`가 없는 `AM_Death`를 소프트 참조하는 것. 이번 작업과 무관하다.
- 반례 시험:
  - 임시 폴더에 몽타주 사본 7개(섹션 이름 오타, 착지 섹션 없음, 번호 빈틈, 경계에 놓인 단발 노티파이, 수정 전 회피, Forward 없음), 테이블 2장, 세트 1개를 만들어 저장 시 검증을 돌렸다.
  - 규칙마다 기대한 오류·경고가 나왔다. 수정 전 회피 사본에서는 정확히 구간 9개가 잡혔다. 몽타주를 다시 저장하면 몽타주 검증기가 역참조로 행 타입의 규칙을 돌렸다.
  - 시험 뒤 임시 에셋을 모두 지웠다.
  - 반례로 확인하지 못한 규칙은 "독립 섹션끼리의 링크"다. 섹션 링크를 바꿀 도구가 없다. 지금 몽타주 45개에서 잘못 잡는 것이 없음만 확인했다.
- 알려진 중복 메시지: 번호가 이어지지 않는 섹션은 단계 규칙의 오류와 "재생되지 않는다" 경고가 함께 나온다.
- 발견한 기존 데이터 결함(수정하지 않음): `DT_Dialogue`가 없는 `/Game/Character/Shared/Abilities/AM_Death`를 소프트 참조한다.
- 사용자가 에디터로 확인했다(2026-09-25). 이어서 준 판단은 두 가지다: "분신과 도플갱어가 함께 있을 수 없습니다", "DT_Dialogue는 지금은 무시하세요". 그 뒤 고친 것은 세 가지다.
  - `HGTest_Skill_3`의 발동 조건에 `IgnoreTags` `Master.Minion`을 더했다. 도플갱어 변형인 약공 2·궁극기 2와 같은 방식이다. 둘이 공존하지 않으므로 동작은 같고, 세트 경고가 없어진다.
  - 이어지지 않은 번호 섹션도 독립 섹션으로 넘겨 위의 중복 메시지를 없앴다.
  - 세트 검증이 속성 행(`AttributeInitRow`)도 "풀리지 않는 행"으로 본다. 비워 둔 속성 행은 허용한다. 이 검사가 `ABS_Soldier`의 없는 행 `Enemy`를 오류로 잡는다(2단계 결과의 기존 결함).
  - 빌드(Development·DebugGame)와 커맨드릿 재검증을 마쳤다. 경고는 없어졌고, 남은 오류는 `ABS_Soldier`의 속성 행 하나다(`DT_Dialogue` 제외).
- `ABS_Soldier` 조사 결과:
  - `Enemy` 행은 2026-09-09 커밋 `e0e3ecc51`("캐릭터 폴더 정리")에서 `TemplateEnemy`로 이름이 바뀌었다. `ABS_Soldier`만 옛 이름을 가리키고 있다.
  - 세트는 캐릭터의 `AbilitySets` 배열 순서대로 부여되고, 세트마다 속성 기본값을 덮어쓴다. 그래서 마지막 세트의 속성 행이 이긴다.
  - 캐릭터 BP의 세트 순서:
    - `BP_HGTest`·템플릿 플레이어·템플릿 적: 공용 세트 → 캐릭터 세트. 캐릭터 행이 이긴다.
    - `BP_Minion`·`BP_Soldier`: 캐릭터 세트 → 공용 적 세트. 공용 적 세트의 `TemplateEnemy`가 이긴다.
  - 그래서 솔저는 지금 `TemplateEnemy` 수치로 산다. 분신도 `Minion` 행(최대 HP 0, 공격력 40) 대신 `TemplateEnemy`(최대 HP 100, 공격력 20)로 산다. `Minion` 행은 세트가 하나뿐인 도플갱어에만 적용된다.
  - 사용자 결정(2026-09-25): A, 지금 동작 유지. `ABS_Soldier`의 속성 행을 `TemplateEnemy`로 다시 연결해 저장했고, 저장 시 검증을 통과했다. 수치는 바뀌지 않는다. 기각: B(세트 순서를 바꿔 캐릭터 행이 이기게 하고 솔저 전용 행을 만들기, 분신 수치가 바뀜).
  - 이로써 어빌리티 테이블·세트·몽타주에서 검증기가 내는 오류와 경고는 없다. `DT_Dialogue` 오류는 사용자 지시로 무시한다.
- 제출할 때 주의: `DT_Ability_HGTest`(스킬 3 조건)와 `ABS_Soldier`(속성 행)의 4단계 변경은 2단계에서 바뀐 같은 바이너리에 들어 있어 나눌 수 없다. 두 파일은 2단계 데이터 커밋에 함께 넣는다. 4단계만의 에셋 변경은 `AM_Shared_Dodge`뿐이다.

### 후속 변경 제안: 타입 칸 제거(2026-09-25)

사용자 요청: "테이블에서 AbilityType 지정을 하고 있는데, 그럼 어빌리티 태그 지정은 없어도 되는 것 아닌가요? 둘 중 하나만 설정하는 쪽으로 정리정돈하고 싶어요", "데이터 작업과 디버깅 모두 편한 방법으로 하고 싶어요".

- 제안: 행 태그만 남기고 `Type` 칸을 없앤다. 타입은 행 태그가 정한다. 행 태그 자신이나 가장 가까운 조상이 식별 태그인 구체 타입이 그 행의 타입이다.
  - 예: `Ability.Skill.2`는 `Ability.Skill`을 가진 `UWxAbility_Skill`이고, `Ability.HitReact.KnockUp`은 `UWxAbility_HitReact`다.
  - 처음 사용자가 낸 "에셋 태그가 곧 ID" 발상을 행 태그 쪽에서 살린 형태다.
- 반대(타입만 남기기)는 성립하지 않는다.
  - 스펙이 자기 행을 찾는 복제 통로는 행 태그뿐이다(2단계 결정 기록). 정정(2026-09-25): `SourceObject`가 행마다 있는 객체를 가리키게 하면 태그 없이도 행을 가리킬 수 있다. 아래 "행 찾기 대안"을 본다.
  - 같은 타입의 행이 한 세트에 여럿 있다: HGTest 스킬 3개, 공용 피격 3개, 솔저 패턴 3개.
  - BT도 `Ability.Skill.1`처럼 행 태그로 변형을 고른다.
- 확인(MCP):
  - `UWxAbilityBase` 하위 구체 타입 19개(추상 둘 제외)는 모두 식별 태그를 하나씩 갖고 서로 겹치지 않는다. 블루프린트 하위 클래스는 없다(2026-09-25 재확인).
  - 지금 행 38개 모두 행 태그로 찾은 타입이 지금 `Type` 값과 같다. 그래서 칸을 없애도 동작은 바뀌지 않는다.
- 데이터 작업: 칸 하나만 고르면 된다. 지금은 `Type`과 행 태그가 어긋나도(예: 타입은 Skill, 태그는 `Ability.Attack.Light`) 막는 곳이 없는데, 그 오류가 원천적으로 없어진다. 새 변형은 지금처럼 하위 태그를 코드에 추가한다(Q-태그 B).
- 디버깅: 실행 중 스펙에는 지금처럼 클래스(타입)와 행 태그가 모두 보인다. 태그로 타입을 찾지 못하면 테이블 검증이 오류로 알리고, 부여 로그도 같은 이유를 남긴다.
- 구현 범위:
  - `FWxAbilityTableRow::Type`을 지운다.
  - `UWxAbilityBase`에 행 태그로 타입을 찾는 정적 함수를 둔다. 엔진의 파생 클래스 목록을 훑고, 추상 클래스와 시스템이 부여하는 `UWxAbility_PlayMontageOnce`는 뺀다(지금 `Type` 칸의 `DisallowedClasses`를 대신한다).
  - 부여, 행 검증, 세트 검증, 몽타주 검증기가 이 함수를 쓴다.
  - 테이블 5장을 다시 저장해 옛 `Type` 값을 걷어낸다.
- 상태: 사용자 확정(2026-09-25): "일단은 '타입 칸 제거' 제안으로 진행합시다".
- 관련 논의: 사용자가 "테이블과 클래스, 애니메이션 몽타주 간의 암묵적인 계약이 저는 없었으면 해서요"라고 했다. 그래서 계약을 드러내는 방법 세 가지를 냈다: A 선언과 편집 화면 표시(추천), B 역할 표식, C 상황별 몽타주. 사용자는 이번에는 타입 칸 제거만 하기로 했다("일단은"). 계약을 드러내는 작업은 미결로 남긴다.
- 결과(2026-09-25):
  - Development 빌드와 커맨드릿 검증을 통과했다. 어빌리티 테이블 5장, `DT_Passive`, 세트 9개, 몽타주 44개에 오류와 경고가 없다(`DT_Dialogue` 제외).
  - 테이블 5장을 다시 저장했다. 파일에서 클래스 참조가 사라져 옛 `Type` 값이 걷혔고, 저장 시 검증도 통과했다.
  - PIE(`LV_DevCombat`): 솔저는 공용 적 5행과 패턴 3행, 템플릿 플레이어는 공용 플레이어 12행과 캐릭터 6행에 패시브가 세트 그대로 부여됐다. 부여 실패 로그는 없다. HGTest와 분신은 이 맵에 없어 세트 검증으로만 확인했다.
  - 재검토 결론 뒤 DebugGame 에디터 빌드도 통과했다(오류·경고 없음). Wiki 반영은 제출과 함께 한다.
  - Wiki 반영에 넣을 것(2026-09-25 AI 작업성 논의):
    - 값이 있는 곳: 규칙은 타입 C++, 데이터는 행, 부여와 순서는 세트, 시점 값은 몽타주 노티파이, AI 선택은 BT 태그
    - 함정: 클래스 API 금지, 행은 `GetAbilityRow()`로만 읽는다(CDO·GE 컨텍스트 `GetAbility()`는 빈 값), 행 태그는 한 세트 안에서 고유, 행 태그는 엔진 태그 API 대상이 아니다
    - `concepts/combat-finisher.md`의 `GA_Shared_Finisher` 변형 두 개 설명을 고친다
- 재검토 요청(2026-09-25): 사용자가 "태그로 타입을 찾는 코드가 런타임에서도 이렇게 동작을 하는건가 싶어요", "차라리 어빌리티 테이블에서 태그를 빼고 타입만 유지하는게 코드 파악 측면에서 낫지 않을까요?"라고 물었다.
  - 런타임: 서버가 캐릭터에 세트를 처음 부여할 때 행마다 한 번 같은 함수로 찾는다. 발동과 클라이언트는 이 함수를 지나지 않는다. 클라이언트는 클래스가 실린 스펙을 복제로 받는다.
  - 답: 태그만 빼기는 위 이유로 성립하지 않는다. 코드 파악이 목적이면 타입 칸을 되살려 타입과 태그를 둘 다 두는 쪽을 권했다. Lyra 세트와 같은 순정 방식이다(클래스를 직접 지정하고 태그를 `DynamicSpecSourceTags`에 넣는다, `LyraAbilitySet.cpp:116`). 부여에서 파생 클래스 순회가 사라진다. 대가는 대부분 행에서 태그가 타입 이름을 되풀이하는 것과, 둘이 어긋나지 않게 하는 검증 규칙이다.
  - 타입 칸 제거를 제안할 때 순정 방식에서 벗어난다는 점을 밝히지 않았다.
  - 이어진 질문: "타입(클래스)로 바꿀수 있다는거 맞죠?", "행 구분 없이 클래스 일치하는거 모두 발동시도해도 되지 않나요?". 답: 발동 시도는 이미 후보를 모두 시도한다(입력 `WxAbilitySystemComponent.cpp:136`, 피격 이벤트). 막히는 곳은 시도받은 스펙이 자기 행의 조건·몽타주·쿨다운을 읽는 단계다. HGTest 스킬 1·2·3은 클래스와 입력(`IA_Skill`)이 같고 `Master.*` 조건으로만 갈린다.
  - 이어진 질문: "행을 찾을 때 아예 RowName으로 전달하면 더 확실하게 찾을 수 있지 않나요?". 답: 행은 키 후보 고르기, 조건·쿨다운·비용 판정, 몽타주 재생에서 읽고, `LocalPredicted`라 클라이언트가 먼저 읽는다. UE 5.8 스펙의 복제 칸(`GameplayAbilitySpec.h:194`~`261`)에는 이름(`FName`) 칸이 없다. 발동 때 넘기기에는 행이 발동 전부터 필요하다. `Level`·`InputID`에 행 번호를 넣으면 엔진 의미(GE 수치 레벨, 입력 바인딩)와 부딪힌다. (세트, 태그)는 부여 거부와 세트 검증으로 늘 한 행이다.
  - 결론(2026-09-25): 사용자가 "태그는 어차피 어빌리티 클래스 안에도 있으니까, 굳이 테이블에서도 이중 관리하고 싶지 않아요"라고 했다. 타입+태그는 타입 칸이 정하는 것을 태그 칸에 또 적는 이중 관리라서 뺀다. 이중 관리가 없는 지금 구조(태그 한 칸으로 클래스를 가리키고 행 이름도 삼음)를 유지한다. Lyra의 명시 클래스 지정과 다른 점은 알린 채로 택했다.

### 지금 구조 점검(2026-09-25)

사용자 요청: "현재의 방식의 문제점은 없을까요?". 지금 틀리게 동작하는 곳은 찾지 못했다. 데이터나 클래스를 늘릴 때 조용히 틀어질 수 있는 곳은 아래와 같다.

1. 식별 태그 겹침: 두 구체 타입이 같은 식별 태그를 가지면 `FindTypeByTag`는 파생 클래스 목록 순서로 먼저 나온 쪽을 고른다. 이 순서는 보장되지 않는다. 하위 클래스가 생성자에서 태그를 바꾸지 않으면 부모 태그를 물려받아 겹친다. 막는 장치는 없다. 지금 19개 타입은 겹치지 않는다. 제안: 같은 층에서 두 타입이 걸리면 오류를 남기고 고르지 않는다.
2. 행 태그가 클래스를 정하므로 같은 태그는 모든 캐릭터에서 같은 클래스다. 한 캐릭터의 스킬 2만 새 클래스로 만들려면 그 행을 새 클래스의 태그 계열로 옮기고, 그 태그를 부르는 BT도 바꿔야 한다. 지금 구조를 택한 대가다.
3. 캐릭터 단위 중복: 한 캐릭터가 받는 여러 세트 사이에서 행 태그가 겹치는지는 부여도 검증도 보지 않는다. 겹치면 둘 다 부여되어, 피격처럼 이벤트로 도는 타입은 한 번 맞을 때 둘이 반응한다. 지금 캐릭터 7종의 세트 조합에는 겹침이 없다.
4. BT가 부르는 행 태그가 세트에 없으면 `WxBTTask_ActivateAbility`가 로그 없이 실패한다. 지금 BT 4개(`BT_Minion`, `BT_Doppelganger`, `BT_Soldier`, `BT_Template`)가 부르는 행 태그는 모두 상대 세트에 있다. 테이블 전환 전부터 있던 성질이다.
5. UI 슬롯(`UWxViewModel_Ability`)은 클래스 태그로만 어빌리티를 고르고 행 태그는 보지 않는다(`WxViewModel_Ability.cpp:184`). 같은 클래스를 다른 키로 둘 쓰는 캐릭터가 생기면 두 슬롯을 가를 수 없다. 지금은 없다. HGTest 스킬 3개는 한 키의 변형이다.
6. 작은 것: 시스템 전용 타입 제외가 `FindTypeByTag` 안에 `UWxAbility_PlayMontageOnce`로 박혀 있다. 새 변형(`Ability.Skill.5` 등)은 네이티브 태그 추가와 빌드가 필요하다(Q-태그 B).

확인했고 문제 아닌 것:
- 클래스로 스펙을 찾는 API(`FindAbilitySpecFromClass`, `TryActivateAbilityByClass`)는 코드와 에셋 어디에서도 쓰지 않는다.
- CDO로 행을 읽는 코드는 없다. 비용 계산도 인스턴스에서 읽는다(`WxEffect_Cost.cpp:40`).
- 피격 몽타주 3개의 반응 섹션은 겹치지 않는다: `Normal` / `KnockBack`·`KnockDown`·`Parry` / `KnockUp`.
- 성능: 키를 누르는 동안 프레임마다 스펙 수 × 세트 행 수만큼 행을 조회한다. 지금 규모에서는 수백 번의 해시 조회라 부담이 작다고 봤다. 측정은 하지 않았다.

상태: 사용자 판단 대기. 1번만 지금 고치고 나머지는 기록해 두기를 권했다.

**행 찾기 대안(2026-09-25)**

사용자 요청: "테이블에서 행을 태그로 찾는 부분이 자꾸 신경이 쓰입니다. 확실하게 행을 찾을 수 있는 방법 없을까요?", "저작할 때 규칙을 줄이고, 칼럼 수도 불필요하게 늘리고 싶지 않으며, 디버깅하기 쉬웠으면 해요".

- 지금 방식의 결과는 늘 한 행이다. 찾는 범위가 스펙의 세트 하나이고, 그 세트 안의 같은 태그는 부여가 거부하고(`WxAbilitySet.cpp:69`) 검증이 오류로 막는다.
- 대안 A, 행 항목 객체: 세트가 행 핸들을 담은 인스턴스 객체 배열을 갖고, 스펙의 `SourceObject`가 그 객체를 가리킨다. Lyra가 장비 인스턴스를 `SourceObject`로 싣고 읽는 것과 같은 방식이다(`LyraGameplayAbility_FromEquipment.cpp:22`).
  - 태그는 타입 선택과 BT·분신 호출 이름으로 남으므로 규칙은 세트 안 태그 유일 하나만 식별 용도에서 풀리고, 칼럼은 그대로다.
  - 대가: 새 클래스, 세트 9개 구조 변경과 이관, 세트 편집이 한 단계 무거워진다(항목마다 클래스를 고르고 펼쳐서 행을 고른다).
  - 엔진은 디스크에서 로드된 객체만 경로로 복제한다(`PackageMapClient.cpp:3153`, `Obj.cpp:6102`). 그래서 에디터를 켠 뒤 새로 넣은 항목은 재시작 전까지 원격 클라이언트에 닿지 않는다. 막으려면 항목 클래스가 이 판정을 덮어써야 한다.
- 대안 B, `InputID`에 세트 안 행 번호를 싣기: 가장 작지만 엔진의 입력 바인딩 칸을 다른 뜻으로 쓴다. 디버깅 때 번호를 세트에서 세어야 한다.
- 대안 C, SetByCaller(사용자 제안 "SetByCaller로 행을 전달하는건 어떻습니까?"): 성립하지 않는다. 스펙의 `SetByCallerTagMagnitudes`는 `UPROPERTY`가 아니라 복제되지 않는다(`GameplayAbilitySpec.h:272`). 그래서 먼저 판정하는 소유 클라이언트가 행을 모른다. 값도 태그 키의 실수라 행 이름을 담지 못하고 번호만 담는다. 용도도 어빌리티를 부여한 GE가 수치를 넘기는 통로다.
- 태그를 아예 없애고 타입 칸 하나로 가는 길(A에 더해 BT가 행 핸들로 행을 고름)은 BT·분신이 캐릭터 공통 이름(`Ability.Pattern.2` 등)을 잃는다. BT가 테이블의 행 이름에 묶이고, 행 이름 변경에 약해진다.
- A 점검(사용자 질문 "A로 했을 때 문제 있나요?"). 구현을 막는 문제는 없고, 아래가 생긴다.
  - 복제: 에디터를 켠 뒤 새로 넣은 항목은 재시작 전까지 원격 클라이언트에 닿지 않는다. 항목 클래스가 `IsNameStableForNetworking`을 덮어써야 한다. 지금 방식은 행을 더해도 객체가 새로 생기지 않아 해당이 없다. 새로 만든 세트만 두 방식 모두 같은 제약을 받는다.
  - 세트 편집: 항목마다 추가한 뒤 클래스를 고르고, 펼쳐서 행을 고른다. 행 미리보기(`WxPreviewRow`)는 항목의 행 칸으로 옮기면 유지된다.
  - 스톡 디버그 표시: Visual Logger(`AbilitySystemComponent.cpp:2038`)와 Gameplay Debugger(`GameplayDebuggerCategory_Abilities.cpp:170`)가 스펙의 출처를 객체 이름으로 보인다. 세트 이름이던 것이 자동 번호 이름(`WxAbilitySetEntry_3` 같은)으로 바뀐다.
  - 이관: 새 클래스와 세트 9개 구조 변경. 옛 배열과 새 배열을 잠시 함께 두고 옮기느라 빌드가 두 번 든다. 부여·조회·입력 수집·검증 코드를 고친다.
  - 규칙: 세트 안 태그 유일은 식별용에서 풀린다. 대신 같은 행 중복과 빈 항목을 검증해야 한다. 태그 규칙(타입 계열, 네이티브 태그)과 칼럼 수는 그대로다.
  - 엔진이 GE 컨텍스트에 싣는 `SourceObject`가 세트에서 항목으로 바뀐다(`GameplayAbility.cpp:1922`). 이것을 읽는 코드와 에셋은 없다.
  - 얻는 것: 스펙에서 행으로 바로 간다. Lyra 장비와 같은 방식이다.
  - 판단: 사용자 기준(규칙·칼럼·디버깅)에서 얻는 것이 작아 권고를 유지했다.
- 안 D(사용자 제안 "타입 칸을 부활시켜서 그것으로 클래스를 찾게 하고, 행을 찾게 하는 법은 더 나은 방법이 있지 않을까요?"): 타입 칸으로 클래스를 정한다. 행은 A(행 항목)로 찾는다. BT는 특정 행을 행 핸들로 지목한다. 이렇게 하면 테이블에서 태그 칸이 빠진다.
  - 앞서 "태그만 빼기는 성립하지 않는다"고 한 것은 행을 가리킬 통로가 태그뿐이라는 틀린 전제에서 나왔다. A가 그 통로를 대신한다.
  - 기준별: 태그 계열·세트 안 태그 유일·변형마다 네이티브 태그 추가가 사라진다. 남는 규칙은 세트에 같은 행 두 번·빈 항목 금지다(검증). 칼럼 수는 그대로다(태그 → 타입). 이중 관리가 없다. 클래스가 행에 보이고 스펙에서 행으로 바로 간다. `FindTypeByTag`와 태그 검색이 없어진다.
  - BT: 클래스 단위 선택은 지금처럼 클래스 태그로 한다(`Ability.Death`, `Ability.Attack.Heavy`, 분신 제외 목록의 `Ability.Finisher`·`Ability.Ultimate`). 행 단위로 부르는 11곳은 행 지정으로 옮긴다: `BT_Minion` 5노드(스킬 1·2, 주인 관찰 포함), `BT_Doppelganger` 1노드(주인 스킬 3 바라보기), `BT_Soldier` 3노드, `BT_Template` 2노드.
  - 약점: A의 세 가지(복제 보완, 세트 편집, 스톡 디버그 이름)를 떠안는다. BT 노드 3종(`ActivateAbility`, `ObserveAbility`, `MirrorMovement`)에 행 지정 칸을 더해 선택 방식이 둘이 된다. BT가 행 이름에 묶여 행 이름을 바꾸면 깨지므로 검증이 필요하다. 캐릭터 공통 이름이 사라진다. 지금 BT는 모두 캐릭터 전용(`BT_Template`은 `BP_Template`만 씀)이라 손해는 없다. 분신 BT가 주인의 스킬 1과 자기 스킬 1을 같은 태그로 잇던 연결은 명시적으로 바뀐다.
  - 작업량: 새 클래스, 테이블 5장 38행(태그 → 타입), 패시브 행, 세트 9개, BT 노드 3종과 BT 4개, 쓰지 않게 되는 변형 태그 정리, 검증기, 문서. 이관에 빌드 두 번.
- 대안 E(사용자 제안 "WxAbilitySystemComponent나 WxAbilityBase의 함수를 추가하거나 적절하게 override 하면 되지 않을까요?"): 함수는 어디서 찾는지를 바꿀 뿐, 클라이언트가 무엇을 아는지는 바꾸지 못한다. 클라이언트는 부여하지 않고 복제된 스펙을 받아 `OnGiveAbility`만 부른다(`GameplayAbilityTypes.cpp:277`, 스펙 복제는 `COND_ReplayOrOwner`, `AbilitySystemComponent.cpp:1861`). 그런데 `LocalPredicted`라 클라이언트가 먼저 행을 읽는다.
  - 서버 전용 핸들→행 맵: 클라이언트가 모른다.
  - 그 맵을 ASC에서 따로 복제: 스펙과 맵이 다른 프로퍼티라 클라이언트에서 스펙이 먼저 처리되는 순간(`OnGiveAbility`) 행이 비고, 부여·제거·분신 부여마다 맵을 함께 맞춰야 한다. 동기화 장치다.
  - 어빌리티 인스턴스 복제로 행을 싣기: 인스턴스도 스펙과 따로 도착해 같은 틈이 생기고, 엔진이 필수로 보지 않는 복제 설정을 켜야 한다(`GameplayAbility.h:689`).
  - 인스턴스 복제 설정(`ReplicationPolicy`)은 엔진 코드에 비권장 표시가 없고 RPC를 쓰는 어빌리티에는 요구된다(`GameplayAbility.cpp:314`). 비권장은 커뮤니티 문서(GASDocumentation 4.6.1.1 "Don't use this option")가 에픽 Dave Ratti의 제거 의사를 인용한 것이다. 프로젝트는 설정하지 않아 기본값(복제 안 함)이다.
  - 이어진 질문 "OnGiveAbility 에서 어빌리티 행을 기억해두면 클라와 서버 모두 문제 없지 않나요?": `OnGiveAbility`는 클라이언트에서도 스펙이 복제되어 올 때 인스턴스를 만든 뒤 불린다(`AbilitySystemComponent_Abilities.cpp`의 `OnGiveAbility`). 그래서 기억하기는 양쪽에서 된다. 다만 받는 것이 스펙뿐이라 어느 행인지는 스펙에 실린 표시(지금은 태그)로 알아내야 한다. 기억하기는 검색을 부여 때 한 번으로 줄이고 디버거에서 행이 보이게 할 뿐, 태그를 없애지는 못한다. 기억할 것은 행 포인터가 아니라 행 핸들이다(에디터에서 테이블을 고치면 포인터가 무효가 될 수 있다). 조회할 수 있는 값을 저장하는 캐시다.
  - 이어진 질문 "우리 프로젝트만의 커스텀 AbilitySpec이나 SpecHandle을 만들 수 없나요?": 엔진(GAS 플러그인)을 고치지 않고는 안 된다. ASC의 스펙 배열은 `TArray<FGameplayAbilitySpec>`로 타입이 고정된 멤버다(`GameplayAbilitySpec.h:312`, `AbilitySystemComponent.h:1668`). `GiveAbility`는 받은 스펙을 값으로 복사한다(`AbilitySystemComponent.h:949`). 그래서 상속해 더한 칸은 저장 때 잘리고 복제되지 않는다. 핸들은 전역 카운터에서 뽑는 정수다(`GameplayAbilitySpecHandle.cpp:9`). 스펙에 사용자 데이터를 붙이는 확장 칸도 5.8에는 없다. 남는 길은 GAS 수정(업그레이드마다 다시 맞춤)이나 ASC의 별도 복제 배열(동기화 장치)이다.
  - 결론: 행 정보는 스펙 자체에 실려야 틈이 없다(지금의 태그, 또는 A). 행 찾기는 이미 `UWxAbilityBase::GetAbilityRow()` 한 곳으로 모여 있어 방식을 바꿔도 그 함수와 부여만 바뀐다.
- 안 F, 슬롯 클래스(사용자 제안 "BT는 신경 안써도 될거 같아요. 왜냐면 Pattern_1 ~ 9 까지 어빌리티 클래스를 늘리면 되니까요."): 한 세트 안에서 같은 클래스가 두 번 나오지 않게 슬롯마다 클래스를 둔다. 그러면 스펙이 이미 싣는 클래스와 세트(`SourceObject`)만으로 행이 하나로 정해져 A도 필요 없다.
  - 테이블은 타입 칸 하나만 둔다(태그 칸 없음). 부여는 `FGameplayAbilitySpec(Row.Type, 1, INDEX_NONE, 세트)`이고 태그를 싣지 않는다. 행은 스펙의 세트에서 `Type`이 스펙 클래스와 같은 행이다. 규칙은 "한 세트에 같은 클래스는 한 번" 하나다. Lyra 세트와 같은 방식이다.
  - 슬롯 태그는 이미 네이티브로 있다: `Ability.Pattern.1`~`9`, `Ability.Skill.1`~`4`, `Ability.Attack.Light.1`·`2`, `Ability.Attack.Heavy.1`·`2`, `Ability.Ultimate.1`·`2`, `Ability.HitReact.KnockUp`·`Normal`. 새로 필요한 것은 넉 계열 피격 행 하나다(지금 부모 태그 `Ability.HitReact`를 행 태그로 쓴다).
  - BT 에셋이 부르는 태그 이름이 그대로 클래스 태그가 되어 BT는 바뀌지 않는다. 엔진 태그 기능(태그로 발동·취소·차단)과 UI 슬롯도 슬롯 단위로 고를 수 있게 된다.
  - 확인한 동작 변화: 분신의 제외 목록이 `Ability.Ultimate`를 정확히 일치로 비교한다(`WxBTTask_MirrorAbility.cpp:86`). 궁극기가 슬롯 클래스(`Ability.Ultimate.1`·`2`)가 되면 제외되지 않으므로 부모까지 보게 고친다. 엔진 차단·취소, UI 슬롯(`HasAll`), BT 발동·관찰(`HasTag`·`HasAny`)은 부모 태그로 맞고, 주인 바라보기의 `Ability.Skill.3` 정확 비교는 스킬 3 클래스와 그대로 맞는다.
  - 대가: 슬롯 클래스 약 22개(생성자에서 태그만 정함)와 계열 클래스의 추상화. 정한 수를 넘는 슬롯은 코드로 늘린다. 테이블 38행을 태그에서 타입으로 옮긴다. 패시브는 모두 같은 클래스라 이 방식이 맞지 않는다. 서버 전용(`WxAbility_Passive.cpp:16`)이라 클라이언트가 행을 알 필요는 없으므로 별도로 정한다.
  - 클래스 수(사용자 지적 "이 방안은 너무 C++ 클래스가 많아진다는 단점이 있네요"): 지금 데이터가 쓰는 슬롯만 세면 15개다(약공격 2, 강공격 2, 스킬 3, 궁극기 2, 패턴 3, 피격 3). 네이티브 태그가 있는 범위(스킬 4, 패턴 9)까지 만들면 22개다. 계열 클래스 6개는 추상 부모가 된다. 클래스마다 헤더 8줄, 생성자 4줄 정도이고 `WxAbility_Attack.h`처럼 계열 파일에 모은다. 15개 중 5개(약공격 2, 강공격 2, 스킬 2·3, 궁극기 2)는 HGTest의 같은 입력 변형(분신·도플갱어 상태별) 때문이고, 패턴 2·3은 BT 슬롯, 피격 3개는 몽타주 슬롯이 달라서다.
  - D와 비교: 새 항목 클래스, 세트 구조 변경, 복제 보완, BT 변경이 모두 없다. 스톡 디버그 화면에 `WxAbility_Pattern_2`와 `ABS_Soldier`가 보여 행이 바로 드러난다.
- 부수 발견: `BT_Minion`은 주인의 약공격을 관찰하고(`ObserveAbility_4`) 자기 `Ability.Attack.Light`를 발동하는데(`ActivateAbility_3`), 분신 세트에 약공격 행이 없다. 전환 전 `ABS_Minion`(HEAD)에도 없어, 원래부터 비어 있던 가지다. 위 점검 4번의 "BT가 부르는 행 태그는 모두 상대 세트에 있다"는 행 단위 태그에 한한 말이었다.
- 권고(A만 볼 때): 찾는 방식은 그대로 둔다. 디버깅 표시를 행 이름으로 바꾼다. 행을 태그로 알리는 로그 네 곳(`WxAbilityBase.cpp:357`·`:428`, `WxBTTask_MirrorAbility.cpp:127`·`:139`)에 세트와 행 이름을 찍는다. A만으로는 태그가 남아 얻는 것이 작았다.
- 권고(D를 본 뒤): 사용자 기준에 가장 맞는 것은 D다. 태그가 통째로 빠져 태그 규칙 셋과 이중 관리가 함께 사라지기 때문이다. 작업이 커서 설계를 문서로 확정한 뒤 단계로 나눠 진행하기를 권했다. D를 택하면 점검 1번(식별 태그 겹침)은 `FindTypeByTag`가 없어져 해당이 없어진다.
- 권고(F를 본 뒤): F가 D보다 낫다. 행 찾기, 타입 선택, BT 이름이 모두 클래스 하나로 모이고, 엔진의 태그 기능이 슬롯 단위로 살아난다. 설계 문서로 확정한 뒤 진행하기를 권했다.
- 종합 판단(사용자 질문 "어떻게 해야 GA 에셋 생성 없이, 모든 데이터를 안정적으로 테이블에서 관리할 수 있을까요? 가장 유지보수와 확장성이 좋은 방법으로 하고 싶어요."): 원칙을 세우고 D를 권했다. F 권고는 거둔다.
  - 원칙: 클래스는 동작이며, 새 동작이 필요할 때만 늘린다. 행은 콘텐츠이며, 코드 없이 늘린다. 특정 콘텐츠를 가리킬 때는 행 자체를 가리키고, 범주를 가리킬 때는 클래스 태그를 쓴다.
  - 지금 방식은 새 행 이름마다 네이티브 태그(코드)가 필요하고, 타입을 태그 계층에서 유추한다. F는 콘텐츠 수만큼 클래스가 늘어 콘텐츠와 코드가 묶인다(같은 입력 변형까지 클래스가 된다). D는 새 콘텐츠를 행 추가만으로 끝내고, 클래스는 동작 수만큼만 둔다.
  - 이어진 질문 "슬롯 변경을 위한 어빌리티 구현을 위한 칼럼을 추가하는게 나을까요? 대신에 어빌리티 C++ 클래스 종류를 확 압축하구요.": 권하지 않았다. GAS에서 클래스는 엔진이 태그 관계를 읽는 단위다. 에셋 태그는 생성자에서만 정하고 런타임에는 CDO로 조회한다(`GameplayAbility.h:516`~`521`). 차단·취소는 발동 때 클래스의 에셋 태그와 차단·취소 목록으로 건다(`GameplayAbility.cpp:999`). 태그로 발동·취소하는 판정도 스펙의 CDO 에셋 태그를 본다(`AbilitySystemComponent_Abilities.cpp:1340`, `:1549`). 그래서 계열을 칼럼으로 옮기면 엔진이 그 값을 읽지 못하고, 살리려면 엔진 판정을 대신하는 override와 태그 칼럼 여럿이 필요하다. 지금 타입 19개는 동작과 태그 관계 한 벌씩이라 알맞은 크기다. 예로 피격은 공격·스킬은 끊고 패턴은 끊지 않는다.
  - D에서 한 번 치르는 대가: 항목 클래스와 복제 보완, 세트 구조 이관과 편집 한 단계 추가, BT 노드 3종의 행 지정 칸과 11곳 이관, 행 이름 변경 검증, 엔진 디버그 화면의 항목 이름(로그로 보완).
- 상태: 사용자 판단 대기.

**안 D 확정(2026-09-25)**: 사용자 "네. 좋습니다. 합의된 방향으로 구현 진행합시다!". 정할 것 세 가지는 권고대로 간다: 패시브도 항목으로 통일, 항목 이름을 행 이름으로 자동으로 맞춤, 쓰지 않게 되는 행 태그 정리. 같은 행 중복은 캐릭터 단위로 한 번만 부여하고 세트 안 중복·빈 항목은 경고한다. HUD의 키 기반 선택은 필요할 때 한다. 스킬·궁극기 타입은 이미 클래스 하나라 따로 합칠 것이 없다("아 이건 통합이 되어있었군요").
- 구현 세부(설계에서 AI가 정함):
  - 세트의 새 배열 이름은 `AbilityEntries`·`PassiveEntries`다. 옛 배열(`Abilities`·`Passives`)과 이름이 달라야 1차 빌드에서 옛 데이터를 읽어 옮길 수 있다. 같은 이름으로 바꾸면 형식이 달라 옛 값이 로드에서 버려진다.
  - 항목 클래스는 액션용 `UWxAbilityEntry`와 패시브용 `UWxPassiveEntry` 둘이다. 행 선택기의 테이블 거르기(`RowType`)가 속성 메타데이터라 행 형식마다 클래스가 하나씩 필요하다.
  - 1차 빌드의 세트 `PostLoad`(에디터 전용)가 옛 행 핸들을 행 이름을 붙인 항목으로 옮긴다. 2차 빌드에서 옛 배열과 이 코드를 지운다.
  - 테이블 `Type`은 MCP로 채운다(행 태그 → 타입 대응). `Tag`는 1차에서 남기고 2차에서 지운다.
  - BT 행 지정 검증은 WxEditor의 BT 에셋 검증기로 한다. BT 노드의 모든 행 핸들 속성이 풀리는지 본다.
    - 변경(사용자 "Validator는 지금 추가하지 않아도 됩니다."): 1차에 넣은 BT 행 검증기(`UWxBehaviorTreeRowValidator`)와 WxEditor의 `AIModule` 의존성을 2차에서 지웠다. 세트·테이블의 기존 검증은 그대로다.
  - 사용자 질문 "WxAbilityEntry는 꼭 추가해야되는건가요?": 안 D의 핵심이라 필요하다고 답했다. 행마다 하나씩 있으면서 복제되는 기존 객체가 없다. 세트와 테이블은 여러 행이 함께 쓰고, 몽타주도 행끼리 겹친다(`Minion_Attack_Heavy`와 `HGTest_Attack_Heavy_1`이 같은 몽타주). 줄일 수 있는 것은 패시브 항목 클래스다. 행 선택기의 테이블 거르기를 포기하면 하나로 합칠 수 있다.
- 구현 결과(2026-09-25):
  - 1차 빌드: WxCore `IWxAbilityRowSource`, WxCombat `UWxAbilityEntry`·`UWxPassiveEntry`(복제 이름 안정성 덮어쓰기, 편집 시 행 이름으로 개명), 세트의 `AbilityEntries`·`PassiveEntries`와 부여·조회·검증, 캐릭터 단위 중복 부여 거름(`GiveAbilitySets`), 행 `Type`, `FindTypeByTag` 삭제, BT 노드 3종의 행 칸, 분신 태스크의 태그 복사 삭제. Development 빌드 통과.
  - 이관(MCP): 테이블 5장 38행에 `Type`을 채웠다(행 태그 → 타입, 전과 같은 결과). 세트 9개는 `PostLoad`가 행 이름을 붙인 항목으로 옮겼고(패시브 2행 포함), 저장했다. BT 4개 11노드에 행을 지정하고 행 단위 태그를 비웠다(`BT_Minion`의 관찰 노드는 모두 `Master`를 봐서 HGTest 행, 발동 노드는 Minion 행). 저장 시 검증 오류는 없다.
  - 2차 빌드: 행의 `Tag`, 세트의 옛 배열과 `PostLoad`, 번호 붙은 행 태그 21개(`Ability.Skill.1`~`4`, `Ability.Pattern.1`~`9` 등), BT 행 검증기를 지운다. 코드·설정·에셋 어디에서도 그 태그를 쓰지 않음을 확인했다. Development 빌드는 경고 없이 통과했다. 테이블 5장, `DT_Passive`, 세트 9개를 다시 저장해 옛 값이 파일에서 빠졌음을 확인했다.
  - PIE 단독(`LV_DevCombat`): 스펙의 `SourceObject`가 항목이고(`ABS_Soldier:Soldier_Pattern_1` 등) 스펙 태그는 비어 있다. 부여 목록은 전과 같다(솔저 8, 템플릿 플레이어 18+패시브). 플레이어를 솔저 앞에 두자 솔저 BT가 행 지정으로 패턴을 발동했다(패턴 활성, `State.Engaged`, 플레이어 HP 100→16). 부여 오류와 복제 경고는 없다.
  - PIE 네트워크(리슨 서버 + 클라이언트 1, 한 프로세스): 클라이언트 월드(`UEDPIE_1`)의 자기 캐릭터 스펙 19개 모두 `SourceObject`가 항목 경로로 풀렸다(패시브 포함). 서버 월드는 두 캐릭터 모두 19개다. "NOT Supported"·풀기 실패 경고는 없다. 플레이 설정은 원래 값(단독, 1명)으로 되돌렸다.
  - 미확인: 에디터를 켠 뒤 새로 넣은 항목의 원격 클라이언트 복제(`IsNameStableForNetworking` 덮어쓰기가 막는 경우). 확인하려면 세트를 고쳐야 해서 하지 않았다.
  - 일괄 검증(커맨드릿): 테이블 14장, 세트 9개, 몽타주 44개 모두 유효하다(`DT_Dialogue`만 기존 오류). DebugGame 빌드도 통과했다.
- 방향 변경(2026-09-25, 구현 뒤 사용자 지시):
  - "BT 노드는 이전처럼 어빌리티 태그로 발동하게 합시다." 태그를 어디에 둘지 물었고, 사용자가 "일종의 DynamicOwnedTag를 테이블에서 설정할 수 있게 한다"를 골랐다. 그래서 행에 `DynamicTags`(태그 컨테이너) 칸을 두고, 부여할 때 스펙의 `DynamicSpecSourceTags`에 싣는다. BT 노드 3종과 분신 태스크는 행 칸을 지우고 예전 태그 매칭(타입 태그 또는 스펙의 동적 태그)으로 되돌렸다. 분신 태스크는 동적 태그도 다시 옮긴다. 행 출처 인터페이스(`IWxAbilityRowSource`)는 쓰는 곳이 없어 지웠다. 행 찾기는 계속 항목(`SourceObject`)으로 한다.
    - 복원한 네이티브 태그: `Ability.Skill.1`~`4`, `Ability.Pattern.1`~`9`. 약공격·강공격·궁극기·피격 번호 태그는 쓰는 곳이 없어 복원하지 않았다.
    - 활성 중 캐릭터 소유 태그로는 걸지 않는다(스펙 태그만). BT가 보는 곳이 스펙이다.
    - 동적 태그를 채우는 행: `HGTest_Skill_1`~`3`, `Minion_Skill_1`·`2`, `Soldier_Pattern_1`~`3`, `Template_Pattern_1`·`2`(BT·분신이 부르는 행). 규칙은 없다(비워도 된다).
  - "MontageValidator도 지금은 불필요합니다. 제거해주세요." `UWxAbilityMontageValidator`와 이를 위해 넣은 WxEditor 의존성(`AssetRegistry`·`DataValidation`·`WxCombat`)을 지웠다. WxEditor는 HEAD와 같아졌다. 타입별 몽타주 규칙은 테이블을 저장할 때 행 검증으로 계속 돈다.
  - 결과: Development 빌드 통과(경고 없음). 테이블 10행에 동적 태그를 채우고 BT 4개 11노드의 태그를 원래 값으로 되돌려 저장했다(저장 시 검증 통과). `BT_Minion`·`BT_Template`은 HEAD와 바이트까지 같다. `BT_Soldier`·`BT_Doppelganger`는 내용은 같고 다시 저장한 바이너리만 조금 다르다(제출 때 HEAD로 되돌려도 된다). PIE에서 솔저 스펙에 동적 태그가 실렸고(`Soldier_Pattern_2` → `Ability.Pattern.2`), 솔저 BT가 태그로 패턴을 발동해 플레이어 HP가 0이 됐다.

**세트 = 테이블 목록(2026-09-25)**: 사용자 "WxAbilitySet에서 AbilityEntries 말고, 어빌리티 DT 에셋 하나를 입력하면, 그 테이블 안에 있는 모든 어빌리티를 쓰도록 합시다. 이게 더 편하고 복제에서도 안정적일 것 같아요." 선택지에서 "세트 = 테이블 목록"을 골랐다.
- 구조: 세트는 `AbilityTables`·`PassiveTables`(행 형식으로 거른 테이블 목록)를 갖고 테이블의 모든 행을 부여한다. 스펙의 `SourceObject`는 테이블이고, 행의 `DynamicTags`를 `DynamicSpecSourceTags`로 싣는다. 스펙은 테이블 안에서 타입과 동적 태그가 모두 같은 행을 자기 행으로 찾는다(`UWxAbilitySet::FindAbilityRow`). 패시브는 클래스가 하나라 동적 태그로만 찾는다(패시브 행에 `DynamicTags` 칸 추가).
- 규칙: 한 테이블 안에서 타입이 같은 행은 동적 태그가 달라야 한다. 세트 검증이 오류로 알리고, 부여는 뒤 행을 거부하고 로그를 남긴다. 테이블 단위 규칙이라 서로 다른 테이블끼리는 겹쳐도 된다. 캐릭터 단위 같은 행 한 번 부여(`GrantedRows`)는 그대로다.
- 복제: 테이블은 디스크에서 로드된 에셋이라 이름 안정성 덮어쓰기가 필요 없다. 항목 클래스(`UWxAbilityEntry`·`UWxPassiveEntry`)를 지웠다.
- 테이블 재구성: 세트가 테이블을 통째로 받으므로, 세트마다 골라 쓰던 행을 테이블로 나눴다. 복제 후 행 삭제로 옮겨 행 값은 그대로다.
  - `DT_Ability_Shared`는 공용 4행(피격 3, 사망)만 남기고, `DT_Ability_SharedPlayer`(플레이어 8행)와 `DT_Ability_SharedEnemy`(그로기)를 새로 만들었다.
  - `DT_Ability_HGTest`에 있던 분신 3행을 `DT_Ability_Minion`으로 옮겼다.
  - `DT_Passive`는 `DT_Passive_HGTest`·`DT_Passive_TemplatePlayer`로 나누고 지웠다.
- 세트 연결:
  - `ABS_Shared_Player` → Shared, SharedPlayer
  - `ABS_Shared_Enemy`, `ABS_Sandbag` → Shared, SharedEnemy
  - `ABS_HGTest` → HGTest, 패시브 HGTest
  - `ABS_Minion` → Minion
  - `ABS_Template`(플레이어) → TemplatePlayer, 패시브 TemplatePlayer
  - `ABS_Template`(적) → TemplateEnemy
  - `ABS_Soldier` → Soldier
  - `ABS_Doppelganger` → 없음
- 부여 목록은 전과 같다. 순서는 테이블 순서라 일부 바뀌었다. 같은 입력을 나눠 쓰는 행끼리의 순서는 같고, `Event.Hit`로 함께 도는 피격과 가드 반응도 피격이 먼저인 채다. 사망·그로기는 각자 이벤트라 순서와 무관하다.
- 동적 태그를 새로 채운 행(같은 테이블 안 같은 타입):
  - HGTest 약공격 1·2(`Ability.Attack.Light.1`·`2`), 강공격 1·2(`Ability.Attack.Heavy.1`·`2`), 궁극기 1·2(`Ability.Ultimate.1`·`2`)
  - 공용 피격 넉업·일반(`Ability.HitReact.KnockUp`·`Normal`). 기본 피격 행은 비워 둔다(빈 태그도 구분된다).
  - 이 8개 네이티브 태그를 다시 추가했다.
- 결과:
  - Development·DebugGame 빌드 경고 없음. 테이블과 세트 저장 시 검증 통과.
  - 옛 세트 파일의 항목 객체는 로드할 때 클래스 없음 경고를 냈고, 다시 저장하면서 빠졌다.
  - 네트워크 PIE(`LV_DevCombat`, 리슨 서버 + 클라이언트 1): 서버 플레이어와 클라이언트 자기 캐릭터 모두 19스펙이다. `SourceObject`가 테이블로 풀리고 동적 태그도 복제됐다.
  - 솔저는 8스펙이다(패턴 3에 `Ability.Pattern.1`~`3`). 솔저 BT가 태그로 패턴을 발동해 플레이어 HP가 100→30이 됐다. `LogWxCombat` 오류는 없다. 플레이 설정은 원래 값으로 되돌렸다.
  - 미확인: HGTest·분신·도플갱어는 이 맵에 없어 세트 검증으로만 확인했다.

**안 D 상세(초안, 2026-09-25, 사용자 요청 "안 D에 대해 구체적으로 설명해주세요")**
- 데이터: 액션 행은 `Tag`를 지우고 첫 칸에 `Type`(추상·`UWxAbility_PlayMontageOnce` 제외)을 둔다. 패시브 행은 `Tag`를 지운다. 세트의 `Abilities`·`Passives`는 행 핸들 하나를 담은 인스턴스 항목 객체 배열이 된다(행 칸에 `RowType`·`WxPreviewRow` 유지). 행 이름이 콘텐츠의 이름이다.
- 실행: 부여는 `FGameplayAbilitySpec(Row.Type, 1, INDEX_NONE, 항목)`이고 태그를 싣지 않는다. 행 찾기는 스펙 → `SourceObject`(항목) → 행 핸들이다. 분신은 클래스와 `SourceObject`를 복사한다. `GetAbilityRow()`를 부르는 곳(입력, UI, 비용, 쿨다운, 몽타주)은 그대로다.
- 모듈 경계: WxAI는 WxCore에만 의존한다(`WxAI.Build.cs`). BT 노드가 스펙의 행을 비교하려면 WxCore에 행 출처 인터페이스를 두고 항목이 구현한다.
- BT: 범주는 지금처럼 클래스 태그로 고른다. 특정 콘텐츠는 행 지정 칸(`ActivateAbility`·`ObserveAbility`·`MirrorMovement`)으로 고른다. 이관할 곳은 11노드다. `MirrorAbility`의 제외 목록은 클래스 태그로 남는다.
- 규칙: 세트 안의 빈 항목, 같은 행 두 번, 풀리지 않는 행은 오류다. BT의 풀리지 않는 행 지정도 오류다. 태그 계열, 태그 유일, 태그 등록 규칙은 사라진다.
  - 수정(사용자 지적 "이게 된다면 세트에 같은 행 두번 금지 규칙 없어도 되는거 아닌가요?"): 맞다. 각 스펙이 자기 항목으로 행을 찾으므로 식별에는 필요 없다. 남는 문제는 같은 어빌리티가 두 번 부여되는 것이다. 이벤트로 도는 타입(피격·가드 반응·사망·패시브)은 한 이벤트에 두 번 반응하고, 패시브는 효과가 두 번 걸린다. 입력으로 도는 타입은 대개 첫 번째만 발동하지만, 첫 번째가 실행 중이면 배타 그룹이 막지 않는 한 두 번째가 따로 발동한다(`WxAbilitySystemComponent.cpp:136` 순회는 성공할 때까지 다음 스펙을 시도).
  - 그래서 외울 규칙에서 빼고 시스템 동작으로 바꾼다. 캐릭터가 받는 세트 전체에서 같은 행은 한 번만 부여하고(`GiveAbilitySets`에서 행 핸들로 거름, 로그), 검증기는 세트 안 중복을 경고로 알린다. 행 핸들은 세트를 넘어 비교할 수 있어서, 점검 3번(여러 세트 사이 중복)도 함께 풀린다. 빈 항목도 부여가 건너뛰고 검증기가 경고한다.
- 복제: 항목 클래스가 `IsNameStableForNetworking`을 덮어써 에디터에서 새로 넣은 항목도 원격 클라이언트에 닿게 한다. 네트워크 PIE(리슨 서버 + 클라이언트)로 확인한다.
- 이관: 1차 빌드에서 새 칸·배열·BT 칸을 옛것과 함께 둔다. MCP로 옮긴다(타입은 지금의 태그→타입 결과, 항목은 옛 핸들, BT는 태그→행 대응). 2차 빌드에서 옛 칸·경로·`FindTypeByTag`·쓰지 않는 행 태그를 지운다. 이어 검증(커맨드릿), PIE(단독·네트워크), DebugGame 빌드를 한다.
- HUD(사용자 질문 "HUD는 자동으로 Requirements가 충족되는 어빌리티를 출력하면 되지 않을까요?"): 같은 키의 변형은 이미 그렇게 고른다. 슬롯 클래스 태그에 맞는 스펙 중 조건을 충족하는 첫 어빌리티를 보이고, 모두 막히면 보던 것을 유지한다(`WxViewModel_Ability.cpp:184`). 조건으로 가를 수 없는 경우는 다른 키에 같은 클래스 두 개가 동시에 쓸 수 있을 때다. 그때 가르는 것은 키다. 확장안은 슬롯을 클래스 태그 대신 키(`InputAction`)로 고르게 하는 것이다. "지금 이 키를 누르면 나갈 어빌리티"는 입력 처리와 같은 규칙이다. WxUI는 WxCore에만 의존하므로 어빌리티의 입력은 WxCore UI 인터페이스(`IWxUIData`)로 받는다. 지금은 해당 캐릭터가 없어 필요할 때 한다.
- 정할 것: 패시브도 항목으로 통일할지, 항목 이름을 행 이름으로 자동으로 맞출지(엔진 디버그 화면용), 쓰지 않게 되는 행 태그를 지울지.

### 외부 사례 조사(2026-09-25)

사용자 요청: "다른 GAS 기반 언리얼 게임 프로젝트에서도 저처럼 테이블로 구동되는 어빌리티를 설계한 사례를 찾아서 참고 분석해주세요." 전체 보고서는 대화로 전달했고 저장소에는 두지 않았다.

- 결론: 공개 자료에서 같은 조합(타입 클래스 하나를 여러 스펙이 공유, 행 하나가 스펙 하나, `SourceObject` = 테이블과 동적 태그로 행 찾기)은 찾지 못했다. 구조가 확인된 사례는 모두 어빌리티 하나에 GA 클래스·에셋 하나다. 데이터로 빼는 범위는 부여 목록, 입력 매핑, 태그 관계, 수치(CurveTable, SetByCaller), UI 정보까지다.
  - Lyra: 세트 필드는 클래스·레벨·InputTag뿐이다. 취소·차단은 `ULyraAbilityTagRelationshipMapping`, 콘텐츠는 `GA_Weapon_Fire_*` BP 자식에 둔다. `SourceObject`는 장비 인스턴스다.
  - 구 Action RPG: `SourceObject`에 정적 데이터 에셋(`URPGItem`)을 싣는다. Epic 선례 중 지금 구조와 가장 가깝다. 공식 문서는 액션 게임에는 어빌리티 BP가 잘 맞는다고 적는다.
  - Slitterhead(UE5, UNREAL FEST 2024 TOKYO): GA 380개, 1액션 1에셋, 베이스 → 공격 종류 → 캐릭터별 3단 상속이다. 이점으로 병렬 작업과 파일 충돌 감소를 들었다.
  - Ninja Combat: 범용 C++ 어빌리티의 BP 서브클래스 기본값에 몽타주·GE·태그를 둔다.
  - Aura: `UAbilityInfo` DataAsset은 태그·UI 정보 카탈로그이고 어빌리티는 클래스별이다.
  - AbilityEditorHelper(중국, 검색 요약만 확인): Excel에서 GA/GE 에셋을 생성한다. 기각한 "테이블에서 GA_ 자동 생성"의 공개 구현이다.
- 구성 요소별 선례는 있다. `SourceObject`로 콘텐츠 가리키기(Lyra, GASShooter, Action RPG), 스펙 동적 태그(Lyra InputTag), 인스턴스를 읽는 비용·쿨다운 MMC(GASDocumentation, Instanced 전제), `DoesAbilitySatisfyTagRequirements` 오버라이드(Lyra)다. Epic Dave Ratti가 스펙이 서브클래싱 가능한 UObject였어야 했다고 말한 기록이 있어, 스펙 확장 불가 판단과 맞는다.
- 코드 대조(보고서가 짚은 엔진 경로):
  - `TryActivateAbilitiesByTag`는 CDO 에셋 태그와 CDO 조건으로 스펙을 고른 뒤 고른 스펙을 모두 발동한다(`AbilitySystemComponent_Abilities.cpp:1562`). 프로젝트 호출부는 `WxInventoryComponent.cpp:494`(UseItem) 하나다. CDO에는 행이 없어 `UWxAbilityBase::DoesAbilitySatisfyTagRequirements`가 행 조건을 건너뛰고, 행 조건은 인스턴스 판정에서 걸린다. 지금은 맞게 동작한다. UseItem 행이 둘이 되면 "처음 성공" 규칙과 달리 둘 다 시도한다.
  - 동적 태그는 어빌리티가 만든 GE 스펙의 `CapturedSourceTags`로 복사된다(`GameplayAbility.cpp:1381`). 피해 GE는 ASC의 `MakeOutgoingSpec`으로 만들어(`WxDamageTableRow.cpp:35`) 해당이 없다. 복사되는 곳은 비용·쿨다운·그로기 `DrainGP`·질주 GE이고, C++에서 소스 태그로 `Ability` 태그를 보는 곳은 없다.
  - 패시브 클래스는 클래스 `AbilityTriggers`가 비어 있어 스펙의 `DynamicAbilityTriggers`가 쓰인다. `HasActivatableTriggeredAbility`는 쓰지 않는다.
  - HUD는 기본 인스턴스로 조건을 본다(`WxViewModel_Ability.cpp:183`).
  - `showdebug abilitysystem`의 쿨다운 남은 시간은 CDO로 계산한다. 기존 "디버그 표시" 대가에 포함된다.
- 판단: 구조를 바꿀 근거는 나오지 않았다. 선례가 에셋 단위를 고른 이유(병렬 작업, 충돌 감소)는 이미 적은 대가(편집 충돌 단위)와 같다.

### GA_ 복귀 설계(2026-09-25)

사용자 결정: "네, 좋습니다. GA_ 에셋으로 돌아갑시다." 기준(판단 근거는 미결의 Q-GA_ 복귀): AI가 구조를 파악하고 작업하기 편한 쪽. 기준 코드는 지금 작업 트리(미커밋, 세트 = 테이블 목록)다. 상태: 사용자 확정(2026-09-25, 아래 수정안에 "네"), 구현·자체 검증 끝(아래 "GA_ 복귀 결과").

초안 수정(사용자 "HideCategories로 숨기지는 마세요. 그리고 동적 태그는 테이블에서나 필요한 거였지, GA 에셋으로 하게 된다면 굳이 동적 태그 필요 없을거 같아요. BP 그래프에 로직을 두지는 않을 것입니다."):
- 엔진 칸을 숨기지 않는다. 규칙 잠금은 그래프 로직 금지와 함께 관례가 된다.
- 동적 태그를 없앤다. BT가 부르는 GA_는 에셋 태그에 번호 태그를 더한다(HEAD 방식).
- 엔진 칸이 보이므로 같은 뜻의 Wx 칸은 엔진 칸으로 바꾼다(발동 조건, 쿨다운 그룹, 패시브 트리거). 타입 효과와 콘텐츠 효과도 한 칸으로 합친다.

**원칙**
- 어빌리티 하나에 데이터 전용 GA_ 하나를 둔다. 부모는 구체 타입(C++)이다. 타입은 생성자에서 규칙 기본값을 정하고, GA_는 콘텐츠를 채운다. GA_는 규칙 칸과 그래프를 건드리지 않는다(관례).
- 테이블 전환에서 얻은 것은 유지한다: 공격 4분할과 타입 규칙, 몽타주 섹션 규칙, 노티파이(컷신·처형·아이템), 쿨다운 그룹 GE, 검증 규칙, 락온 프리셋의 설정 이동, 인스턴스로 판정하는 HUD.
- HEAD의 GA_를 되살리지 않는다. 옛 프로퍼티(`ComboMontage`, `AbilityDataRow` 등)로 저장돼 있어 값이 로드에서 버려진다. 값은 지금 테이블 행에서 옮긴다.

**칸 배치**

| 값 | 액션(`UWxAbilityBase`) | 패시브(`UWxAbility_Passive`) |
|---|---|---|
| 타입 | GA_의 부모 클래스 | GA_의 부모 클래스 |
| BT가 부르는 이름 | 엔진 `AssetTags`에 번호 태그 추가(예: `Ability.Pattern.2`) | — |
| 발동 조건(`Master.*`) | 엔진 `ActivationRequiredTags`·`ActivationBlockedTags` | 같다 |
| 쿨다운 그룹 | 엔진 `CooldownGameplayEffectClass`(`UWxEffect_Cooldown_*`) | — |
| 트리거 | 타입 생성자(피격 등) | 엔진 `AbilityTriggers` |
| 활성 중 효과 | `ActivationOwnedEffects` 하나(타입 기본값 + GA_ 추가, HEAD 방식) | — |
| 적용 효과 | — | `Effects` |
| 입력, 몽타주, 쿨다운 시간·충전 수, 비용 자원·양, 표시 | Wx 프로퍼티(`EditDefaultsOnly`) | — |

- `ActivationGroup`과 타입 튜닝 값(락온·질주 등)도 HEAD처럼 `EditDefaultsOnly`로 되돌린다. 값은 지금 코드 기본값 그대로다.
- 비용 GE는 지금처럼 타입 생성자가 `CostGameplayEffectClass`에 `UWxEffect_Cost`를 넣는다.

**코드**

| 곳 | 변경 |
|---|---|
| `UWxAbilityBase` | `NotBlueprintable` → `Blueprintable`. 위 Wx 프로퍼티를 둔다. `GetAbilityRow()`를 지우고 자기 프로퍼티를 읽는다. `DoesAbilitySatisfyTagRequirements`에서 행 조건 줄을 지운다(무시 태그·콤보 창 우회는 유지). `GetCooldownGameplayEffect`는 쿨다운 시간이 0이면 nullptr, 아니면 엔진 기본. 행 검증은 `IsDataValid`로 옮긴다(엔진이 BP 저장 때 CDO의 `IsDataValid`를 부른다, `Blueprint.cpp:2233`). 쿨다운 시간이 있으면 쿨다운 GE가 `UWxEffect_Cooldown` 파생이어야 한다 |
| 타입 클래스 | 구체 타입은 `Blueprintable`을 물려받는다. 추상 `UWxAbility_Attack`과 시스템이 부여하는 `UWxAbility_PlayMontageOnce`는 `NotBlueprintable`로 둔다(공격 구체 넷은 `Blueprintable` 명시) |
| `UWxAbility_Passive` | `Blueprintable`. `Effects` 프로퍼티. `DoesAbilitySatisfyTagRequirements` 오버라이드를 지운다. 트리거 조상 관계 검사를 `IsDataValid`로 옮기고, 트리거 원천이 GameplayEvent가 아니면 오류로 한다 |
| `UWxAbilitySet` | `AbilityTables`·`PassiveTables` → `GrantedAbilities`(`TSubclassOf<UGameplayAbility>` 목록, HEAD 이름). 부여는 `FGameplayAbilitySpec(클래스, 1, INDEX_NONE, 세트)`다. 캐릭터에 같은 클래스가 이미 있으면 경고하고 건너뛴다(`FindAbilitySpecFromClass`, 클래스당 스펙이 하나라 다시 쓸 수 있다). `FindAbilityRow`·`FindPassiveRow`·`GrantedRows`를 지운다. 입력 목록과 세트 검증(입력 겹침, 쿨다운 그룹 값, 피격 섹션, 속성 행, 빈 칸)은 CDO로 본다 |
| ASC 입력 라우팅, 입력 버퍼, 비용 MMC | 행 대신 어빌리티 프로퍼티를 읽는다 |
| BT 노드 3종(`ActivateAbility`, `ObserveAbility`, `MirrorMovement`), `MirrorAbility` | 스펙 동적 태그 매칭을 지우고 에셋 태그만 본다. `MirrorAbility`는 클래스·레벨·`SourceObject`만 복사한다 |
| 태그 | 테이블 안 구분에만 쓰던 8개(`Ability.Attack.Light.1`·`2`, `Heavy.1`·`2`, `Ultimate.1`·`2`, `HitReact.KnockUp`·`Normal`)를 지운다. BT가 쓰는 `Ability.Skill.1`~`4`, `Ability.Pattern.1`~`9`는 유지한다 |
| 삭제 | `FWxAbilityTableRow`, `FWxPassiveTableRow` |

**데이터**
- GA_ 40개(액션 38, 패시브 2)를 HEAD와 같은 경로·이름으로 만든다. `GA_Minion_Death`는 만들지 않는다(`GA_Shared_Death`로 합친 상태 유지). git에서는 삭제가 아니라 수정으로 보인다.
- 행 → GA_ 대응: 타입 → 부모, 동적 태그 → 에셋 태그(BT용 10개만, 구분용 8개는 버림), 발동 조건 `RequireTags`·`IgnoreTags` → `ActivationRequiredTags`·`ActivationBlockedTags`(`TagQuery`는 쓰는 행이 없음을 확인한 뒤 버림), 쿨다운 그룹 → `CooldownGameplayEffectClass`, 활성 중 효과 → `ActivationOwnedEffects`에 추가, 나머지는 같은 이름. 패시브: 트리거 이벤트 → `AbilityTriggers`(GameplayEvent), 효과 → `Effects`.
- 세트 9개는 지금 부여 순서(테이블 순서 → 행 순서) 그대로 GA_ 목록으로 바꾼다. `ABS_Doppelganger`는 비워 둔다.
- 테이블 10장을 지운다.
- 이관 순서: 1차 빌드(새 프로퍼티, 옛 행 구조체, 임시 이관 함수) → 에디터에서 GA_ 생성·값 복사·세트 갱신, 행과 대조 → 2차 빌드(행 구조체·이관 함수·태그 삭제) → 테이블 삭제, 재저장.

**동작 변화**
- 없는 것이 목표다. 엔진의 CDO 경로(`TryActivateAbilitiesByTag` 후보 거르기, `HasActivatableTriggeredAbility`, showdebug 쿨다운)는 GA_ 값을 보게 되어 오히려 맞아진다.
- 엔진 로그와 디버거에 GA_ 이름이 나온다.

**검증**
- 빌드: Development·DebugGame(1·2차).
- 이관 직후 GA_ 40개의 값을 행과 대조한다(테이블 삭제 전).
- 데이터 검증 커맨드릿: GA_, 세트, 몽타주에서 오류·경고가 없어야 한다(`DT_Dialogue` 제외).
- 코드·설정·Content에 테이블, 행 구조체, 지운 태그 이름이 남지 않는다.
- PIE 단독·네트워크(`LV_DevCombat`): 부여 목록이 전과 같다(클래스가 GA_). 솔저 BT 패턴 발동, 패시브 트리거, HUD 아이콘·쿨다운·비용, 클라이언트 스펙 복제를 본다.
- 사람 확인: HGTest·분신·도플갱어(맵에 없음), 조작감.

**약점**: 에셋 수가 늘어난다. 규칙 칸 잠금과 그래프 로직 금지는 관례다. 이관에 빌드가 두 번 든다.

### GA_ 복귀 결과(2026-09-25)

- 코드: 설계 표대로 바꿨다. 설계와 달라진 것은 아래다. 모두 HEAD 형태로 되돌려 순정·코드 양을 줄이는 쪽이다.
  - 패시브는 HEAD 그대로다: 부모 `UWxAbilityBase`, `Abstract`, 효과 칸 `TriggeredEffects`. 트리거 검사(GameplayEvent, 조상 관계)만 `IsDataValid`로 더했다.
  - 타입은 HEAD처럼 `Abstract`로 되돌렸다(Dodge, Finisher, Guard, GuardReact, HitReact, Pattern, Skill, Interact, UseItem, 공격 구체 넷). 부여는 GA_로만 한다. `UWxAbility_Attack`·`PlayMontageOnce`에 `NotBlueprintable`은 두지 않았다(HEAD에도 없다).
  - 스펙에 `SourceObject`를 싣지 않는다(HEAD). 분신 따라하기도 HEAD 코드다.
  - HEAD와 같아진 파일: WxAI 전부(BT 노드 3종, 분신 따라하기), `WxGameplayTags`(구분용 태그 8개 삭제), ASC 입력 라우팅, 입력 버퍼, 쿨다운 MMC, `WxAbility_Interact.h`.
  - 비용 MMC는 CDO(`GetAbility()`)에서 읽는다(HEAD). HUD VM과 상호작용 스캐너의 인스턴스 판정은 유지하고 주석만 고쳤다.
  - 몽타주 칸 이름은 `AbilityMontage`다. `Montage`로 두면 몽타주 인자를 받는 함수들의 매개변수가 멤버를 가린다.
  - 행 구조체 `FWxAbilityTableRow`(HEAD에도 있던 `DT_Ability` 행)와 `FWxPassiveTableRow`, 쿨다운 그룹 부여 로그(`OnGiveAbility`)를 지웠다. 쿨다운 GE 검사는 `IsDataValid`가 한다.
- 이관: 빌드 두 번 대신 한 번으로 했다. 코드를 바꾸기 전에 MCP로 테이블 10장 40행과 세트 9개를 스냅숏으로 떠 두고, 새 빌드에서 스냅숏으로 GA_를 만들었다.
  - MCP `BlueprintTools.create`는 일반 `Blueprint`를 만든다. HEAD의 GA_는 순정 `GameplayAbilityBlueprint`라서, 헤드리스 Python(`GameplayAbilitiesBlueprintFactory`)으로 같은 경로에 다시 만들고 값을 옮겼다.
  - GA_ 40개는 HEAD 경로·이름 그대로다. git에서 수정으로 보인다. `GA_Minion_Death`는 삭제 상태다.
  - BT용 번호 태그 10개(`Ability.Skill.1`~`3`, `Ability.Pattern.1`~`3`)는 GA_의 에셋 태그와 소유 태그에 더했다. 구분용 8개는 버렸다. 발동 조건은 엔진 `ActivationRequiredTags`·`ActivationBlockedTags`, 쿨다운 그룹은 `CooldownGameplayEffectClass`, 행의 활성 중 효과는 타입 기본값 뒤에 붙였다.
  - 설명 문구의 현지화 키는 새로 잡혔다. 현지화 데이터(`Content/Localization`)가 없어 영향이 없다.
  - 테이블 10장을 지웠다. `BT_Soldier`·`BT_Doppelganger`는 HEAD로 되돌렸다(내용 같음, 재저장 바이너리만 달랐다). `ABS_Doppelganger`는 HEAD와 같아졌다.
  - git 인덱스: GA_ 40개의 삭제 스테이징을 풀었고, 인덱스에 올라 있던 지운 테이블을 뺐다. `DT_Ability`·`GA_Minion_Death`·`AM_Shared_Dodge`의 스테이징은 그대로다.
- 작업 중 만난 것:
  - `.git/index.lock`이 04:18부터 남아 있었고 git 프로세스가 없어 지웠다.
  - 삭제가 스테이징된 경로에 에셋을 만들면 에디터 git 연동이 모달을 띄워 MCP가 멈춘다. 그 세션만 `-SCCProvider=None`으로 띄웠다(설정 파일은 바꾸지 않음).
  - 블루프린트 컴파일도 CDO의 `IsDataValid`를 부른다. 몽타주가 필요한 타입으로 GA_를 새로 만들면 몽타주를 넣기 전까지 컴파일 오류가 보인다.
- 검증:
  - 빌드: Development·DebugGame 경고 없음.
  - 값 대조: MCP로 만든 직후 40개를 스냅숏과 대조했고(불일치 0), 순정 형식으로 다시 만든 뒤 새 프로세스에서 다시 대조했다(불일치 0, 클래스 `GameplayAbilityBlueprint`, 부모 일치, 세트 9개 일치).
  - 데이터 검증 커맨드릿: GA_ 40개, 세트 9개 모두 오류·경고 없음.
  - 코드·설정에 행 구조체, 테이블, `DynamicSpecSourceTags`, 지운 태그 이름이 남지 않았다.
  - PIE 단독(`LV_DevCombat`): 플레이어 19, 솔저 8, 템플릿 적 7, 샌드백 5개가 GA_ 클래스로 부여됐다(전과 같은 수). 솔저 BT가 에셋 태그로 패턴 1·2를 발동했고 소유 태그에 `Ability.Pattern.N`이 실렸다. 플레이어 HP 100→0. `LogWxCombat` 오류 없음.
  - PIE 네트워크(리슨 서버 + 클라이언트 1): 클라이언트 자기 캐릭터에 스펙 19개가 GA_ 클래스로 복제됐다. `NOT Supported` 없음. 플레이 설정은 원래 값(단독, 1명)으로 되돌렸다.
  - 미확인: HGTest·분신·도플갱어(맵에 없음), 조작감. 사용자 플레이 확인 대상이다.
- 남은 일: 인게임 플레이 확인, 기획 브리핑 수정 제안. 커밋·푸시와 Wiki 반영은 끝났다(2026-09-25).

### DT_Effect 제거(2026-09-25)

사용자 질문 "마찬가지로 DT_Effect도 제거하는게 맞을까요?"에 제거를 권했고, 사용자가 "네, 제거합시다"로 정했다. 기준: 행이 에셋과 1:1이면 값은 그 에셋에 둔다. `DT_Damage`는 여러 노티파이·투사체가 행을 골라 쓰고 짝이 되는 에셋이 없어 유지한다.

- 전: 행 2개(`GE_Shared_GuardReduction` 경감률 0.5·아이콘 `T_UI_Shield`, `GE_Template_Passive_AddUP` UP 5). GE의 `UWxEffectComponent_Table`이 행 핸들을 들고, `UWxMMC_EffectMagnitude`가 모디파이어 값을 행에서 읽었다. `UWxMMC_EffectDuration`은 쓰는 에셋이 없었다. 같은 역할의 `GE_HGTest_AddUP`은 이미 모디파이어 값을 직접 썼다.
- 코드:
  - `UWxEffectComponent_Table` → `UWxEffectComponent_UIData`(다른 GE 컴포넌트와 같은 이름 규칙). 행 대신 `Title`·`Description`·`Icon` 프로퍼티를 가진다. 엔진 `UGameplayEffectUIData` 파생이라 WxUI의 조회 앵커는 그대로다.
  - `FWxEffectTableRow`, MMC 둘을 지웠다.
  - `UWxEffect_GuardReduction`의 모디파이어는 `FScalableFloat`이고 값은 GE_ 에셋이 채운다.
- 에셋(헤드리스 Python):
  - `GE_Shared_GuardReduction`: 모디파이어 0.5, UI 컴포넌트 아이콘 `T_UI_Shield`.
  - `GE_Template_Passive_AddUP`: 모디파이어 5, 표시가 없어 UI 컴포넌트를 뺐다.
  - `DT_Effect`를 지웠다(참조 없음 확인). 클래스 이름 변경은 임시 `ClassRedirects`로 옮기고 재저장한 뒤 리디렉트를 뺐다(`DefaultEngine.ini`는 HEAD와 같다).
  - `GE_Shared_GuardReduction` 파일에는 컴포넌트 인스턴스 이름 `WxEffectComponent_Table_0`이 남는다. 클래스는 새 이름이다.
- 검증: Development·DebugGame 빌드 경고 없음. 리디렉트 없이 새 프로세스로 두 GE를 로드해 값·컴포넌트 클래스가 맞고 경고가 없음을 확인했다. 코드·설정·Content·Wiki·Docs에 옛 이름이 남지 않았다(위 인스턴스 이름 제외).
- 미확인: 인게임 가드 경감률과 버프 아이콘, 템플릿 패시브 UP 지급. 사용자 플레이 확인 대상이다.

### 락온 프리셋 복귀와 몽타주 검증 제거(2026-09-25)

- 락온 타게팅 프리셋(사용자 "락온 어빌리티의 타게팅 프리셋도 프로젝트 세팅이 아니라 다시 어빌리티의 데이터로 복원시키는게 낫겠네요"): 쓰는 곳이 락온 어빌리티 하나뿐이고, 설정으로 옮긴 이유는 GA_가 없어서였다. `WxAbility_LockOn.h`·`.cpp`, `WxCombatDeveloperSettings.h`, `DefaultGame.ini`를 HEAD로 되돌렸다. `GA_Shared_LockOn`의 `TargetingPreset`에 `TP_LockOn`을 넣었다(헤드리스 Python, 새 프로세스로 확인).
- 몽타주 검증(사용자 "ValidateMontage도 지금은 제거해주세요. 나중에 필요하면 재검토하겠습니다."): 4단계에서 넣은 타입별 `ValidateMontage`와 기반 도우미(`RequireMontage`, `ValidateStartSections`, `ValidateStageMontage`, `RequireSection`, `RequireNotify`, `GetFirstSectionName`), `UWxAbility_HitReact::GetReactionSectionNames`, 세트의 피격 반응 섹션 누락 검사를 지웠다. 이들만 쓰던 include도 뺐다.
  - 남은 검증: GA_의 `IsDataValid`(쿨다운 시간에 `UWxEffect_Cooldown` 파생 쿨다운 GE), 세트의 `IsDataValid`(속성 행, 빈 칸, 중복, 같은 입력의 조건 겹침, 같은 쿨다운 GE의 다른 값), 패시브 트리거 검사.
  - 남긴 것: 실행 코드가 쓰는 섹션 이름 상수(회피 `Backstep`·`Success` 접두사, 가드 반응 섹션 넷, `UWxAbilityBase::LandingSectionName`)와 `PlayMontage`의 없는 섹션 경고. 몽타주 도구(`WxAnimMontageToolset`)의 경계 맞춤 기능도 그대로다.
  - 다시 넣을 때 참고: 규칙과 반례 시험 결과는 위 "4단계 설계"·"4단계 결과"에 있다.
- 검증: Development·DebugGame 빌드 경고 없음. 데이터 검증 커맨드릿으로 GA_ 40개, 세트 9개 통과. 코드에 지운 함수 이름이 남지 않았다.
- 미확인: 인게임 락온. 사용자 플레이 확인 대상이다.

## 위험과 대응(설계에서 반영)

**오류 없이 조용히 틀어지는 곳**
- **CDO 읽기**: 한 클래스를 여러 행이 공유하므로 `Spec.Ability`나 GE 컨텍스트의 `GetAbility()`로 읽으면 기본값이 나온다. 대응은 두 가지다.
  - 행 데이터를 클래스 프로퍼티로 두지 않는다.
  - 행은 스펙에서만 읽는다. CDO에는 스펙이 없어 빈 값이 나온다. 처음 계획한 ensure는 엔진이 CDO로 부르는 정상 경로와 부딪혀 2단계 설계에서 뺐다.
- **클래스로 찾는 API**: `FindAbilitySpecFromClass`, `TryActivateAbilityByClass`, BP의 By Class 노드는 첫 스펙만 본다. 규칙으로 금지한다. 분신 따라하기는 클래스와 함께 `SourceObject`(테이블)와 동적 태그를 복사한다.
- **이름 규약 오타**: 반응·콤보·회피 섹션 이름, `Grounded` 이름이 틀리면 반응이 나오지 않거나(Normal 폴백 없음) 기본 방향으로 간다. 대응은 두 가지다.
  - 몽타주·테이블 저장 시 검증기를 돌린다.
  - 실행 중 누락 섹션은 경고를 한 번 남긴다.

**GA_ 방식 대비 잃는 것**
- **디버그 표시**: `showdebug abilitysystem`, 게임플레이 디버거, 엔진 로그가 클래스 이름으로만 표시된다. 행 태그를 찍는 Wx 로그로 보완한다(디버그 치트는 2단계 설계에서 뺐다).
- **다른 머신에서 행 식별 불가**: 스펙은 소유 클라에만 복제되고(`COND_ReplayOrOwner`, `AbilitySystemComponent.cpp:1862`), GE 컨텍스트로 복제되는 것은 공유 CDO다. 그래서 다른 머신은 어느 행에서 왔는지 알 수 없다. 지금 이것을 쓰는 코드는 없다. 필요해지면 행 태그를 따로 싣는다.
- **편집 충돌 단위**: GA_ 파일 단위에서 캐릭터 테이블 단위로 커진다(바이너리 잠금). 필요하면 더 잘게 나눈다.
- **정보 분산**: 테이블, 몽타주 노티파이(컷신·짝 몽타주·피해 행), 타입 규칙, 세트, BT에 나뉜다. 검증기로 보완하고, 행에서 노티파이 값을 보여 주는 에디터 표시는 후순위로 둔다.

**운영 규칙**
- **동적 태그**: 행 이름은 자유라 바꿔도 태그·BT에 영향이 없다. 한 테이블에 같은 타입 행을 더하면 동적 태그로 가르고, 새 태그가 필요하면 `WxGameplayTags`에 추가하고 빌드한다(Q-태그 B). 태그 이름을 바꾸면 코드의 태그와 BT 참조를 함께 바꾸고, 한 번에 고치기 어려우면 리디렉트 ini를 손으로 적는다(2026-09-25 세트 = 테이블 목록 기준).
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
- **에셋 태그를 곧 행 ID로 쓰기(2026-09-24 사용자 제안)**: GA_ 없이는 성립하지 않는다. 에셋 태그는 클래스에 붙고 생성자에서만 정한다(`GameplayAbility.cpp:1306`). `GetAssetTags()`는 가상 함수가 아니다(`GameplayAbility.h:173`). 그래서 한 클래스를 쓰는 행들은 같은 에셋 태그를 가진다. 행 태그를 싣는 `DynamicSpecSourceTags`(옛 이름 `DynamicAbilityTags`)가 엔진이 스펙마다 따로 주는 어빌리티 태그 칸이다. 엔진 주석도 스펙의 어빌리티 태그를 "에셋 태그와 DynamicAbilityTags를 합친 것"으로 설명한다(`GameplayAbility.h:521`). 이 설명 뒤 사용자는 기존 `Ability` 태그를 행 태그로 쓰는 쪽을 골랐다(2단계 설계의 결정 기록).

## 미결

검토 단계의 미결 두 건은 2026-09-24 사용자 동의로 확정했다.
- 소비 아이템은 인벤토리가 고른다.
- 컷신 시작 실패는 따로 처리하지 않는다.

컷신 중 입력 차단은 이후 사용자 제안으로 추가했다.

**Q-GA_ 복귀(2026-09-25, 결정 대기)**: 사용자가 GAS 일반 방식(어빌리티마다 GA_ 에셋)이 나은지 물었다.
- 처음에는 계속 진행을 권했고, 사용자는 "기획자가 BP로 만들 일은 없습니다"라고 답했다. 이어 "확신하나요?"라는 질문을 받고 다시 검증해 보니, 권한 근거가 틀렸다.
- 틀린 근거: "GA_는 변형 태그를 에셋 태그로 적어야 하는데, 에셋 태그와 취소·차단 태그가 같은 `Tags` 분류라 규칙을 숨길 수 없다"(`GameplayAbility.h:475`, `:738`~`:771`).
  - `UCLASS(HideCategories)`는 BP 파생 클래스가 물려받고(`BlueprintEditorUtils.cpp:1378`), 디테일 패널이 이를 따른다(`ObjectPropertyNode.cpp:534`).
  - 변형 태그를 Wx 프로퍼티로 두고 부여할 때 `DynamicSpecSourceTags`에 실으면 `Tags`·`Advanced`를 숨길 수 있다.
  - 전환 전 숨은 예외 3건도 GA_ 방식에서 모두 막힌다.
- 다시 비교한 결과:
  - GA_ 정리안(데이터 전용 GA_, 규칙 분류 숨김, 변형 태그를 스펙에 싣기)이 코드 양·순정·AI 함정에서 낫다.
  - 테이블은 편집 방식(캐릭터당 한 장, 행 추가, 한눈에 비교)에서만 앞선다.
- 사용자가 정할 것: 테이블 전환의 목적. 편집 방식이면 테이블을 유지하고, 규칙 잠금·정리면 GA_ 정리안으로 간다.
- 사용자가 밝힌 목적(2026-09-25): (1) AI가 데이터를 한눈에 보고 디버깅·작업하기 편하게, (2) GA_ 에셋이 늘어 관리가 까다로워지는 것을 막기. 둘 다 편집·조회 쪽 목적이라 테이블 유지를 권했다(사용자 확인 대기).
  - (2)는 GA_ 정리안으로는 이룰 수 없다. 어빌리티마다 에셋 하나가 그 구조 자체다.
  - (1)은 데이터 읽기에서는 테이블이 낫고, 코드 작성·런타임 디버깅에서는 GA_가 낫다(CDO·클래스 API 함정이 없다). 테이블 쪽 약점은 Wiki 함정 목록으로 메운다.
  - 다시 판단할 조건: 여러 사람이 같은 캐릭터 테이블을 동시에 고칠 때, 특정 행끼리 엔진 취소·차단 관계가 필요할 때, 다른 머신에서 행별 연출이 필요할 때.
- 기준 재정의(2026-09-25, 외부 사례 조사 뒤): 사용자 "AI 관점에서 구조 파악하고 작업하기 가장 편리한 방법으로 하고 싶습니다. 반드시 테이블화를 고집하려고 하는 것은 아닙니다." GA_ 정리안으로 돌아가기를 권했다(사용자 판단 대기).
  - 근거: 공개 선례가 없는 구조라 AI의 GAS 지식이 통하지 않고 함정 규칙을 매번 익혀야 한다. GA_는 엔진 로그·디버거·git 이력·파일 목록에서 어빌리티가 이름으로 보인다. 테이블이 앞서는 것은 한눈에 보기와 행 추가뿐이고, 한눈에 보기는 GA_ 전체 덤프로 대신한다. 값 읽기가 에디터에 묶이는 것은 두 방식이 같다.
  - 형태: 행 구조체를 `UWxAbilityBase` 프로퍼티로 옮겨 `GetAbilityRow()` 호출부를 그대로 둔다. 세트는 클래스 목록, 패시브는 GA_마다 엔진 `AbilityTriggers`를 쓴다. 행 검증은 `IsDataValid`로 옮긴다(엔진이 BP 저장 때 CDO의 `IsDataValid`를 부른다, `Blueprint.cpp:2233`). 타입 규칙, 몽타주 섹션, 노티파이, 쿨다운 그룹, BT 동적 태그는 유지한다.
  - 약점: 에셋 수 증가(목적 2 포기), BP 그래프 로직은 구조로 막지 못한다(관례), 이관 작업(GA_ 약 40개 스크립트 생성, 세트 9개, 테이블 삭제, 빌드·PIE 검증).

2단계 설계의 쟁점 두 건은 2026-09-24 사용자가 정했다. Q-단계는 A(2·3단계 통합), Q-태그는 B(C++ 네이티브)다(2단계 설계 → 쟁점).

**Q-궁극기(2026-09-24, 구현 중 발견, B로 확정)**: 확정한 "몽타주 먼저, 노티파이 시점에 컷신 시작"은 컷신 구조와 충돌한다. 사용자가 B를 골랐다(2026-09-24). 중간에 "노티파이 시점에 동기 로드해서 재생"을 제안했다가 "지금 방식으로"로 거두었다.
- 원인: 컷신은 머신마다 로컬 재생을 시작할 때 시전자 몽타주를 멈춘다(`UWxSkillCutsceneComponent::StartLocalPlayer`의 `StopAnimMontage`). 그 뒤 시퀀서가 Force Custom Mode로 포즈를 넘겨받는다. 그래서 몽타주 도중에 컷신을 열면 몽타주가 끊기고, 몽타주 중단 콜백이 어빌리티를 끝낸다. 검토 때 "전역 시간 정지라 몽타주도 멈춘다"고 본 것은 틀렸다.
- 선택지 A(시점형 유지): 노티파이 시점에 컷신을 열고, 머신마다 컷신이 끝나면 노티파이 시각부터 몽타주를 다시 튼다. 컷신 앞에 준비 동작을 둘 수 있다. 대신 궁극기에 장치가 셋 붙는다: 컷신 대기 상태, 대기 중 몽타주 중단 무시, 재개 시 컷신 노티파이 재발동 방지. 데디 서버는 로컬 재생이 없어 몽타주가 멈추지 않고 얼어 있다는 차이도 다뤄야 한다.
- 선택지 B(재생 전 읽기, 추천): 어빌리티가 재생 전에 몽타주의 컷신 노티파이에서 시퀀스를 읽고, 지금 흐름(컷신 → 몽타주)을 그대로 쓴다. 노티파이는 표식이라 불려도 하는 일이 없다. 코드 변경이 가장 적고 검증된 흐름을 유지한다. 컷신 앞 준비 동작은 둘 수 없다(지금도 없다).


## AI 추가 요청 처리 · 2026-09-25T16:55:15.010Z

<!-- test-feedback:request-f27f7ec5-a4ef-43c7-83ac-29cd403c3438:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 사람 확인 필요

요청:

> 이 작업은 테이블화를 안하는 쪽으로 마무리되었습니다. 기능은 전부 정상 작동하고 있습니다.

AI 요약:

> 마지막 요청은 기존 GA_ 복귀 결정에 따른 종료 통보이며, 새 구현이나 방향 변경 요청이 아닙니다. 테이블화를 진행하지 않고 GA_ 에셋 방식을 유지하며, 모든 기능이 정상 작동한다는 사용자 확인으로 이해했습니다. 추가 수정·테스트는 수행하지 않았고, 파일과 기존 사람 체크리스트 결과는 변경하지 않았습니다.


## AI 추가 요청 처리 · 2026-09-25T16:56:46.611Z

<!-- test-feedback:request-388f18b1-481a-4159-8e22-b3a2d5f3f3e4:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 사람 확인 필요

요청:

> 이 작업은 테이블화 안하는 쪽으로 결론났습니다. 테스트 했을 때에도 모두 이상 없습니다.

AI 요약:

> 테이블화를 하지 않고 GA_ 에셋 방식을 유지하는 것으로 결론을 확인했습니다. 기록상 GA_ 복귀 구현은 이미 끝났으며, 이번 요청은 사용자 테스트에서도 모두 이상 없었다는 최종 확인입니다. 추가 구현이나 새 계획은 필요하지 않습니다. 파일 수정·테스트 재실행은 하지 않았고, 기존 사람 체크리스트 결과도 변경하지 않았습니다.


## AI 추가 요청 처리 · 2026-09-25T17:03:12.801Z

<!-- test-feedback:request-677b2207-d1e0-4622-857a-bdc5d4a2b319:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 사람 확인 필요

요청:

> 이 작업은 테이블 전환을 하지 않는 것으로 마무리되었습니다. 테스트했을 때에도 문제 없었습니다.

AI 요약:

> 테이블 전환 없이 GA_ 에셋 방식을 유지하는 것으로 마무리되었으며, 사용자 테스트에서도 문제가 없었다는 최종 확인으로 이해했습니다. 기록의 기존 GA_ 복귀 결정과 일치하므로 추가 구현이나 새 계획은 필요하지 않습니다. 파일 수정·테스트 재실행은 하지 않았고, 사람 체크리스트 결과도 변경하지 않았습니다.


## 사용자 테스트 결과 · 2026-09-25T17:04:24.338Z

<!-- test-feedback:request-24a5a920-005b-4dfe-8d3e-77030b43ba20:submitted -->
- 전달한 사람: 이우성

> 통과 · HGTest 어빌리티
> 통과 · 분신
> 통과 · 도플갱어
> 통과 · 조작감
> 통과 · 가드 경감·버프 아이콘
> 통과 · 템플릿 패시브 UP
> 통과 · 락온


## AI 완료 정리 · 2026-09-25T17:04:24.345Z

<!-- test-feedback:request-24a5a920-005b-4dfe-8d3e-77030b43ba20-cleanup:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 정리 완료

AI 요약:

> GA_ 유지 결정과 사람 테스트 7개 통과 범위를 Wiki에 반영했습니다. 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 변경하지 않았습니다.
> 무관한 기존 변경과 다른 문서의 줄바꿈 경고는 그대로 두었습니다.

> 변경: .wiki/raw/notes/2026-09-26-ability-ga-play-acceptance.md에 최종 결정·테스트 범위·출처 해시를 수집했습니다.

> 변경: .wiki/wiki/concepts/combat-abilities.md에 사람 확인 결과를 반영하고 과거 플레이 미확인 설명을 갱신했습니다.

> 변경: Wiki 색인·원자료 수 102건·log.md와 Saved/Wiki 뷰어를 갱신했습니다.

> 근거: AGENTS.md, 워크플로우 정본, ability-table-driven.md, wiki 스킬과 .wiki/config.md·schema.md를 읽었습니다.

> 근거: 작업 기록 SHA-256은 처리 전후 접수 taskHash와 일치했습니다.

> 근거: 순정 llm-wiki lint --local --json: 통과, 오류·경고·제안 0건.

> 근거: Export-Wiki.ps1: Wiki·Workflow 뷰어 생성 성공. git diff --check -- .wiki: 공백 오류 없음.

> 근거: 빌드·게임 테스트는 재실행하지 않았습니다.
