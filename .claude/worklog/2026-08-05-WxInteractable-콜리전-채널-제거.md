# WxInteractable 콜리전 채널 제거

## 계획

### 목표

`DefaultEngine.ini`에 등록만 되어 있고 아무도 쓰지 않는 커스텀 오브젝트 채널 `ECC_GameTraceChannel2 = "WxInteractable"`을 제거한다. 헤더(`WxCollisionChannels.h`)는 자신을 커스텀 채널의 단일 정의처로 선언하는데 이 채널만 ini에 편측 존재해, 다음에 새 Wx 채널을 추가하는 사람이 헤더만 보고 GameTraceChannel2가 비었다고 판단하면 죽은 이름과 `DefaultResponse=Ignore`를 물고 있는 슬롯을 재사용해 원인 짚기 어려운 버그가 된다. 2026-08-03 리뷰에서 최초 지적 후 미수정 건이다.

### 사용처 없음 검증 (제거의 전제)

| 확인 경로 | 결과 |
|---|---|
| C++ 전체(`Source`+`Plugins`, Intermediate 제외) | `ECC_GameTraceChannel2` 참조 0건 |
| 모든 ini | 등록 줄 1개뿐. 커스텀 프로파일 2종(`WxProjectile`·`WxCharacterMesh`)이 ObjectType·CustomResponses로 안 씀 |
| 에셋 바이너리 `.uasset`/`.umap` (World Partition `__ExternalActors__` 포함) | `"WxInteractable"` FName 0건 (대조군 `WxAttack`은 130건 → grep 방법 유효) |
| 오브젝트 타입 쿼리 사용 에셋 | TargetingPreset 5종뿐이며 전부 `ECC_PAWN` |
| 상호작용 감지 실제 구현 | 채널이 아니라 전 오브젝트 쿼리 (`WxInteractionScannerComponent.cpp:166`의 `FCollisionObjectQueryParams::AllObjects`) |

`BP_ItemPickup` 덤프에 보이는 `WxInteractable → ECR_BLOCK`은 BP가 저자한 값이 아니라 C++ 생성자의 `SetCollisionResponseToAllChannels(ECR_Block)`(`WxItemPickup.cpp:30`) 부수효과가 런타임 CDO에 찍힌 것이다. 해당 `.uasset` 바이너리에는 그 이름이 없다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | `ECC_GameTraceChannel2` 채널 등록 줄 삭제 | 수정 |

### 접근 방식

- **ini 삭제 (슬롯 반납)**: 위 검증으로 이름 참조가 어디에도 없음이 확인됐으므로 등록을 지워 슬롯을 실제로 비운다. 다음 채널 추가자가 GameTraceChannel2를 깨끗한 상태로 쓸 수 있다.
- **헤더에 상수를 두지 않는다**: 앞서 검토한 대안(`ECC_WxInteractable` 상수를 헤더에 노출해 점유 사실을 드러내는 방향)은 슬롯을 계속 점유한다는 전제 위에 있었다. 슬롯을 반납하기로 한 이상 상수는 불필요하다.

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | `ECC_GameTraceChannel2 = "WxInteractable"` 등록 줄 삭제 | 수정 |

`WxCollisionChannels.h`는 변경 없음 — 중간에 넣었던 `ECC_WxInteractable` 상수를 원복해 최종 diff에서 빠졌다.

### 구현·결정과 그 이유

- **상수 노출 → 슬롯 반납으로 방향 전환**: 처음엔 무위험을 이유로 헤더에 상수만 노출했으나, 사용처 없음이 다섯 경로로 확인되면서 슬롯을 실제로 비우는 쪽이 근본 해결이 됐다. 상수는 "슬롯을 계속 점유한다"는 전제 위의 장치라 함께 걷어냈다.
- **에셋 후속 조치 없음**: 응답 오버라이드는 채널 FName으로 직렬화되는데 그 이름을 가진 에셋이 0건이라, 등록을 지워도 해석 실패할 참조가 없다.

### 계획 대비 달라진 점

계획대로.

### 후속 과제

- 컴파일 검증(`Result: Succeeded`)은 통과했으나 **ini 변경은 컴파일이 검증하지 못한다.** 실질 근거는 위 사용처 없음 감사다. 다음에 에디터를 열 때 Project Settings > Collision에서 GameTraceChannel2 슬롯이 비어 있고 WxAttack이 그대로인지 눈으로 확인하면 완전히 닫힌다.
- BP 그래프는 채널을 이름이 아닌 enum으로 저장해 이름 grep으로 못 잡는다. 이번엔 `ObjectTypeQuery` 문자열 보유 에셋을 따로 뒤져(TargetingPreset 5종, 전부 Pawn) 그 경로를 닫았다. `Plugins/WxBlueprintSnapshot/Snapshots/`가 실제로는 존재하지 않아 스냅샷 경유 검증은 불가했다 — 생성해두면 이후 BP 그래프 확인이 쉬워진다.
