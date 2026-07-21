# FWxAbilitySet_GameplayAbility 제거 → TSubclassOf 직접 사용

## 계획

### 목표
필드가 `TSubclassOf<UWxAbilityBase> Ability` 하나뿐인 얇은 래퍼 구조체 `FWxAbilitySet_GameplayAbility`를 없애고, `UWxAbilitySet::GrantedAbilities`를 `TArray<TSubclassOf<UWxAbilityBase>>`로 직접 쓴다. 입력 키가 어빌리티 CDO로 이관된 뒤 이 구조체는 존재 이유가 사라졌다. 배열 원소 타입이 struct→object로 바뀌면 언리얼이 기존 데이터를 버리므로, 6개 `ABS_` 에셋을 안전하게 마이그레이션한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbilitySet.h` | `FWxAbilitySet_GameplayAbility` 구조체 삭제, `GrantedAbilities` 타입을 `TArray<TSubclassOf<UWxAbilityBase>>`로 변경 | 삭제·수정 |
| `WxAbilitySet.cpp` | `GiveToAbilitySystem` 어빌리티 부여 루프를 새 타입으로 교체 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 구조체 언급 제거, `GrantedAbilities`가 직접 클래스 배열임을 반영 | 수정 |
| `Content/Character/**/ABS_*.uasset` (6개) | `GrantedAbilities` 데이터 마이그레이션(dump→reauthor) | 수정(데이터) |
| `Wx.uproject` | `PythonScriptPlugin` 임시 활성화 후 원복(최종 무변경) | 임시 |

### 접근 방식
- **dump → 코드 변경 → reauthor (Python 헤드리스 커맨드릿)**: 코드 변경 전 각 에셋의 `granted_abilities`(순서·클래스 참조)를 리플렉션으로 JSON에 뜬다. 이 JSON이 파괴적 변경 이전의 검증 체크포인트다. 코드는 1회로 깔끔히 바꾸고(구조체 완전 제거), 변경·빌드 후 동일 리스트를 새 타입 필드에 다시 써 넣고 저장한다.
- **왜 자동 이관이 안 되나**: 배열 inner 타입이 struct→object라 이름이 같아도 태그드-프로퍼티 로더가 기존 배열을 폐기한다. CoreRedirect도 struct↔object 원소 타입 변경은 못 잇는다.
- **보존 근거**: `AttributeInitRow`·`GrantedEffects`는 타입 불변이라 reauthor 로드/저장 과정에서 그대로 유지된다. `GrantedAbilities`만 명시 재기입 필요.
- **대안 기각**: C++ 2단계(`_DEPRECATED`+CoreRedirect+`PostLoad`+`ResavePackages`)는 에디터 실행 1회로 줄지만 임시 코드 잔재·오라우팅 창·강제저장 플래그 불확실성이 있어 제외.

---

## 완료

> **상태(2026-07-21): 완료.** dump→코드변경→reauthor 전 과정을 라이브 에디터 + unreal-mcp 구조화 툴셋(`ObjectTools`/`AssetTools`)으로 수행. 6개 `ABS_` 에셋의 `GrantedAbilities`가 새 타입으로 재기입·저장됨(git `M` 6건). 헤드리스 커맨드릿·raw Python 은 쓰지 않아 `dump_abs.py`/`reauthor_abs.py` 는 미사용(백업 기록만). `Wx.uproject` 는 사용자 결정으로 **원복(최종 무변경)**.

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbilitySet.h` | `FWxAbilitySet_GameplayAbility` 구조체 삭제, `GrantedAbilities` → `TArray<TSubclassOf<UWxAbilityBase>>` | 삭제·수정 |
| `WxAbilitySet.cpp` | 어빌리티 부여 루프를 `TSubclassOf` 직접 순회로 교체 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 참조표에서 구조체 제거, `GrantedAbilities`가 클래스 배열임을 반영 | 수정 |
| `Content/Character/**/ABS_*.uasset` (6) | `GrantedAbilities` 데이터 마이그레이션(reauthor) | 수정(데이터) |
| `Saved/WxTools/abs_granted.json` | dump 결과(검증 체크포인트·백업) | 신규 |
| `Wx.uproject` | (임시 활성화했던 PythonScriptPlugin 원복 → 무변경) | 원복 |

### 구현·결정과 그 이유
- **실행 경로**: 헤드리스 커맨드릿(모듈 로드 실패)·raw Python 대신 **MCP 구조화 툴셋**을 썼다. `ObjectTools.get_properties`로 dump, `set_properties`로 reauthor, `AssetTools.save_assets`로 저장 — PythonScriptPlugin의 커맨드릿 경로가 불필요해졌다.
- **파괴적 변경 확인**: 코드 교체 후 새 에디터로 재시작해 `ABS_Player`를 읽으니 `GrantedAbilities`가 `None` 16개(배열 길이만 보존, 원소는 null)로 로드됐다. struct→object 원소 타입 변경 시 자동 이관 불가라는 전제를 실증 — dump JSON 없었으면 목록 소실.
- **재시작 필요**: 필드 타입(USTRUCT/UPROPERTY 레이아웃) 변경은 Live Coding 불가라 빌드 후 에디터 재시작으로 새 리플렉션을 로드해야 reauthor 가능. DebugGame 에디터 종료→Development 에디터 실행.
- **저장 함정(read-only)**: 첫 `save_assets`가 false. 로그상 `Cannot remove ...uasset as it is read only` — `ABS_HR_Player`·`ABS_Sandbag` 두 .uasset이 디스크에서 읽기 전용이라 덮어쓰기 실패. 읽기 전용 속성 해제 후 재저장하니 6개 모두 저장. 나머지 4개는 이미 쓰기 가능.
- **PythonScriptPlugin 원복**: `EditorToolset/Content/Python/init_unreal.py`가 PythonScriptPlugin이 켜져야 실행되며 `editor_toolset.*`(ObjectTools/AssetTools/DataAssetTools/Material 등) 툴셋을 등록한다. 즉 이 플러그인을 끄면 다음 실행부터 그 Python MCP 툴셋이 사라진다(C++ 툴셋 GAS/Niagara/UMG 등은 유지). 사용자가 계획대로 원복을 택함.

### 계획 대비 달라진 점
- dump/reauthor를 계획의 python 스크립트가 아닌 MCP 구조화 툴셋으로 수행(스크립트는 미사용 백업으로만 잔존).
- 계획에 없던 두 가지가 드러남: (1) 재기입한 2개 .uasset의 read-only로 인한 저장 실패, (2) PythonScriptPlugin이 python MCP 툴셋 등록을 좌우한다는 사실(원복 판단에 반영).

### 후속 과제
- 없음(마이그레이션 종료). 6개 .uasset + 코드/문서 변경은 다음 커밋에 포함.
