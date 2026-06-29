# 처형 워프: 피해자 앵커 + 회전 정렬, WarpDistance 제거

## 계획

### 목표
처형 발동 시 공격자·피해자 위치가 어긋나는 문제를 잡는다. 워프 타겟을 "적 위치 − WarpDistance 지점 + 플레이어 회전 유지"가 아니라 **피해자 위치(공유 앵커) + 피해자를 바라보는 회전**으로만 등록하고, 멈출 간격·상대 포즈는 공격 몽타주의 Motion Warping Warp Point(애니)가 소유하도록 통일한다. GA 멤버 `WarpDistance`는 제거한다(간격 진실 공급원을 애니 하나로).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../WxAbility_Finisher.cpp` | `RegisterWarpTarget`을 피해자 위치 앵커 + 피해자 바라보는 회전으로 단순화(`WarpDistance`/`StopDistance`/`DirectionNorm`/`FMath::Max` 제거, `IsNearlyZero` 가드 유지). 정렬 관련 주석 갱신 | 수정 |
| `WxCombat/.../WxAbility_Finisher.h` | `WarpDistance` UPROPERTY 제거. 클래스 독 주석(정렬·회전 기준) 갱신 | 수정 |

### 접근 방식
- **피해자 앵커 + 회전 정렬**: 워프 타겟 위치 = `Target->GetActorLocation()`, 회전 = `(Target − Avatar)` 수평화 후 `.Rotation()`. 등록은 기존과 동일한 `AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName="Finisher", ...)`.
- **간격은 애니가 소유**: 멈출 거리·상대 포즈는 공격 몽타주의 Warp Point(Static/Bone)로 저작. 코드는 앵커·회전만 제공.
- **전제(콘텐츠, 코드 밖)**: `AM_Finisher` Motion Warping 노티파이의 `Warp Target Name = Finisher`, `Warp Point Anim Provider = Static/Bone`. 처형 애니를 피해자 앵커 기준 매칭 페어로 저작. 미저작 시 공격자가 피해자에 겹침.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../WxAbility_Finisher.cpp` | `RegisterWarpTarget`을 피해자 위치 앵커 + 피해자 바라보는 회전(`Direction.Rotation()`)으로 단순화. `WarpDistance`/`Distance`/`StopDistance`/`DirectionNorm`/`FMath::Max` 제거, `IsNearlyZero` 가드 유지. 정렬 주석(함수 내·`ActivateAbility`) 갱신 | 수정 |
| `WxCombat/.../WxAbility_Finisher.h` | `WarpDistance` UPROPERTY 제거. 클래스 독 주석 2·4단계 갱신(피해자 앵커·공격자가 피해자 바라봄·간격은 Warp Point). `WarpTargetName` 주석에 Warp Point 언급 | 수정 |

### 구현·결정과 그 이유
- **앵커=피해자 위치, 회전=피해자 바라봄**: 워프 타겟을 접근선 위 임의 지점이 아니라 공유 앵커(피해자)로 단일화해 진실 공급원을 하나로. 회전은 기존 "플레이어 방향 유지"에서 "피해자 바라봄"으로 바꿔, 짝 피격의 `FaceInstigator`(적→공격자)와 마주보게 정합.
- **간격을 애니로 위임**: 멈출 거리·상대 포즈를 GA 매직 플로트가 아니라 공격 몽타주 Warp Point가 소유. GA 멤버 `WarpDistance` 제거로 간격 이중통제 해소.
- **가드 유지**: 공격자·피해자 동일 위치(`IsNearlyZero`)만 예외 처리. 그 외 방어적 분기는 추가하지 않음.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **콘텐츠 필수(코드 밖)**: `AM_Finisher`(추후 분리될 뒤잡 몽타주 포함) Motion Warping 노티파이의 `Warp Target Name = Finisher`, `Warp Point Anim Provider = Static/Bone` 설정. 처형 애니를 피해자 앵커 기준 매칭 페어(루트모션 + Warp Point)로 저작. 미저작 시 공격자가 피해자에 겹침.
- 에디터 플레이로 앞잡(다양한 접근 각도)·뒤잡(후방) 정렬·마주봄·겹침 없음 점검.
