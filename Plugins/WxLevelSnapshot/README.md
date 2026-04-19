# WxLevelSnapshot

레벨에 배치된 액터의 상태를 저장할 때마다 JSON 파일로 자동 기록합니다.

## 이 플러그인으로 할 수 있는 것

### 1. 서버에 레벨 정보 전달
서버가 직접 읽어 캐릭터 플레이어 시작 위치 확인, 스폰 검증 등에 활용할 수 있습니다.   
`.umap`은 언리얼 에디터/런타임이 있어야 열 수 있지만, JSON은 어디서든 파싱할 수 있기 때문입니다.

### 2. 레벨 변경점 diff
World Partition 레벨의 경우 각 액터가 `__ExternalActors__/*.uasset` 바이너리 파일로 저장되기 때문에, 변경 내용을 사람이 읽을 수 없습니다.  
이 플러그인은 액터별 스냅샷을 JSON으로 남기므로, diff만 보고도 "누가 언제 어떤 액터를 어디로 옮겼는지, 어떤 프로퍼티를 바꿨는지"를 추적할 수 있습니다.  

### 3. 레벨 일괄 검수
전체 프로젝트 레벨의 액터를 스캔할 수 있습니다.  
예: "특정 BP 클래스 인스턴스를 가진 레벨 찾기", "스케일이 비정상인 액터 찾기".

---

## Quick Start

1. 이 플러그인 전체를 프로젝트의 Plugins 폴더에 추가.
2. `Project Settings > Wx > WxLevelSnapshot`에서 대상/제외 폴더를 선택.
3. 레벨 또는 ExternalActor를 저장 (Ctrl+S).  
4. `Plugins/WxLevelSnapshot/Snapshots/<레벨 패키지 경로>.json` 파일이 생성/업데이트됨.

---

## 어떤 레벨이 대상인가?

| 레벨 종류 | 지원 | 비고 |
|---|---|---|
| 일반 레벨 (Monolithic) | ✅ | umap 저장 시 레벨 전체를 기록 |
| World Partition 레벨 | ✅ | ExternalActor 파일 단위로 해당 항목만 upsert. umap 저장 시에는 로드된 액터만 upsert하고 언로드 영역 엔트리는 보존 |
| 서브레벨 | ⚠️ | 각 서브레벨 단위로 관리됨 |

---

## 어떤 데이터가 추출되는가?

최상위 오브젝트는 `Project Settings > Wx > WxLevelSnapshot`의 `KeyProperties`에서 설정합니다.

> ⚠️ 같은 키 값을 가지면 덮어써질 수 있습니다.

| 필드 | 타입 | 설명 |
|---|---|---|
| `class` | string | 액터의 클래스 경로 |
| `level` | string | 액터가 속한 레벨 패키지 경로 |
| `actorReference` | string | `Actor->GetPathName()`. DataTable의 `TSoftObjectPtr<AActor>` 컬럼에 import 가능 |
| `transform.location` | object `{x,y,z}` | 월드 위치 |
| `transform.rotation` | object `{pitch,yaw,roll}` | 월드 회전 |
| `transform.scale` | object `{x,y,z}` | 월드 스케일 |
| `guid` | string | `AActor::GetActorGuid()` 값. valid일 때만 포함 (WP 삭제 이벤트 매칭용) |
| `properties` | object | `bIncludeAllProperties=true`일 때만 포함. 액터의 모든 edit/visible 프로퍼티를 재귀적으로 담음 |

### 추출에서 제외되는 데이터

| 항목 | 비고 |
|---|---|
| Transient 플래그가 있는 액터 | 자동 스킵 |
| `Transient` / `DuplicateTransient` / `Deprecated` / `EditorOnly` 속성 | 의도적으로 제외 |
| DataLayer 할당 | 현재는 미지원 |
| Landscape / Foliage 인스턴스 | 전용 포맷, 미지원 |

---

## 무엇을 위한 도구로 쓰면 안되는가?

- **레벨 백업/복원 도구가 아닙니다.**
  - JSON에서 `.umap`이나 `__ExternalActors__/*.uasset`을 재생성하지 않습니다.
  - 백업은 Source Control로 하세요.
- **런타임 도구가 아닙니다.**
  - 에디터 전용입니다.
  - 패키징된 게임에 포함되지 않습니다.

---

## 트러블슈팅

**스냅샷이 안 생겨요**
- 레벨을 `Ctrl+S`로 실제 저장했는지 확인 (Autosave는 무시됨).
- `Project Settings > Wx > WxLevelSnapshot`에서 `bEnabled`가 켜져있는지.
- `IncludeDirectories`에 값이 있다면 레벨 경로가 포함되는지.

**WP 레벨에서 일부 액터만 JSON에 있어요**
- WP 레벨에서는 현재 로드된 액터만 저장 이벤트를 발생시킵니다.
- 모든 액터를 한 번에 스냅샷하려면 에디터에서 전부 로드 후 레벨 저장하거나, 각 액터를 수정/저장하세요.

---

## 프로젝트 세팅

`Project Settings > Wx > WxLevelSnapshot`

| 항목 | 기본값 | 설명 |
|---|---|---|
| `bEnabled` | true | 전체 기능 on/off |
| `FileExtension` | `.json` | 스냅샷 파일 확장자. 점을 포함해 입력 |
| `OutputDirectory` | `Plugins/WxLevelSnapshot/Snapshots` | 스냅샷 저장 루트 폴더 |
| `bSaveFilePerLevel` | false | false면 모든 레벨을 단일 `AllLevels.json` 파일로 합쳐 저장. true면 레벨별 별도 파일로 저장 |
| `IncludeDirectories` | [] | 대상 레벨 폴더 (비어있으면 전체) |
| `ExcludeDirectories` | [] | 제외 레벨 폴더 |
| `bIncludeAllProperties` | false | true면 액터 내부 프로퍼티를 모두 재귀적으로 추출해 `properties` 필드에 기록. false면 식별·위치 정보(`class`/`level`/`actorReference`/`guid`/`transform`)만 기록 |
| `KeyProperties` | `{ AActor: ActorGuid }` | 클래스별 키 프로퍼티 매핑. 등록된 클래스(및 하위)만 스냅샷되며 해당 프로퍼티 값이 JSON 키가 됨. 등록되지 않은 클래스나 프로퍼티 추출에 실패한 액터는 스냅샷에서 제외 |

---

## JSON 포맷

- UTF-8 (no BOM)
- 키 알파벳 정렬
- 들여쓰기 2-space pretty print

---

## 파일 동기화

- ReadOnly 플래그는 자동 해제 후 덮어쓰기 (Perforce 등에서 편의)
- 레벨 에셋 삭제 시 대응 JSON 파일 제거
- ExternalActor 삭제 시 레벨 JSON에서 해당 항목만 제거 (Guid로 매칭. 마지막 항목이면 파일까지 삭제)

---

## 요구사항

- Unreal Engine 5.7
- 에디터 빌드 (`WxEditor.Target.cs`)
