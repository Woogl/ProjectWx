# CoreRedirects 정리 — 남은 옛 경로 에셋 재저장 후 리다이렉트 전부 제거

## 계획

### 목표
`DefaultEngine.ini`에 쌓인 리다이렉트 7개를 모두 없앤다. 옛 경로로 저장된 에셋이 남아 있으면 리다이렉트를 지우는 순간 그 참조가 조용히 끊기므로, 먼저 해당 에셋을 새 경로로 다시 저장한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | `ActiveGameNameRedirects` 2줄, `[CoreRedirects]` 클래스 리다이렉트 5줄 제거 | 수정 |
| `BP_HGTest`, `WBP_Nameplate_Player`, `BP_Sandbag`, LV_DevCombat의 BP_Soldier 외부 액터 | 리다이렉트가 켜진 에디터에서 다시 저장해 임포트를 새 경로로 갱신 | 수정 |

### 접근 방식
- **판정은 원본 임포트 테이블로**: 엔진의 PkgInfo는 로드 과정에서 리다이렉트를 적용한 결과를 보여 줘 쓸 수 없다. 파일의 이름 테이블과 임포트 테이블(항목당 40바이트)을 직접 읽어, 각 클래스 임포트가 어느 스크립트 패키지를 가리키는지 확인한다.
- **이미 필요 없는 것**: `TP_Blank`는 어디에도 참조가 없다. `WxAnimNotify_UseItem`은 쓰는 에셋이 이미 새 경로를 가리킨다. `WxBTService_TargetDistance`는 옛 이름이 노드 객체 이름으로만 남아 있고 클래스는 새 이름이다.
- **재저장이 필요한 것**: 위 에셋 4개는 모듈을 옮긴 뒤 저장된 적이 없어 옛 경로를 쥐고 있다. 켜져 있는 에디터에서 MCP로 저장하면 로드 시 적용된 리다이렉트 결과가 파일에 기록된다. 저장 후 원본 임포트를 다시 읽어 확인한 다음 리다이렉트를 지운다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Config/DefaultEngine.ini` | 리다이렉트 7줄과 빈 `[CoreRedirects]` 섹션 제거 | 수정 |
| `Content/Character/HGTest/BP_HGTest.uasset` | 재저장, 아이템 사용 컴포넌트 임포트가 WxInventory로 | 수정 |
| `Content/Character/Sandbag/BP_Sandbag.uasset` | 재저장, AI 행동 컴포넌트 임포트가 WxAI로 | 수정 |
| `Content/UI/Widget/WBP_Nameplate_Player.uasset` | 재저장, 뷰모델 리졸버 임포트가 WxUI로 | 수정 |
| `Content/__ExternalActors__/Maps/LV_DevCombat/2/RU/NRCPT86OI84K9H87YKQM54.uasset` | LV_DevCombat의 BP_Soldier 인스턴스 재저장, AI 행동 컴포넌트 임포트가 WxAI로 | 수정 |

### 구현·결정과 그 이유
- **수정됨 상태는 같은 값 재설정으로 만든다**: MCP 저장 도구는 수정되지 않은 패키지를 건너뛴다. 컴파일로는 수정됨이 되지 않았고, 이미 가진 값을 그대로 다시 넣으면 파일에 흔적 없이 수정됨이 된다.
- **외부 액터는 레벨 저장으로**: MCP 저장 도구들이 외부 액터 패키지를 에셋으로 찾지 못한다. 에디터에서 맵을 열고 액터를 수정됨으로 만든 뒤, 사용자가 현재 레벨 저장으로 저장했다. 작업 뒤 에디터 레벨은 원래의 LV_FrontEnd로 되돌렸다.
- **제거 판정은 전수 재검사로**: 저장 후 옛 이름이 들어 있는 에셋 15개의 원본 임포트를 다시 읽어, 옛 경로를 가리키는 곳이 0건인 것을 확인하고 지웠다. `TP_Blank` 참조도 0건이다.

### 계획 대비 달라진 점
- WBP_Nameplate_Player를 수정됨으로 만들려고 처음에 메타데이터 태그를 붙였는데, MCP의 태그 제거가 인자 변환 버그(`list[str] | None`을 UStruct로 못 바꿈)로 동작하지 않았다. 레벨 저장 때 이 WBP도 함께 저장되어 표식 `WxResaveMarker=1`이 파일에 남았다. HEAD로 되돌리려 했으나 에디터가 파일을 잡고 있어 실패했다.

### 후속 과제
- 없음. 표식은 사용자가 에디터 콘솔에서 `py`로 `remove_metadata_tag` 후 저장해 지웠다. 파일에 표식이 없고 임포트가 새 경로인 것을 확인했다.
