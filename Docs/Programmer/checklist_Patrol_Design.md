# 적 정찰 역할 — 구현 체크리스트

> `Docs/CombatDesign/Patrol_Design.md` 기준의 **예상 작업 목록**이다. 구체적 구현 방법·로직·수치 설계는 담지 않는다 — 실제 설계는 구현 단계(plan mode·worklog)에서 정한다.

## 개요
개별 적 인스턴스(일반·엘리트)에 켜는 "정찰 역할" — **순찰 → 추격 → 귀환** 상태를 도는 AI 행동이며, 들키면 교전이 시작되는 전투 트리거다. 걸치는 도메인: **AI(WxAI)** 중심 + **레벨 배치**(정찰 역할 토글·순찰 경로) + **전투**(교전 진입, 재사용). WxAI가 정찰 경로·감지·추격·리시(복귀) 인프라를 이미 제공하므로, 대부분 **기존 요소 재사용·설정·BT 조립**에 가깝다.

## 작업 목록

### AI — 정찰 행동 (순찰→추격→귀환)
- ❓ 정찰 역할 BT 브랜치: 순찰→추격→귀환 상태 전이 구성 (2) — 기존 정찰/복귀 Task·리시 데코레이터 조립
- ✅ 순찰: 지정 경로를 점→점(A→B→A)으로 이동 (2, 3) — 기존 `UWxPatrolComponent`(정찰 경로) 재사용
- ❓ 순찰 지점 도착 시 5초 정지 (3)
- ⚠️ 순찰 이동 속도 = 기본 걷기 속도의 0.75배 (3)
- ✅ 플레이어 인식 시 순찰→추격 전환 (2) — 기존 Perception 전투 인식 재사용
- ✅ 추격 중 사정거리 진입 시 공격 (2) — 기존 교전/어빌리티 발동 경로 재사용
- ✅ 추적 종료 조건 충족 시 귀환 (2, 5) — 기존 리시/복귀 메커니즘 재사용
- ✅ 귀환: 정찰 시작 지점까지 복귀 후 순찰 재개 (2, 5)

### AI — 감지
- ✅ 플레이어 감지 규칙 — 네임플레이트 기획과 동일, 기존 `UWxAIPerceptionComponent` 재사용 (4)
- ✅ 감지 비전파: 감지 시 주변 다른 적에 영향 없음 (4)

### 레벨 배치 / 설정
- ⚠️ "정찰 역할" 인스턴스 토글 — 개별 적에 지정, 보스 제외 (0)
- ✅ 순찰 경로 지정 — 순찰 지점(A/B) 배치·편집 (3)
- ✅ 정찰 시작 지점(홈) = 귀환 목표 설정 (5)

## 열린 질문 / 기획 공백
- **감지·추적 종료·사정거리 수치가 위임됨** — 4·5절이 "네임플레이트 기획과 동일"이라 값을 담지 않는다. 해당 기획서 확인 필요.
- **순찰 지점 수·순회 방식** — 기획은 A→B→A(2점 왕복)인데, 기존 정찰 경로는 스플라인 기반이다. 3점 이상/순환 지원과 왕복 vs 순환을 확정해야 한다.
- **"정찰 역할" 토글의 형태** — 폰 속성인지, 컴포넌트 부착인지, 스포너 설정인지, 레벨디자이너 워크플로우가 미정.
- **5초 정지 중 감지 여부** — 정지 동안에도 플레이어를 인식하는지 명시 없음.
- **귀환 도중 재감지 처리** — 홈 도착 전 재발견 시 즉시 재추격인지, 기존 리시 규약(복귀 중 재-어그로 억제)과 기획 의도가 일치하는지 확인 필요.
- **정찰 역할 off인 적의 기본 행동** — 제자리 대기 등, 정찰 미지정 적과의 관계가 없다.
- **"기본 걷는 속도" 기준** — 0.75배의 기준 속도가 적 타입별로 다른지 불명확.

---
*`Docs/CombatDesign/Patrol_Design.md` 기준 · 생성일 2026-07-24 — `/design-checklist`로 갱신*

## 점검 특이사항

> 커밋 `10f1722b` · 점검일 2026-07-24 · ✅9 ⚠️2 ❌0 ➕4 ❓2. 코드·에셋 존재 대조 기반이라 런타임 데이터(DataTable 값·BP/BT 내부)·의도된 미구현까지는 단정하지 못한다. WxAI는 BT 스냅샷이 없어 BT 그래프 내부 구성·노드 값은 범위 밖이다.

- ⚠️ **순찰 이동 속도 0.75배** — 배율 메커니즘은 존재하나(`WxBTTask_Patrol`의 `MoveSpeedMultiplier`가 MaxWalkSpeed를 곱해 제한, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-61`) C++ 디폴트가 **0.5**로 기획의 0.75와 다르다(`Public/WxBTTask_Patrol.h:31-33`). 실제 적용값은 BT 노드 데이터라 미확인, 기본 걷기 속도는 400(`Source/WxGame/Character/WxEnemyCharacter.cpp:25`). 확신도 중간.
- ⚠️ **"정찰 역할" 인스턴스 토글** — 명시적 toggle/bool 없이 "스포너(Owner)에 `UWxPatrolComponent` 부착"으로 성립한다(없으면 Patrol Failed→배회 폴백, `WxBTTask_Patrol.cpp:32-37`). 보스 제외는 암묵적(보스는 순찰 컴포넌트 미부착). 열린 질문의 "토글 형태 미정"과 일치. 확신도 중간.
- ➕ **비-정찰 폴백 배회 `WxBTTask_Wander`** — 정찰 안 하는 적(Patrol Failed 캐스케이드)의 8방향 무작위 배회(`Plugins/WxAI/Source/WxAI/Public/WxBTTask_Wander.h`). 열린 질문 "정찰 역할 off인 적의 기본 행동"에 대응하는 구현이 이미 존재.
- ➕ **전투 중 실시간 리시 abort** — BeyondLeash가 관찰자로 매 프레임 이탈을 폴링하다 값 전환 순간 진행 중인 전투 패턴도 중단·복귀시킨다(`WxBTDecorator_BeyondLeash.cpp:51-64`). 단순 "종료 조건 시 귀환"보다 정교.
- ➕ **strafe 응시 회전 발행 + 사망 인식 정리** — Perception이 TargetActor 유무로 폰 회전 모드(타겟 응시 ↔ 이동방향)를 발행하고(`WxAIPerceptionComponent.cpp:207-252`), State.Dead에 TargetActor·State.InCombat을 정리해 시체 위 인식 잔존을 막는다(`:194-205`).
- ➕ **무작위 공격 패턴 인프라** — `WxBTComposite_RandomChoice`+`WxBTDecorator_RandomWeight`(가중 추첨)+`WxBTDecorator_AttributeRatio`(HP 비율)가 추격 후 교전 패턴 분기를 지원. 정찰 범위 밖이나 "사거리 진입 시 공격"을 뒷받침.
- ❓ **정찰 역할 BT 브랜치** — 구성 요소(`WxBTTask_Patrol`·`WxBTTask_ReturnHome`·`WxBTDecorator_BeyondLeash`·`WxBTService_TargetDistance`+Perception)는 전부 존재하나, 하나의 브랜치로 조립한 결과는 BT 그래프 내부다(`Content/AI/BT_Enemy*.uasset` 존재). 확인 필요: 리시(Lower Priority) 복귀 > 전투 > 순찰 우선순위 배치.
- ❓ **순찰 지점 도착 시 5초 정지** — `WxBTTask_Patrol`은 스스로 대기하지 않고 `[Patrol→Wait]` 시퀀스를 전제한다(`Public/WxBTTask_Patrol.h:14`). "5초"는 엔진 `UBTTask_Wait` 노드 값이라 존재만으론 판정 불가. 확인 필요: BT_Enemy* 순찰 시퀀스의 Wait Duration.

### 기획-코드 갭 (조치 제안)
- **기획서 최신화 필요**: 순찰 속도 C++ 디폴트 0.5 vs 기획 0.75 재확정 · "정찰 역할 토글 = 스포너에 `UWxPatrolComponent` 부착"으로 명문화 · ➕ 4건(배회·실시간 리시 abort·strafe 응시·무작위 패턴)을 기획서에 반영.
- **구현 필요**: 없음(❌ 0). 잔여는 코드가 아니라 **BT/BB 에셋 결선·값**(5초 Wait·0.75 배율·순찰→추격→귀환 우선순위 브랜치)과 **레벨 배치**(스포너에 순찰 컴포넌트·경로) — BT 스냅샷 부재로 이 점검으론 확인 불가.

---
*점검 커밋 `10f1722b` · 점검일 2026-07-24 — `/checklist-verify`로 갱신*
