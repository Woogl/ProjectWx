# SnapToTarget 개선: #2 디싱크 · #3 부위/수직 · #4 테일 이중 돌진

## 계획

### 목표
`UWxAnimNotifyState_SnapToTarget`의 3가지 문제 수정. AnimNotifyState 인스턴스는 모든 재생이 공유 → per-play 멤버 상태 불가.

### 수정 범위
| 모듈 | 파일 | 내용 |
|---|---|---|
| `WxCombat` | `AnimNotify/WxAnimNotifyState_SnapToTarget.h/.cpp` | #2 위치워프 락온대상 한정, #3 부위+3D 접근, #4 테일 hold를 NotifyEnd로 |

### 접근 방식
- **#2**: `bShouldWarpTranslation = bSnapLocation && bTargetInSnapRange && (FacingTarget == LockOnTarget)`. 위치 스냅은 복제된 락온 대상에만, 폴백은 회전만 → 디싱크 제거.
- **#3**: 락온 SceneComponent 유지. TargetPoint XY=부위(`GetComponentLocation`), Z=`FacingTarget` 액터 Z(접지, 부양 방지). 3D 접근/거리, 회전은 yaw 전용.
- **#4**: NotifyBegin 테일 블록 제거 → `NotifyEnd`에서 그 시점 실제 위치를 hold(`[EndTrigger, PlayLength]` translation-only SkewWarp). 메인 도달/막힘 무관하게 잔여 전진만 억제, 2차 돌진 없음. `bSnapLocation`(CDO-safe)으로 게이팅.

WxCombat 내부 한정, WxCore·의존성 무변경.

---

## 완료

### 수정한 파일
| 모듈 | 파일 | 수정 내용 |
|---|---|---|
| `WxCombat` | `AnimNotify/WxAnimNotifyState_SnapToTarget.h` | `NotifyEnd` 오버라이드 선언 추가 |
| `WxCombat` | `AnimNotify/WxAnimNotifyState_SnapToTarget.cpp` | #2 위치워프 락온대상 게이팅, #3 부위 XY + 3D 접근(bIgnoreZAxis=false) + yaw 회전, #4 테일을 NotifyEnd 실위치 hold로 이동 |

### 구현·결정과 그 이유
- **#2 디싱크(플레이어 한정으로 보정)**: 디싱크는 클라가 예측하는 **플레이어 폰**에서만 발생하므로, 락온 대상 게이팅을 `OwnerPawn->IsPlayerControlled()`일 때만 적용(`bRequireLockOnForTranslation`). 즉 플레이어는 복제된 락온 대상에만 위치 스냅(폴백은 회전만), **AI 등 서버 권위 폰은 폴백 위치 스냅 유지**. IsPlayerControlled는 소유 클라/서버 양쪽에서 일관돼 그 쌍의 결정이 일치 → 러버밴딩 제거하면서 AI 돌진 보존.
- **#3 부위/수직 → 수평 전용 유지(원복)**: 처음엔 부위 XY + 3D 접근(`bIgnoreZAxis=false`)으로 높이차를 반영했으나, **캐릭터 액터 위치 = 캡슐 센터**라 거대 적 상대로 공격자가 위로 끌려 올라가는 부작용 발견. 수직 처리의 이득(단차/공중 정렬)은 마이너하고 CMC step-up이 이미 흡수하므로, **원래의 수평 전용**(`Direction.Z=0`, `bIgnoreZAxis=true`, 액터 위치 기준)으로 되돌림. 결과적으로 #3은 무변경(원동작 유지).
- **#4 테일**: NotifyBegin의 begin-시점 테일(고정 WarpLocation 재워프) 제거. NotifyEnd에서 그 시점 실제 위치를 hold(XY only, [EndTrigger,PlayLength])로 등록 → 메인 도달/막힘 무관하게 2차 돌진 없음. AnimNotifyState 인스턴스 공유 제약 때문에 per-play 멤버 없이, NotifyBegin의 위치 스냅 조건과 동일하게 게이팅(플레이어 폰이면 락온 있어야 hold; 범위 체크만 제외). 이로써 "락온 없는 플레이어가 종료 후 제자리에 갇히는" #2·#4 결합 부작용도 방지.

### 계획 대비 달라진 점
- 계획대로. (계획에 없던 디테일: 메인 modifier `bIgnoreZAxis`를 false로 바꿔야 #3 수직이 실제 적용됨 — 반영.)

### 후속 과제
- 분석 문서 `Docs/Programmer/Snap_To_Target.md`의 「주의할 점」·「네트워크」 절을 갱신하면 좋음(선택).
