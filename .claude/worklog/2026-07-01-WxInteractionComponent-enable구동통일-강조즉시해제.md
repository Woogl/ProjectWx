# WxInteractionComponent enable 구동 통일(B) + 비활성 시 강조 즉시 해제(C)

## 계획

### 목표
A(enable 복제) 위에서 두 가지를 정리한다. **B**: enemy가 클라 포함 양측에서 `SetInteractionEnabled(false)`를 블라인드 라이트해, 레이트조인 시 이미 복제된 `true`를 클라 로컬로 덮어써 처형이 영구 불가해지는 문제를 enemy 토글을 권위 전용으로 바꿔 해결(클라는 복제만 추종). **C**: 비활성 후 외곽선이 레지스트리 다음 스캔(≤0.1s)까지 남는 잔상을, 콜리전 적용 choke point에서 비활성 시 강조를 즉시 꺼 제거.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | `BeginPlay`의 `SetInteractionEnabled(false)`를 어포던스 타이머와 함께 `if (HasAuthority())` 블록으로 이동. `OnInteracted` 바인딩은 양측 유지 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | `ApplyInteractionCollision()`에 `if (!bInteractionEnabled) SetHighlightEnabled(false);` 추가 | 수정 |

### 접근 방식
- **B(권위 전용 구동)**: enemy 어포던스는 서버 권위 계산이라 클라가 파생 불가 → 클라는 A의 복제(`OnRep`)만 추종해야 한다. 클라의 블라인드 라이트를 제거하면 레이트조인 클로버가 사라진다. Gimmick은 복제 State에서 ST로 enable을 재파생(자기일관·지연0)하므로 양측 구동을 그대로 둔다.
- **C(choke point에서 해제)**: `SetInteractionEnabled`(로컬)·`OnRep`(복제) 둘 다 `ApplyInteractionCollision`을 거치므로, 여기서 비활성 시 강조를 끄면 리슨 호스트·원격 클라 양쪽에서 즉시 정리된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | `BeginPlay`에서 `SetInteractionEnabled(false)`를 어포던스 타이머와 함께 `if (HasAuthority())` 블록으로 이동. `OnInteracted` 바인딩은 양측 유지 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | `ApplyInteractionCollision()`에 비활성 시 `SetHighlightEnabled(false)` 추가 | 수정 |

### 구현·결정과 그 이유
- **B — enemy enable을 권위 전용으로**: enemy 어포던스는 서버 권위 계산(ASC 태그·player0 위치)이라 클라가 파생 불가하므로, 클라는 A의 복제만 추종해야 한다. 클라의 블라인드 라이트를 제거해 "레이트조인 시 복제된 `true`를 로컬 `false`로 덮어써 처형이 영구 불가"해지던 클로버를 없앴다.
- **Gimmick 무변경**: gimmick은 복제 State에서 ST가 enable을 재파생하므로 양측 값이 항상 일치(자기일관·지연0) → 그대로 두는 것이 오히려 옳다. "클라가 파생 가능한 값만 클라에서 써도 된다"는 원칙으로 두 소비자를 구분.
- **C — choke point에서 강조 해제**: `SetInteractionEnabled`(로컬)·`OnRep_InteractionEnabled`(복제)이 공유하는 `ApplyInteractionCollision`에서 비활성 시 강조를 끄면, 리슨 호스트·원격 클라 양쪽에서 한 곳으로 즉시 정리된다. 활성화 시엔 강조를 건드리지 않아(레지스트리가 선택 대상만 켬) 책임 경계 유지.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 성공(EXIT CODE 0). `WxEnemyCharacter.cpp`·`WxInteractionComponent.cpp` 컴파일 및 WxGame/WxWorld 링크 정상. 로그: `.claude/skills/build-doctor/logs/build_2026-07-01_095012.log`.

### 후속 과제
- (범위 밖) `WxEnemyCharacter.cpp`의 `GetPlayerPawn(this, 0)` 백스탭 판정은 단일 로컬 플레이어 전제 — enemy finisher의 완전한 co-op 정합성은 별도 작업.
- (별개) 프롬프트 리스트는 비활성 후 레지스트리 다음 스캔(≤0.1s)까지 남는다(외곽선은 이번에 즉시 해제됨). 필요 시 별도 개선.
- (선택) 런타임 검증: PIE Play As Client로 (B) 레이트조인 처형 노출, (C) 발동 직후 외곽선 즉시 소멸 확인.
