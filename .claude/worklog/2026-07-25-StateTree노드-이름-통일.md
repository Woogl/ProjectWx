# StateTree 노드 표시 이름 통일 (접두 제거 + Category 묶음)

## 계획

### 목표

Wx StateTree 태스크의 이름이 피커(메타 `DisplayName`)와 트리 행(`GetDescription`) 두 곳에서 따로 정해져, 고를 때는 접두가 붙고 놓고 나면 떨어진다. 설명 오버라이드가 없는 노드만 트리에서도 접두가 남아 같은 상태 안에서 표기가 갈린다. 이름의 출처를 메타 하나로 정리한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.h`<br>`Plugins/WxInventory/.../Inventory/WxRewardStateTreeNodes.h`<br>`.../WxInventoryStateTreeNodes.h`<br>`Plugins/WxSave/.../WxStateTreeTask_SaveGame.h` | 태스크 15개의 `USTRUCT` 메타를 `DisplayName`(접두 제거) + `Category = "Wx"` 로 통일. 메타가 빠져 있던 SaveGame 도 같은 형태로 되살림 | 수정 |
| `Plugins/WxInventory/.../WxInventoryStateTreeNodes.h`<br>`.../Private/Inventory/WxInventoryStateTreeNodes.cpp` | 상수만 돌려주던 `RefillItemCharges` 설명 오버라이드 제거(메타 폴백과 동일 문자열) | 수정 |
| 소스 21개 파일·문서 6개 | 주석·문서가 인용하는 옛 표시 이름에서 접두 제거 | 수정 |

### 접근 방식

- **이름은 메타가 정하고, 설명은 인자를 덧붙일 때만**: 피커는 인스턴스가 없어 메타만 볼 수 있으므로 메타가 단일 출처가 되어야 두 화면의 이름이 일치한다. 설명 오버라이드는 인자 표시라는 고유한 일이 있을 때만 남긴다.
- **접두 대신 Category**: 접두 없이도 피커가 `Wx` 그룹으로 우리 노드를 모아준다. 엔진 기본 노드(`Move To` + `AI|Action`)와 같은 방식이다.
- **구조체 이름은 그대로**: 이미 전부 `FWxStateTreeTask_<이름>` 형태라 바꿀 것이 없고, 덕분에 ST 에셋의 구조체 경로 참조도 유지된다(CoreRedirect 불필요).

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.h` (12개)<br>`Plugins/WxInventory/.../Inventory/WxRewardStateTreeNodes.h`<br>`.../WxInventoryStateTreeNodes.h`<br>`Plugins/WxSave/.../WxStateTreeTask_SaveGame.h` | 태스크 15개의 메타를 접두 뺀 `DisplayName` + `Category = "Wx"` 로 통일. 메타가 빠져 있던 SaveGame 도 같은 형태로 되살림 | 수정 |
| `Plugins/WxInventory/.../WxInventoryStateTreeNodes.h`<br>`.../Private/Inventory/WxInventoryStateTreeNodes.cpp` | 상수만 돌려주던 RefillItemCharges 설명 오버라이드 제거, 왜 없는지 한 줄 주석으로 대체 | 수정 |
| 소스 21개 파일·문서 6개 (`WxGimmick.h`, `WxElevator.h`, `WxLaserCorridor.h`, `Docs/Programmer/Interaction_System.md`, `Plugins/WxWorld/README.md` 등) | 주석·문서가 인용하던 옛 표시 이름에서 접두 제거 | 수정 |

### 구현·결정과 그 이유

- **이름의 출처를 메타로 단일화**: 피커는 인스턴스가 없어 메타밖에 볼 수 없으므로, 메타가 기준이어야 고를 때와 놓은 뒤의 이름이 같아진다. 설명 오버라이드는 인자를 덧붙인다는 고유한 일이 있을 때만 남겼다.
- **접두 대신 Category**: 이름마다 `Wx` 를 달지 않아도 피커가 한 그룹으로 묶어준다. 엔진 기본 노드도 같은 방식이라 관례에서 벗어나지 않는다.
- **상수 설명 제거**: 접두를 떼고 나니 그 오버라이드가 돌려주던 문자열이 메타 폴백과 같아져, 같은 이름을 두 곳에서 관리할 이유가 없어졌다.
- **구조체 이름 무변경**: 이미 전부 `FWxStateTreeTask_<이름>` 형태였다. 이름을 그대로 두어 ST 에셋의 구조체 경로 참조가 유지되므로 CoreRedirect 도 필요 없다.
- **worklog 는 손대지 않음**: 그 시점의 이름으로 남아야 기록으로서 의미가 있다.

### 계획 대비 달라진 점

- 계획대로.

### 후속 과제

- 에디터 실측 미완: 피커의 `Wx` 그룹과 트리 표기는 다음 에디터 실행에서 확인 필요.
- WxSave 만 노드 정의가 `WxStateTreeTask_SaveGame.h` 단독 파일이라 다른 플러그인의 `Wx*StateTreeNodes.h` 묶음과 다르다. 노드가 하나뿐이라 미룸.
