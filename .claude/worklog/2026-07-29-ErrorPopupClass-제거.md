# ErrorPopupClass 제거

## 계획

### 목표
팝업 위젯 클래스를 확인용·에러용 둘로 나눠 둔 갈래를 없앤다. 팝업은 하나면 충분하고, 에러 경로는 확인 팝업과 동일한 서술자를 쓰면서 위젯 클래스만 다르게 잡을 뿐이었다. 프로젝트 설정의 에러 팝업 클래스는 미지정 상태라 호출해도 아무 것도 뜨지 않는 죽은 경로이기도 하다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` | `ErrorPopupClass` 프로퍼티 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` | `ShowError` 선언 제거, `PushPopup` 선언 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` | `ShowError` 정의 제거, `PushPopup` 본문을 `ShowConfirmation` 으로 합침 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` | BP 파사드 `ShowErrorPopup` 선언 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` | `ShowErrorPopup` 정의 제거 | 수정 |
| `Config/DefaultGame.ini` | `ErrorPopupClass` 항목 제거 | 수정 |
| `Plugins/WxUI/README.md` | 팝업 클래스 지정 설명에서 에러 팝업 언급 제거 | 수정 |

### 접근 방식
- **순수 제거**: BP 에셋에서 에러 팝업 파사드를 호출하는 곳이 없어 대체 경로를 남길 필요가 없다. 남은 확인 팝업 경로는 그대로 둔다.
- **공통 헬퍼 합치기**: 팝업 push 헬퍼는 확인·에러 두 갈래를 위해 뽑아 둔 것이라, 갈래가 하나가 되면 중간 단계로 남길 이유가 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/System/WxUIDeveloperSettings.h` | 에러 팝업 클래스 프로퍼티 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h` | 에러 팝업 표시 함수와 팝업 push 헬퍼 선언 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp` | 에러 팝업 표시 함수 제거, 헬퍼 본문을 확인 팝업 표시 함수로 합침 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h` | BP 파사드의 에러 팝업 함수 선언 제거 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp` | 같은 함수 정의 제거 | 수정 |
| `Config/DefaultGame.ini` | 에러 팝업 클래스 항목 제거 | 수정 |
| `Plugins/WxUI/README.md` | 팝업 클래스 지정 설명에서 에러 팝업 언급 제거 | 수정 |

### 구현·결정과 그 이유
- **대체 경로 없이 전부 삭제**: 에러 팝업은 확인 팝업과 같은 서술자(확인 버튼 하나)를 쓰고 위젯 클래스만 갈랐던 갈래다. 프로젝트 설정에서 클래스가 미지정이라 호출해도 아무 것도 뜨지 않았고, BP 에셋 중 이 파사드를 부르는 곳도 없어 이행 경로를 남길 대상이 없다.
- **push 헬퍼를 확인 팝업 쪽으로 합침**: 이 헬퍼는 두 갈래가 공유하려고 뽑아 둔 것이었다. 갈래가 하나가 된 이상 호출자가 하나뿐인 중간 단계로 남을 이유가 없어, 설정에서 클래스를 읽는 자리와 실제 push 를 한 함수로 모았다.
- **과거 worklog 는 손대지 않음**: 그때의 작업 기록이므로 지금 구조에 맞춰 고치지 않는다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음
