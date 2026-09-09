# Master·Minion 참조 관계 규칙 정립

## 계획

### 목표
"누가 누구의 소환물인가"를 알려주는 통로가 넷으로 흩어지고 "이 폰이 소환물인가" 판정이 세 곳에 서로 다른 기준으로 복제돼 있다. 방향별 단일 소스를 규칙으로 정하고, 복제된 판정을 하나로 합친다.

### 정하는 규칙
1. **소환물 → 주인은 `Instigator` 단일 소스.** "나를 만든 게 누구인가"는 사망 후에도 남는 영구 사실이다.
2. **주인 → 소환물은 서브시스템 로스터 단일 소스, 서버 전용.** "지금 살아서 명령을 받을 수 있는 소환물"이라는 뜻이다.
3. **`Owner`는 소환 관계에 쓰지 않는다.** 폰의 Owner는 빙의 시 Controller로 덮인다.
4. **주인은 Pawn이다.** 규칙 1이 `Instigator`(APawn)이므로 타입으로 못 박는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/Minion/WxMinionSubsystem.h` | `static APawn* GetMaster(const APawn&)` 추가, `IsMinion` 삭제, 주인 파라미터·로스터 키를 `APawn` 으로, 클래스 주석에 관계 규칙 | 수정 |
| `Plugins/WxCombat/.../Private/Minion/WxMinionSubsystem.cpp` | `GetMaster` 구현, `IsMinion` 호출 교체·정의 삭제, `Cast<APawn>(&Master)` 제거, Owner 를 비워 두는 이유 주석 | 수정 |
| `Plugins/WxCombat/.../Private/AnimNotify/WxAnimNotify_SpawnMinion.cpp` | 소유자를 `Cast<APawn>` 으로 받아 전달 | 수정 |
| `Plugins/WxCombat/.../Private/AnimNotify/WxAnimNotify_CommandMinion.cpp` | 동일 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | `GetMasterASC` 의 자체 판정을 `UWxMinionSubsystem::GetMaster` 호출로 교체 | 수정 |
| `Source/WxGame/Controller/WxAIController.{h,cpp}` | `ResolveMinionMaster` 삭제, 호출 2곳을 `UWxMinionSubsystem::GetMaster` 로 교체 | 수정 |

### 접근 방식
- **판정 하나로 통합**: 서브시스템의 정적 함수로 둔다. 월드 인스턴스가 필요 없고, 관계를 소유한 클래스에 규칙과 판정이 함께 남는다. `APawn::PreInitializeComponents` 가 빈 인스티게이터를 자기 자신으로 채우므로 `GetInstigator() != this` 가 곧 소환 여부다.
- **소환 자격 게이트 교체**: `IsMinion(Master)` 의 로스터 전체 스캔을 `GetMaster(Master) != nullptr` 포인터 비교로 바꾼다. 판정 시점이 "로스터 등재 중"에서 "소환된 적 있음"으로 넓어지지만, 2단 소환 구조를 쓰지 않기로 확인했으므로 무방하다.
- **주인 타입을 Pawn 으로**: 지금은 `AActor&` 를 받아 놓고 `Cast<APawn>(&Master)` 로 Instigator 를 채우므로, 주인이 폰이 아니면 블랙보드 `Master` 도 `MasterStateTag` 도 없이 로스터에만 남는 반쪽 상태가 조용히 생긴다.
- **손대지 않는 것**: `MasterStateTag` 발행 경로, 상한 정리 로직, 블랙보드 `Master` 키 세팅 시점, `WxBTTask_MirrorAbility`. 역방향 공개 접근자는 호출자가 없어 만들지 않는다.

### 조사로 확인한 사실
- `AWxSpawner` 는 Instigator 를 넘기지 않고 Owner 로만 자신을 넣는다(`WxSpawner.cpp:133`). 배치·스폰된 적이 소환물로 오인되지 않는다.
- 폰에 Instigator 를 넘기는 스폰 경로는 소환 하나뿐이다. 투사체는 사수를 담는 별개 용도다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/Minion/WxMinionSubsystem.h` | `static GetMaster` 추가, `IsMinion` 삭제, 주인 파라미터·로스터 키를 `APawn` 으로, 클래스 주석에 참조 규칙 4줄 | 수정 |
| `Plugins/WxCombat/.../Private/Minion/WxMinionSubsystem.cpp` | `GetMaster` 구현, 소환 자격 게이트를 그것으로 교체, `IsMinion` 정의 삭제, `Cast<APawn>(&Master)` 제거 | 수정 |
| `Plugins/WxCombat/.../Private/AnimNotify/WxAnimNotify_{Spawn,Command}Minion.cpp` | 소유자를 `Cast<APawn>` 으로 받는다 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | `GetMasterASC` 의 자체 판정을 `UWxMinionSubsystem::GetMaster` 로 교체 | 수정 |
| `Source/WxGame/Controller/WxAIController.{h,cpp}` | `ResolveMinionMaster` 삭제, 호출 2곳 교체 | 수정 |

### 구현·결정과 그 이유
- **판정은 서브시스템의 정적 함수 하나로**: 세 곳에 흩어져 있던 소환물 판정을 `UWxMinionSubsystem::GetMaster` 로 모았다. 관계를 소유한 클래스에 규칙 주석과 판정이 함께 남고, 월드 인스턴스가 없어도 부를 수 있어 캐릭터·컨트롤러가 그대로 쓴다.
- **소환 자격 게이트의 의미가 넓어졌다**: 로스터 스캔에서 Instigator 비교로 바뀌면서 "지금 로스터에 있는가"가 "소환된 적 있는가"가 됐다. 사망 후 로스터에서 내려간 소환물도 이제 영구히 소환자가 될 수 없다. 2단 소환 구조를 쓰지 않기로 확인해 받아들인 변화이고, 덤으로 O(n) 스캔이 포인터 비교가 됐다.
- **주인 타입을 Pawn 으로 좁혔다**: 기존 시그니처는 `AActor&` 를 받아 놓고 Instigator 만 `Cast<APawn>` 으로 채워, 폰이 아닌 주인이 오면 블랙보드 `Master` 도 `MasterStateTag` 도 없이 로스터에만 남는 반쪽 상태가 조용히 생겼다. 규칙 1을 타입으로 못 박아 그 조합 자체를 없앴다.
- **`HandleMasterEndPlay` 만 캐스트를 진다**: 델리게이트 시그니처가 `AActor*` 라 로스터 키 조회 지점에서 한 번 캐스트한다. 나머지 경로는 캐스트가 사라졌다.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- UE 5.8 `WxEditor Win64 Development` 빌드 성공(`Result: Succeeded`). 로그: `.claude/skills/build-doctor/logs/build_2026-09-09_184810.log`
- `IsMinion`·`ResolveMinionMaster` 잔여 참조 0건, `git diff --check` 통과.
- PIE 미실시.

### 후속 과제
- PIE 확인: 상한 초과분 교체, 소환물 사망 시 주인 슬롯 태그 반납, 주인 제거 시 소환물 동반 파괴, 분신이 주인의 소환 몽타주를 따라해도 증식하지 않음.
- 상한은 여전히 주인 로스터 전체에 걸린다(2026-09-08 워크로그의 후속 과제 그대로). 한 주인이 두 종류를 부리면 같은 클래스만 세도록 바꿔야 한다.
