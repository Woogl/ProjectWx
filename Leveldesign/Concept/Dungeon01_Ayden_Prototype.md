# 던전 01 — 언리얼 프로토타입 구현 노트

> 최종 수정: 2026-03-26
> 목적: 기획서를 바탕으로 언리얼 엔진 5에서 프로토타입을 만들 때 참고할 구현 정보
> 엔진: Unreal Engine 5.7 / Blueprint

---

## 1. 구현 가능성 체크

### 전체 평가
기획서의 모든 핵심 기믹은 블루프린트로 구현 가능하다.
가장 까다로운 부분은 **빛 체인 반사(거울 → 거울)**이며, 이것이 이 던전의 핵심 블루프린트 학습 포인트가 된다.

---

### 구역별 구현 난이도

| 기믹 | 구현 방식 요약 | 난이도 |
|---|---|---|
| 카메라 연출 (동상 비추기) | Level Sequence 또는 Set View Target with Blend | ⭐ 초급 |
| 레버 조작 → 셔터 열림 | Timeline + Set Relative Rotation | ⭐ 초급 |
| 단일 빛줄기 | Niagara Beam Emitter | ⭐⭐ 중급 |
| 거울 단일 반사 | Line Trace → 반사점 계산 → 새 Beam 생성 | ⭐⭐ 중급 |
| 거울 체인 반사 (거울→거울) | 위 반사 로직을 반복 적용 | ⭐⭐⭐ 중급~고급 |
| 문양 활성화 연출 | Dynamic Material Instance + Timeline (Emissive 파라미터) | ⭐⭐ 중급 |
| 2단계 인지 시스템 | AI Perception 컴포넌트 + Sight 반경 두 단계 설정 | ⭐⭐ 중급 |
| 집결 신호 (대장 → 소환) | AI Controller에서 GameplayEvent 브로드캐스트 | ⭐⭐ 중급 |
| 가드 상태 감지 | Has Matching Gameplay Tag (ANS.Guard) | ⭐ 초급 |
| 가드 반사 방향 계산 | Reflect Vector 노드 | ⭐⭐ 중급 |
| 보스 페이즈 전환 | Health Attribute 임계값 → Event 발동 | ⭐⭐ 중급 |
| 보스 스턴 | Gameplay Effect (Duration 기반 Stun 태그 부여) | ⭐⭐ 중급 |
| 보스 처치 연출 | Level Sequence 또는 Timeline 체인 | ⭐⭐ 중급 |

---

## 2. 핵심 구현 포인트 상세

---

### A. 거울 빛 반사 체인

이 던전의 핵심 기믹. 빛줄기가 거울에 닿으면 반사각으로 꺾여 나가는 구조.

**작동 원리**
```
1. 광원(천창)에서 Line Trace 발사
2. 히트된 오브젝트가 거울이면 → Reflect Vector로 반사 방향 계산
3. 반사 방향으로 새 Line Trace 발사
4. 반복 (체인 길이만큼)
5. 최종 히트 지점이 문양이면 → 활성화 이벤트 발동
```

**블루프린트 핵심 노드**
- `Line Trace By Channel` — 빛 경로 판정
- `Reflect Vector` — 반사 방향 계산 (입사 벡터 + 표면 노멀 필요)
- `Get Hit Normal` — 반사면의 노멀 벡터 추출
- Niagara Beam — 각 구간의 빛줄기 시각화

**주의사항**

- Niagara Beam은 시작점과 끝점만 지정 → 꺾이는 지점마다 별도 Beam 액터 필요

- **빛 경로 재계산 타이밍 — 레버 조작할 때만 하도록 만들 것**

  게임은 1초에 수십 번씩 화면을 갱신한다 (이걸 "틱"이라고 부름).
  "틱마다 재계산"으로 만들면 거울을 안 건드리고 있어도 1초에 수십 번씩 빛 경로를 다시 계산하게 돼서 성능이 나빠진다.
  대신 **레버를 조작하는 순간에만** 재계산하도록 만들면 성능 부담이 없음.
  블루프린트에서는 레버 조작 이벤트 → 빛 경로 재계산 함수 호출 순서로 연결하면 됨.

- **거울이 여러 개 이어질 때 재계산 순서 주의**

  2구역처럼 거울 1 → 거울 2 → 거울 3으로 빛이 이어지는 경우,
  거울 1의 각도를 바꾸면 거울 2, 3의 경로도 전부 다시 계산해야 함.
  레버 1개를 조작했을 때 체인 전체를 처음부터 순서대로 다시 계산하도록 구성할 것.
  중간 거울만 따로 재계산하면 앞뒤가 안 맞아서 빛이 이상하게 그려질 수 있음.

---

### B. 가드 상태 감지 + 반사

최종 관문과 보스 기믹, 비밀방 모두 이 구조를 공유한다.

**작동 원리**
```
1. 빛이 플레이어 캐릭터에 Line Trace로 히트
2. Hit Actor에서 Ability System Component 가져오기
3. Has Matching Gameplay Tag (ANS.Guard) 확인
4. True → Reflect Vector로 반사 방향 계산 후 새 Line Trace
5. 반사된 빛이 문양/타겟에 닿으면 이벤트 발동
```

**블루프린트 핵심 노드**
- `Has Matching Gameplay Tag` — 가드 상태 확인
- `Reflect Vector` — 반사 방향
- `Get Actor Forward Vector` — 캐릭터가 바라보는 방향 (반사각 기준)

---

### C. 2단계 인지 시스템

**작동 원리**
```
AI Perception 컴포넌트에서 Sight 설정:
- 감지 반경 (넓음) → 경계 상태로 전환 (고개 돌림, 대사)
- 전투 반경 (좁음) → 공격 시작
또는
- 단일 Sight로 감지 후 → Timer (2초) → 전투 시작
  (이 방식이 블루프린트로 더 간단함)
```

**권장 방식 (블루프린트 초보)**
AI Perception 하나 + Timer by Event 조합이 구현이 가장 단순하다.

---

## 3. 권장 구현 순서 (프로토타입)

블루프린트 학습 난이도 기준으로 단계적으로 쌓아가는 순서.

```
STEP 1 — 레버 + 셔터 + 빛줄기
  레버 상호작용 → Timeline으로 셔터 회전 → Niagara Beam 활성화
  목표: 상호작용 + Timeline + Niagara 기초 익히기

STEP 2 — 거울 단일 반사
  Line Trace → 거울 히트 → Reflect Vector → 새 Beam
  목표: Line Trace와 벡터 수학 기초 익히기

STEP 3 — 거울 체인 반사 + 문양 활성화
  위 로직 반복 + Dynamic Material Instance
  목표: 반복 구조 + 머티리얼 제어

STEP 4 — 최종 관문 반사 기믹
  조각상 신호 시퀀스 → 빛 반사 → 순서 활성화 → 문 개방
  반사 방식: 가드 반사(Has Matching Gameplay Tag → Reflect Vector) 또는 거울 조작 — 회의 후 확정
  목표: GAS 연동 또는 레버 인터페이스 + 순서 활성화 시퀀스 구현

STEP 5 — AI 인지 + 집결 신호
  AI Perception + Timer + Event 브로드캐스트
  목표: AI 기초 익히기

STEP 6 — 보스 기믹
  페이즈 전환 + 빛 방출 + 스턴 Effect
  목표: 보스 시스템 완성
```

---

## 4. 액터 구조 설계 (예상)

프로토타입에서 만들어야 할 블루프린트 액터 목록.

| 액터 이름 (예상) | 역할 |
|---|---|
| BP_LightSource | 천창 광원. 빛줄기 시작점. |
| BP_Mirror | 거울. 레버 상호작용 + 반사 계산. |
| BP_LightRune | 문양. 빛이 닿으면 활성화 이벤트 발동. |
| BP_Lever | 레버. 상호작용 시 연결된 액터에 이벤트 전달. |
| BP_SealedDoor | 봉인된 문. 활성화 이벤트 수신 시 열림 연출. |
| BP_SecretDoor | 비밀방 문. 특수 문양 활성화 시 열림. |
| BP_Enemy_Thief | 도굴꾼 기본 AI. |
| BP_Enemy_Archer | 도굴꾼 궁수 AI. |
| BP_Enemy_Captain | 도굴꾼 대장 AI. 집결 신호 브로드캐스트 포함. |
| BP_Boss_Guardian | 신전 수호자 보스. 페이즈 관리 + 빛 방출 기믹. |

---

## 5. 미확정 — 구현 전 결정 필요

- 최종 관문 반사 방식 확정 (가드 반사 vs 거울 조작 — 팀 회의 예정)
- 거울 레버 인터페이스 방식 (45도 단계 회전 vs 연속 회전)
- 빛 체인 재계산 타이밍 (Tick vs Event 기반)
- 보스 스턴 지속 시간 및 피격 판정 범위
- 비밀방 내부 보상 내용
