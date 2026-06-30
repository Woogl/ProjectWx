# 상호작용 OnInteracted 서버 권위 전용화 — 멀티캐스트·BP 노출 제거

## 계획

### 목표
`UWxInteractionComponent::OnInteracted` 가 비신뢰 `NetMulticast` 로 서버+모든 클라에서 fire되어 "권위 트리거 + 코스메틱 통지"를 한 채널에 섞던 것을, 서버 권위에서만 fire되는 이벤트로 단순화한다. 전수 조사 결과 모든 소비자(11개 핸들러)가 `HasAuthority()` 가드로 권위에서만 동작하고 클라 비주얼은 별도 복제로 수렴하므로, 클라 fire가 불필요하다. 함께 미사용 `BlueprintAssignable` 노출도 제거한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` | `MulticastInteracted` UFUNCTION 선언 제거. `OnInteracted` 에서 `BlueprintAssignable` 제거(`UPROPERTY()` + dynamic 시그니처 유지). 헤더 흐름 주석·`OnInteracted` 주석 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | `TryInteract` 가 `MulticastInteracted` 대신 `OnInteracted.Broadcast` 직접 호출. `MulticastInteracted_Implementation` 삭제. `SetIsReplicatedByDefault(true)` 유지하되 주석을 TargetData 직렬화 근거로 갱신 | 수정 |
| `Plugins/WxCore/Source/WxCore/Public/WxInteractionSource.h` | 델리게이트 "서버+모든 클라에서 fire" 주석을 "서버 권위에서만 fire" 로 갱신 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 멀티캐스트 근거 주석을 TargetData 직렬화 근거로 갱신(`bReplicates = true` 는 유지) | 수정 |

### 접근 방식
- **멀티캐스트 제거**: `TryInteract` 는 이미 `Owner->HasAuthority() && bInteractionEnabled` 가드를 통과한 뒤에만 진행하므로, 그 자리에서 `OnInteracted.Broadcast` 를 직접 호출하면 이벤트는 서버에서만 fire된다. 원격 클라로의 비신뢰 RPC 전파가 사라진다.
- **복제는 유지**: `OnInteracted` 의 클라 전파는 끊지만, `FWxAbilityTargetData_Interaction` 이 컴포넌트 포인터를 PackageMap 으로 클라→서버 직렬화하므로 동적 스폰 액터(픽업·적)의 컴포넌트가 net-addressable 해야 한다. 따라서 `SetIsReplicatedByDefault(true)` 와 각 액터의 `bReplicates` 는 그대로 둔다.
- **BP 노출 제거**: BP 스냅샷 바인딩 0건 확인. `BlueprintAssignable` 만 제거하고, C++ `AddDynamic` 바인딩(핸들러 11개)을 위해 dynamic 델리게이트와 `UPROPERTY()` 는 보존한다.

### 사이드이펙트 검증(요약)
- 소비자 전수(기믹 7 + LaserCorridor + CheckPoint + EnemyCharacter 피니셔 + ItemPickup)가 모두 `HasAuthority()` 가드 → 클라 fire 의존 없음.
- 클라 비주얼 수렴 경로 확인: 기믹 State 복제(OnRep→ST), 픽업 Destroy 복제, 피니셔 GAS 이벤트→어빌리티 복제, 체크포인트 힐 GE 복제.
- 잠재 리스크: 스냅샷 미반영(미저장) BP 가 `OnInteracted` 바인딩 시 `BlueprintAssignable` 제거로 로드 경고 → 즉시 표면화.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Interaction/WxInteractionComponent.h` | `MulticastInteracted` 선언 삭제, `OnInteracted` 를 `BlueprintAssignable`→`UPROPERTY()`, 흐름·델리게이트 주석 갱신 | 수정 |
| `Plugins/WxWorld/.../Interaction/WxInteractionComponent.cpp` | `TryInteract` 가 `OnInteracted.Broadcast` 직접 호출, `MulticastInteracted_Implementation` 삭제, 복제 유지 근거 주석 추가 | 수정 |
| `Plugins/WxCore/.../WxInteractionSource.h` | `GetOnInteractedDelegate` fire 위치 주석 갱신(서버 전용) | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | `bReplicates` 근거 주석을 TargetData 직렬화로 갱신(값 유지) | 수정 |

### 구현·결정과 그 이유
- **멀티캐스트 제거, 서버 직접 Broadcast**: `TryInteract` 의 권위 가드를 통과한 지점에서 곧바로 `OnInteracted.Broadcast`. 소비자 11개 전부 `HasAuthority()` 가드라 클라 fire 의존이 없고, 클라 비주얼은 복제(State/Destroy/GAS/GE)로 수렴함을 전수 확인 → 비신뢰 RPC 전파가 순수 잉여였다.
- **복제는 유지**: 클라 fire는 끊되 `SetIsReplicatedByDefault(true)` 와 액터 `bReplicates` 는 보존. TargetData 가 컴포넌트 포인터를 PackageMap 직렬화하므로 동적 스폰 액터의 컴포넌트가 net-addressable 해야 한다(초기 진단의 "복제도 끌 수 있다"는 오판을 정정).
- **BP 노출 제거**: 스냅샷 바인딩 0건 + 사용자 확인. `BlueprintAssignable` 만 제거하고 C++ `AddDynamic`(핸들러 11개) 위해 dynamic·`UPROPERTY()` 보존.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 미검증: 스냅샷에 안 잡힌 미저장 BP 가 `OnInteracted` 에 바인딩돼 있었다면 로드 시 경고로 표면화됨(빌드는 영향 없음). 에디터 로드 시 확인 권장.
- 별건(이번 범위 밖): 진단 1번(서버가 클라 제출 TargetData 대상의 거리·자격을 재검증하지 않는 신뢰 경계 문제)은 미해결로 남음.
