# 처형 발동 서버 검증을 instigator 기준으로

## 계획

### 목표
`AWxEnemyCharacter::HandleFinisherInteracted`(서버 실행 검증)가 후방 판정에 `GetPlayerPawn(0)`을 쓰고 있어, co-op에서 **실제 상호작용한 플레이어가 아닌 player0(호스트) 기준**으로 백스탭 자격을 검증한다. `GetEligibleFinisherEventTag`를 상호작용 주체(Interactor) 파라미터를 받도록 바꿔, 발동은 실제 `InstigatorActor` 기준으로 검증하게 한다. (노출용 후방 정합성인 (2)는 범위 밖 — 노출은 로컬 player0 유지.)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxEnemyCharacter.h` | `GetEligibleFinisherEventTag()` → `GetEligibleFinisherEventTag(const AActor* Interactor)`. 주석 갱신(노출=로컬, 발동=instigator) | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | 정의를 파라미터화(후방 판정에 `GetPlayerPawn(0)` 대신 `Interactor` 위치 사용). `HandleFinisherInteracted`는 `InstigatorActor`, `UpdateFinisherAffordance`는 `GetPlayerPawn(0)` 전달 | 수정 |

### 접근 방식
- **주체 파라미터화**: 앞잡(그로기)은 주체 위치 불필요, 뒤잡의 후방 판정만 주체 위치 사용. 파라미터를 `const AActor*`로 받아 `GetActorLocation()`만 쓴다(픽업/폰 무관).
- **노출은 현행 유지**: `UpdateFinisherAffordance`는 로컬 player0을 넘겨 기존 노출 동작 그대로. 발동만 instigator로 검증 정합성 확보. SP는 player0=instigator라 무영향.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxEnemyCharacter.h` | `GetEligibleFinisherEventTag(const AActor* Interactor)`로 시그니처 변경 + 주석 갱신 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | 정의 파라미터화(후방 판정에 `Interactor` 위치 사용, `!Interactor` 가드 추가). `HandleFinisherInteracted`→`InstigatorActor`, `UpdateFinisherAffordance`→`GetPlayerPawn(0)` 전달 | 수정 |

### 구현·결정과 그 이유
- **후방 판정 주체를 파라미터로**: 앞잡(그로기)은 방향 무관이라 주체 위치를 안 쓰고, 뒤잡의 후방 원뿔만 주체 위치를 쓴다. `const AActor*`로 받아 `GetActorLocation()`만 사용(폰/픽업 무관, `!Interactor` 가드로 안전).
- **발동만 instigator, 노출은 player0 유지**: 발동(`HandleFinisherInteracted`)은 실제 상호작용한 `InstigatorActor`로 검증해 co-op에서 엉뚱한 플레이어 검증 버그를 제거. 노출(`UpdateFinisherAffordance`)은 (2)를 안 하므로 로컬 player0 그대로 → 노출 동작 무변경. SP는 player0=instigator라 완전 무영향.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 성공(EXIT CODE 0). UHT 정상, WxGame 컴파일·링크 정상. 로그: `.claude/skills/build-doctor/logs/build_2026-07-01_100602.log`.

### 후속 과제
- (2) 뒤잡 노출을 로컬로 내리는 정합성 개선은 미착수(기능급 — 복제 enable과 별개의 로컬 술어 훅 필요). co-op에서 정면 플레이어가 백스탭 프롬프트를 보되 눌러도 서버가 거부하는 UX는 남아 있다(익스플로잇은 안전).
