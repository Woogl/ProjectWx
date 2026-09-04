# WxCore — Public 헤더가 노출하는 UOL 의존성을 Public으로 승격

## 계획

### 목표
`WxLocatorUtils.h`는 Public 헤더인데 두 함수 시그니처가 `FUniversalObjectLocator`를 받는 반면, `UniversalObjectLocator` 모듈은 `PrivateDependencyModuleNames`에 있어 include 경로가 소비 모듈로 전파되지 않는다. 지금은 소비 모듈 4개가 모두 스스로 같은 의존성을 선언한 덕에 우연히 컴파일될 뿐이라, 새 소비 모듈이 이 헤더만 include하면 불완전 타입 에러를 만난다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` | `bBuildEditor` 블록의 `PrivateDependencyModuleNames` → `PublicDependencyModuleNames` | 수정 |

### 접근 방식
- **UBT 규약 그대로**: Public 헤더의 API에 등장하는 타입의 모듈은 Public 의존성에 둔다. 리뷰가 제시한 대안(헤더 doc-comment로 "소비 모듈이 직접 선언하라"고 안내)은 규약 위반을 문서로 덮는 쪽이라 채택하지 않는다.
- **`bBuildEditor` 조건은 유지**: `FWxLocatorUtils`의 두 함수 모두 `#if WITH_EDITOR` 안에 있어 API 노출 자체가 에디터 전용이다. 조건 안에서 Public에 넣는 형태는 엔진 모듈들이 에디터 전용 의존성을 다루는 표준 형태와 같다.
- **소비 모듈의 기존 선언은 그대로 둔다**: WxQuest·WxUI는 런타임 Public 헤더의 `USTRUCT` 프로퍼티로, WxWorld는 Private 코드에서 UOL을 직접 쓴다. 에디터가 아닌 타겟에서도 필요한 의존성이라 에디터 전용으로 전파되는 WxCore의 것으로 대체할 수 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` | `bBuildEditor` 블록의 의존성 선언을 Private에서 Public으로 | 수정 |

### 구현·결정과 그 이유
- **Public 승격을 택한 이유**: Public 헤더가 시그니처로 노출하는 타입은 그 헤더를 include하는 쪽이 완전한 타입을 볼 수 있어야 한다. 지금까지 컴파일이 됐던 건 소비 모듈들이 저마다 같은 의존성을 따로 선언해 둔 우연이지, 헤더가 자기 요구를 스스로 만족시켜서가 아니었다. 의존성을 Public으로 올리면 그 요구가 헤더를 따라 전파되므로, 새 소비 모듈은 include만으로 성립한다.
- **에디터 조건을 그대로 둔 이유**: 노출되는 두 함수가 `WITH_EDITOR` 안에만 있어서 API 자체가 에디터 전용이다. 조건을 풀면 게임 타겟에 필요 없는 모듈이 딸려 들어간다.
- **소비 모듈을 손대지 않은 이유**: WxQuest·WxUI는 런타임 프로퍼티로, WxWorld는 Private 코드에서 로케이터를 직접 다룬다. 에디터 아닌 타겟에서도 필요한 의존성이라 이번 전파로 대체되지 않는다.

### 검증
WxEditor(Development) 빌드 성공. UBT가 빌드 파일 변경을 감지해 makefile을 무효화하고 WxCore를 재컴파일·재링크했다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- WxDialogue가 쓰지 않는 `UniversalObjectLocator`를 Public에 선언해 둔 건은 별도 항목이라 그대로 남아 있다.
