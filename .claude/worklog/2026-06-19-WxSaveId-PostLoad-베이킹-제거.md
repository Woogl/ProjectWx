# WxSaveId 부여에서 PostLoad 제거

## 계획

### 목표
`IWxSavable` 구현 액터의 세이브 키(`WxSaveId`) 부여를 `PostActorCreated`+`PostDuplicate` 두 훅으로 줄이고, 중복적이고 미묘했던 `PostLoad` 베이킹을 제거한다. 공통 부모 클래스는 도입하지 않는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` | `#if WITH_EDITOR` 블록에서 `PostLoad` 선언 제거(`PostActorCreated`·`PostDuplicate` 유지). 주석의 PostLoad 언급 정리 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp` | `PostLoad` 정의 제거. 베이킹 주석 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` | `PostLoad` 선언 유지(프리뷰용). 변경 없음/주석 정리 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp` | `PostLoad` 본문에서 `WxSaveId = GetActorGuid();` 한 줄만 제거. 프리뷰 갱신은 유지. 베이킹 주석 갱신 | 수정 |

### 접근 방식
- **PostLoad 안전망 제거**: `PostLoad` 는 "레벨 재저장 없이도 쿠킹 시 키 보장"용 안전망. `PostActorCreated`(생성)+`PostDuplicate`(복제)가 새 액터 경로를 덮고, 부여된 값은 에셋 저장 시 직렬화되어 쿠커가 그대로 읽으므로 불필요하다.
- **Gimmick**: PostLoad 가 키 베이킹만 했으므로 통째로 삭제.
- **Spawner**: PostLoad 는 에디터 프리뷰 갱신(Transient 프리뷰 컴포넌트 복원)에도 쓰이므로 유지하되 키 베이킹 라인만 제거.
- **특수 키**: `GetWxSaveId()` 가 virtual 이라 자식이 오버라이드로 교체 가능(추가 장치 불필요).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Public/Gimmick/WxGimmick.h` | `#if WITH_EDITOR` 블록에서 `PostLoad` 선언 제거, 주석 라벨 `AActor/UObject`→`AActor` | 수정 |
| `Private/Gimmick/WxGimmick.cpp` | `PostLoad` 정의 제거, 베이킹 주석을 PostLoad 무관하게 갱신 | 수정 |
| `Private/Spawnable/WxSpawner.cpp` | `PostLoad` 본문의 `WxSaveId = GetActorGuid();` 제거(프리뷰 갱신만 유지), 베이킹 주석 갱신 | 수정 |

### 구현·결정과 그 이유
- **PostLoad 안전망 제거**: 키 부여는 생성(`PostActorCreated`)·복제(`PostDuplicate`) 두 경로만 덮으면 충분하다. 부여값은 에셋 저장 시 직렬화되고 쿠커가 그대로 읽으므로, "재저장 없이 보장"하려던 PostLoad 는 잉여였다. 기존 배치 액터는 이미 값이 구워져 있어 회귀 없음.
- **Spawner PostLoad 는 존치**: 그 훅은 키 외에 Transient 에디터 프리뷰 컴포넌트 복원도 담당하므로, 키 베이킹 라인만 떼고 프리뷰 책임은 남겼다.
- **공통 부모 미도입**: UE 인터페이스가 상태를 못 들고 UHT 가 매크로로 UPROPERTY/override 주입을 막아, 필드·`GetWxSaveId`·잔여 두 훅의 클래스별 반복은 베이스/컴포넌트 없이는 제거 불가하다. 구조 도입 대신 명시적 반복을 택했다.

### 계획 대비 달라진 점
- `WxSpawner.h` 는 `PostLoad` 선언을 유지하고 정리할 주석도 없어 무수정. (계획상 "수정" 후보였으나 실제 변경 불필요)

### 후속 과제
- 신규 savable 액터는 `PostActorCreated`+`PostDuplicate` 두 훅 + `WxSaveId` 필드 + `GetWxSaveId` 를 구현해야 한다(사용처별 반복 잔존).
- **[보류·나중 재검토]** SaveId 사용처별 지정 없이 자동 처리하는 방향 논의함. 엔진엔 런타임 안정 GUID 없음(`ActorGuid`/`GetActorGuid` 모두 에디터 전용). 선택지 — A) 공유 베이스 `AWxSavableActor`(GUID 식별성 유지·자식 코드 0, 베이스 강제하나 현 구조선 비용 0), B) 서브시스템이 액터 이름/경로 키 자동 도출(베이스·필드·훅 전부 제거하나 WP 셀 유니크성·런타임 이름 정합성·PIE 프리픽스 검증 필요, 실패 시 조용한 상태 오염). 결론은 A 추천이었으나 사용자가 보류 결정.
