# Wx Grant Reward — 직접 지급을 무조건 로컬 플레이어에게

## 계획

### 목표
`DirectGrantTarget`을 ST 에디터에서 바인딩할 수 없어, 비-픽업 보상 직접 지급을 무조건 로컬 플레이어(0번 컨트롤러)에게 하도록 단순화한다. 적 사망 경로가 이미 쓰는 `GetPlayerController(0)` 패턴과 통일된다. 바인딩 필드와, 더 이상 쓰이지 않는 상자 `OpeningActor`를 함께 제거한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData `DirectGrantTarget` 제거(빈 구조체), doc 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `DropRewards(GetPlayerController(Owner,0))`, 인스턴스 read 제거, include·주석 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxTreasureChest.h` | `OpeningActor` 멤버·doc 제거 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxTreasureChest.cpp` | `HandleInteracted`에서 `OpeningActor` 대입 제거 | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | 직접 지급 대상을 "로컬 플레이어(0번 컨트롤러)"로 통일 | 수정 |

### 접근 방식
- **태스크 자체 해결**: 권위 가드 후 `UGameplayStatics::GetPlayerController(Owner, 0)`를 `DropRewards`에 전달. 픽업은 월드 스폰, 재화만 이 플레이어 인벤토리로(분기는 기존 `DropRewards`).
- **인스턴스 데이터 빈 구조체**: StateTree 규약상 타입은 유지하되 필드 0.
- **OpeningActor 제거**: 죽은 코드. `HandleInteracted`는 권위에서 `SetChestState(Open)`만 하는 원형 복귀.
- **멀티 주의**: `GetPlayerController(0)`=로컬/호스트. 스탠드얼론 중심·적 사망 경로와 동일 선택이라 일관.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData `DirectGrantTarget`·미사용 `class AActor;` 전방선언 제거(빈 구조체), 클래스 doc을 "로컬 플레이어 인벤토리 직접 지급"으로 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `DropRewards(UGameplayStatics::GetPlayerController(Owner,0))`, 인스턴스 read 제거, `GameplayStatics.h`·`PlayerController.h` include 추가 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxTreasureChest.h` | `OpeningActor` 멤버·doc 제거 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxTreasureChest.cpp` | `HandleInteracted`에서 `OpeningActor` 대입 제거(권위 시 `SetChestState(Open)`만) | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | 직접 지급 대상을 "로컬 플레이어(0번 컨트롤러)"로 통일(경로②·트리거연동·mermaid·참조표) | 수정 |

### 구현·결정과 그 이유
- **`GetPlayerController(Owner, 0)` 채택**: 적 사망 경로(`WxEnemyCharacter.cpp`)가 이미 쓰는 동일 패턴. 스탠드얼론 중심이라 0번=로컬 플레이어가 항상 정답이고, 두 트리거(적 사망·상호작용)의 직접 지급 대상이 통일됐다.
- **OpeningActor 통째 제거**: 바인딩이 불가능해진 이상 상자가 연 폰을 보관할 이유가 사라져 죽은 코드. 멤버·doc·대입 모두 제거하고 `HandleInteracted`를 원형(권위 시 State만 확정)으로 되돌렸다.
- **인스턴스 데이터 빈 구조체 유지**: StateTree 태스크 규약상 `FInstanceDataType`/`GetInstanceDataType`은 있어야 해 빈 USTRUCT로 둠. `EnterState`의 인스턴스 read 제거.
- **include 추가**: `APlayerController*`→`AActor*`(DropRewards 인자) 업캐스트에 완전 타입이 필요해 `PlayerController.h`, `GetPlayerController`에 `GameplayStatics.h` 추가(IWYU). 빌드로 확인.

### 계획 대비 달라진 점
- 헤더에서 미사용이 된 `class AActor;` 전방선언도 함께 제거(계획 외 정리).

### 후속 과제
- **콘텐츠 배선(사용자)**: ST_TreasureChest Open 상태에 `Wx Grant Reward`를 **배치만** 하면 된다(바인딩 칸 전혀 없음). PIE에서 픽업 월드 드랍/재화 플레이어 인벤토리 적재/이중 지급 없음/복원 시 재지급 없음 확인.
