# WxLevelSnapshot

레벨에 배치된 액터의 상태를 저장할 때마다 JSON 파일로 자동 기록합니다.

## 이 플러그인으로 할 수 있는 것

### 1. 서버에 레벨 정보 전달
서버 측에서 `.umap`을 로드하지 않고도 레벨 구성을 파싱해 월드 상태 초기화나 검증 용도로 활용할 수 있습니다.

### 2. 레벨 변경점을 diff로 읽기
`.umap`과 `__ExternalActors__/*.uasset`은 바이너리라 source control에서 `Binary files differ`만 뜹니다.  
이 플러그인을 사용하면 레벨별 JSON 파일을 리포지토리에 커밋할 수 있으므로, 에디터를 열지 않아도 "누가 언제 어떤 액터를 어디로 옮기거나 설정을 바꿨는지"를 추적할 수 있습니다.  
레벨에 배치된 액터, 위치, 내부 프로퍼티(옵션) 값까지 JSON에 담을 수 있습니다.

### 3. 레벨 일괄 검수
JSON이라 **전체 프로젝트 레벨을 스캔**할 수 있습니다.  
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
| 일반 레벨 | ✅ | umap 저장 시 모든 액터 덤프 |
| World Partition 레벨 (OFPA) | ✅ | ExternalActor 파일 하나씩 저장될 때마다 레벨 JSON의 해당 항목을 갱신 |
| 서브레벨 (Streaming) | ⚠️ | 각 서브레벨이 별도 JSON 파일로 관리됨 |

---

## 어떤 데이터가 추출되는가?

`bIncludeAllProperties=true` 시 액터 내부 프로퍼티를 모두 재귀적으로 덤프.  
`bIncludeAllProperties=false` (기본) 시 식별·위치 정보만 기록.

### 액터별 필드

| 항목 | 지원 |
|---|---|
| 클래스 경로 (`class`) | ✅ |
| 레벨 패키지 경로 (`level`) | ✅ 액터가 속한 레벨 |
| 액터 GUID (`guid`) | ✅ 있을 때 (WP 삭제 매칭에 사용) |
| 액터 Soft Path (`actorReference`) | ✅ DataTable의 `TSoftObjectPtr<AActor>`에 바로 import 가능 |
| Transform (`location`/`rotation`/`scale`) | ✅ 루트 컴포넌트가 있을 때 |
| 전체 프로퍼티 (`properties`) | ✅ `bIncludeAllProperties=true` 시 멤버 변수·컴포넌트·struct/배열/맵까지 재귀 |

### 그 외

| 항목 | 지원 |
|---|---|
| Transient 플래그가 있는 액터 | ❌ 자동 스킵 |
| `Transient` / `DuplicateTransient` / `Deprecated` / `EditorOnly` 속성 | ❌ 의도적으로 제외 |
| DataLayer 할당 | ❌ 현재는 미지원 |
| Landscape / Foliage 인스턴스 | ❌ 전용 포맷, 미지원 |

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
- WP 모드는 현재 로드된 액터만 저장 이벤트를 발생시킵니다. 모든 액터를 한 번에 스냅샷하려면 에디터에서 전부 로드 후 레벨 저장하거나, 각 액터를 수정/저장하세요.

---

## 설정

`Project Settings > Wx > WxLevelSnapshot`

| 항목 | 기본값 | 설명 |
|---|---|---|
| `bEnabled` | true | 전체 기능 on/off |
| `FileExtension` | `.json` | 스냅샷 파일 확장자. 점을 포함해 입력 |
| `OutputDirectory` | `Plugins/WxLevelSnapshot/Snapshots` | 스냅샷 저장 루트 폴더 |
| `bCombineAllLevels` | true | true면 모든 레벨을 단일 `AllLevels.json` 파일로 합쳐 저장. false면 레벨별 별도 파일 |
| `IncludeDirectories` | [] | 대상 레벨 폴더 (비어있으면 전체) |
| `ExcludeDirectories` | [] | 제외 레벨 폴더 |
| `bIncludeAllProperties` | false | 액터 내부 프로퍼티를 모두 재귀적으로 추출해 `properties` 필드에 기록 (컴포넌트·서브오브젝트·구조체·배열 포함). false면 class/fname/label/transform/attach만 기록 |
| `KeyProperties` | `{ AActor: ActorGuid }` | 클래스별 키 프로퍼티 매핑. 등록된 클래스(및 하위)만 스냅샷되며 해당 프로퍼티 값이 JSON 키가 됨. 특정 하위 클래스에 다른 프로퍼티를 지정하면 그 클래스의 인스턴스는 해당 값을 키로 사용. 매칭 규칙이 없거나 프로퍼티 추출에 실패한 액터는 스냅샷에서 제외 |

---

## 출력

- **경로**:
  - `bCombineAllLevels=true` (기본): `<OutputDirectory>/AllLevels<FileExtension>` (모든 레벨이 단일 파일)
  - `bCombineAllLevels=false`: `<OutputDirectory>/<레벨 패키지 경로><FileExtension>`
- **포맷**: UTF-8 (no BOM), 키 알파벳 정렬, 들여쓰기 2-space pretty print
- **ReadOnly 플래그**는 자동 해제 후 덮어쓰기 (Perforce 등에서 편의)
- **삭제 동기화**:
  - 레벨 에셋 삭제 시 대응 JSON 파일 제거
  - ExternalActor 삭제 시 레벨 JSON에서 해당 항목만 제거 (마지막 항목이면 파일까지 삭제)

### 최상위 JSON 구조

최상위 오브젝트는 액터 키 → 액터 스냅샷의 flat map입니다.

```
{
  "<ActorKey1>": { ... },
  "<ActorKey2>": { ... }
}
```

### 액터 항목 필드

| 필드 | 타입 | 조건 |
|---|---|---|
| `class` | string | 항상 |
| `level` | string | 액터가 속한 레벨 패키지 경로 |
| `guid` | string | `AActor::GetActorGuid()`가 valid일 때 (WP 삭제 이벤트 매칭용) |
| `actorReference` | string | 항상. `Actor->GetPathName()` — DataTable의 `TSoftObjectPtr<AActor>` 컬럼에 import 가능 |
| `transform.location` | object `{x,y,z}` | 루트 컴포넌트가 있을 때 |
| `transform.rotation` | object `{pitch,yaw,roll}` | 루트 컴포넌트가 있을 때 |
| `transform.scale` | object `{x,y,z}` | 루트 컴포넌트가 있을 때 |
| `properties` | object | `bIncludeAllProperties=true`이고 덤프 결과가 비어있지 않을 때. 액터의 모든 edit/visible 프로퍼티를 재귀적으로 포함 (컴포넌트는 `{class, properties}` 블록으로 중첩) |

---

## 요구사항

- Unreal Engine 5.7
- 에디터 빌드 (`WxEditor.Target.cs`)
