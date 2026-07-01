# WxInteractionComponent 활성(enable) 상태 복제

## 계획

### 목표
`UWxInteractionComponent`의 활성 상태(`bInteractionEnabled`)가 복제되지 않아, 서버 전용 타이머로 토글되는 enemy finisher 어포던스가 원격 클라에서 영원히 `NoCollision`으로 고정돼 스캔에 안 잡힌다. enable 상태를 컴포넌트 레벨에서 복제해 소비자 구동 방식(양측 ST / 서버 전용 타이머)과 무관하게 원격 클라까지 활성 상태를 일치시킨다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` | `GetLifetimeReplicatedProps` override 선언; `bInteractionEnabled`를 `UPROPERTY(ReplicatedUsing=OnRep_InteractionEnabled)`로 승격(인라인 기본값 `true`); `OnRep_InteractionEnabled`·`ApplyInteractionCollision` private 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | `Net/UnrealNetwork.h` include; 생성자의 `bInteractionEnabled=true` 라인 제거; `GetLifetimeReplicatedProps` 구현(DOREPLIFETIME); `SetInteractionEnabled`가 `ApplyInteractionCollision` 호출; `OnRep_InteractionEnabled`/`ApplyInteractionCollision` 구현 | 수정 |

### 접근 방식
- **컴포넌트 레벨 복제(순수 가산)**: 이미 `SetIsReplicatedByDefault(true)`라 property 복제가 바로 동작. `SetInteractionEnabled`는 권위·비권위 모두 즉시 로컬 콜리전을 적용(gimmick 양측 ST 즉시 구동 보존)하고, 권위 쓰기는 복제되어 클라 `OnRep`이 콜리전을 맞춘다(enemy 서버 전용 구동을 원격 클라에 반영). 서버 값이 클라와 같으면 OnRep 미발화, 다르면 서버 우선 수렴.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` | `GetLifetimeReplicatedProps` override 선언; `bInteractionEnabled`를 `UPROPERTY(ReplicatedUsing=OnRep_InteractionEnabled) bool = true`로 승격; `OnRep_InteractionEnabled`·`ApplyInteractionCollision` private 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | `Net/UnrealNetwork.h` include; 생성자의 `bInteractionEnabled=true` 라인 제거(인라인 기본값으로 이관); `GetLifetimeReplicatedProps` 구현(`DOREPLIFETIME`); `SetInteractionEnabled`·`OnRep_InteractionEnabled`가 `ApplyInteractionCollision` 공유 | 수정 |

### 구현·결정과 그 이유
- **콜리전 적용을 `ApplyInteractionCollision`로 공통화**: 로컬 경로(`SetInteractionEnabled`)와 복제 경로(`OnRep_InteractionEnabled`)가 동일 로직을 공유해 서버·클라 결과가 반드시 일치하도록.
- **`SetInteractionEnabled`는 권위·비권위 모두 즉시 로컬 적용 유지**: gimmick은 복제 State로 ST가 양측에서 이 함수를 구동하므로 즉시성이 필요. 순수 가산이라 기존 gimmick 동작 무변경.
- **OnRep은 변경 기반**: 서버 값이 클라와 같으면 미발화, 다르면 서버 우선으로 수렴 — 예측 없는 복제(`feedback_gimmick_state_no_prediction`) 철학과 일치.
- **추가 복제 비용 미미**: 컴포넌트가 이미 `SetIsReplicatedByDefault(true)`라 bool 한 개만 lifetime prop에 추가.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 성공(EXIT CODE 0). UHT가 새 `UPROPERTY(ReplicatedUsing)`/`OnRep` 정상 처리, WxWorld/WxGame 링크 정상. 로그: `.claude/skills/build-doctor/logs/build_2026-07-01_094010.log`.

### 후속 과제
- (범위 밖) `WxEnemyCharacter.cpp`의 `GetPlayerPawn(this, 0)` 백스탭 판정은 여전히 단일 로컬 플레이어 전제 — enemy finisher의 완전한 co-op 정합성은 별도 작업 필요.
- (선택) 런타임 검증: PIE Play As Client로 원격 클라에서 그로기 적의 처형 외곽선·프롬프트·발동 확인.
