# 빙의 해제 시 Enhanced Input MappingContext 회수

## 계획

### 목표
`AWxPlayerCharacter` 가 밀어 넣은 `InputConfig->MappingContext` 를 걷는 곳이 저장소 어디에도 없다. MappingContext 는 폰이 아니라 LocalPlayer 에 붙는 상태라 폰이 사라져도 남아, 빙의 해제·폰 교체·폰 없는 Experience 전환 뒤에도 죽은 폰의 매핑이 키를 계속 소비한다. 추가와 대칭인 회수를 넣는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxPlayerCharacter.cpp` | `NotifyControllerChanged()` 에 회수 분기 추가 | 수정 |

### 접근 방식
- **회수 지점은 `NotifyControllerChanged()`**: 이미 오버라이드하고 있는 함수고, 엔진에서 이 통보가 오는 경로가 필요한 경우의 상위집합이다. 빙의 해제(`UnPossessed`), 폰 파괴(`Destroyed`·`EndPlay` 가 `DetachFromControllerPendingDestroy` 를 거쳐 같은 경로로 들어온다), 소유 클라이언트에서 컨트롤러가 null 로 복제되는 경우(`OnRep_Controller`)를 모두 덮는다. `DestroyPlayerInputComponent()` 오버라이드는 마지막 경로를 놓친다.
- **떠난 컨트롤러는 `PreviousController` 에서 읽는다**: 회수 시점엔 `Controller` 가 이미 null 이라 서브시스템을 되찾을 길이 그것뿐이다. 엔진 public 프로퍼티라 새 멤버를 들이지 않아도 된다. 다만 `Super::NotifyControllerChanged()` 가 본문 끝에서 이 값을 현재 컨트롤러로 갱신하므로 Super 보다 먼저 읽어야 한다.
- **가드는 엔진 헬퍼에 맡긴다**: `ULocalPlayer::GetSubsystemFromController<T>()` 가 컨트롤러 null·PlayerController 아님·LocalPlayer 없음(데디케이티드)을 모두 null 로 접어 준다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxPlayerCharacter.cpp` | `NotifyControllerChanged()` 앞머리에서 떠난 컨트롤러의 입력 서브시스템을 찾아 MappingContext 제거 | 수정 |

### 구현·결정과 그 이유
- **통보 하나로 모든 경로를 덮은 이유**: 회수 지점 후보가 둘이었는데, `DestroyPlayerInputComponent()` 는 빙의 해제에서만 불려 컨트롤러가 null 로 복제되는 클라이언트 경로를 놓친다. 반면 컨트롤러 변경 통보는 빙의 해제·폰 파괴·복제 갱신이 모두 흘러드는 합류점이라, 갈래마다 훅을 다는 대신 한 곳만 잡으면 된다. 폰 파괴도 엔진이 컨트롤러 분리를 거쳐 같은 해제 경로로 들여보내므로 별도 처리가 필요 없었다.
- **Super 보다 먼저 읽는 이유**: 회수 시점엔 현재 컨트롤러가 이미 비워져 있어 서브시스템을 되찾을 실마리가 직전 컨트롤러뿐인데, 부모 구현이 본문 끝에서 그 값을 현재 값으로 덮는다. 순서가 곧 정확성이라 주석으로 이유를 남겼다.
- **회수 대상을 기억하지 않은 이유**: 입력 구성은 기본값 전용 프로퍼티라 런타임에 바뀌지 않는다. 추가할 때와 같은 표현식을 다시 읽으면 항상 같은 에셋을 가리키므로, 무엇을 넣었는지 들고 다닐 멤버를 만들 근거가 없었다.
- **가드를 엔진 헬퍼에 맡긴 이유**: 컨트롤러가 없거나 플레이어 컨트롤러가 아니거나 LocalPlayer 가 없는 서버 상황을 엔진 헬퍼가 이미 전부 null 로 접어 준다. 같은 검사를 손으로 늘어놓으면 읽는 사람이 대조해야 할 것만 늘어난다.
- **추가 시점의 선청소를 넣지 않은 이유**: 회수가 제자리에 있으면 재빙의는 항상 깨끗한 상태에서 시작하고, 입력 컴포넌트가 없을 때만 설정이 도는 엔진 구조상 중복 추가 경로 자체가 없다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 런타임 확인은 남았다. PIE 에서 `showdebug enhancedinput` 으로 폰 파괴·전환 뒤 목록에서 컨텍스트가 빠지는지 눈으로 볼 수 있다.
- WxEditor(Development) 빌드 성공.
