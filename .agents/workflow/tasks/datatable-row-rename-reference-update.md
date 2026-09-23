# DataTable 행 이름 변경 시 사용처 참조 갱신

상태: 구현 완료 · 인간 코드 리뷰·에디터 확인 대기 · 2026-09-23

## 확정 설계 · 2026-09-23 사용자 확정

- 요청: DataTable에서 RowName을 바꾸면 그 행을 쓰던 에셋에도 새 RowName이 반영되기를 원했다.
- 조사: 엔진에는 이 기능이 없다. UE 5.8 `FDataTableEditorUtils::RenameRow`는 테이블 키만 바꾸고(`DataTableEditorUtils.cpp:588`), 행 이름 리다이렉트 장치도 없다.
- 근거: `FDataTableRowHandle::PostSerialize`는 저장할 때 (테이블, 행 이름)을 SearchableName으로 남긴다(`DataTable.cpp:1115`). 엔진의 "Find Row References"도 이 기록을 쓴다.
- 실측: 패키지 헤더의 SearchableNames 절을 파싱해, 프로젝트의 모든 사용처 유형에 기록이 있음을 확인했다.
  - StateTree 태스크 InstancedStruct: `ST_Quest_Main1` → `DT_Reward:Gold1000`
  - BP CDO: `BP_Template` → `DT_Reward:Gold100`
  - 레벨 외부 액터: `LV_DevCombat` 외부 액터 → `DT_Reward:Gold100`
  - 몽타주 노티파이: `AM_Template_Attack_L` → `DT_Damage`
  - GE 컴포넌트: `GE_Shared_GuardReduction` → `DT_Effect`
  - DataAsset: `ABS_Template` → `DT_CharacterAttribute`
- 실측: BP 그래프 핀 리터럴(GetDataTableRow 노드, 핸들 핀 기본값)은 기록되지 않는다. 프로젝트 에셋을 바이너리 검색한 결과 이런 사용처는 0건이다.
- 판단(사용자): 자동으로 갱신하되 에셋은 Dirty 상태로만 두고, 저장은 사용자가 직접 한다.
- 판단(사용자): 열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다. 엔진 에셋 이름 변경도 `FEditorFileUtils::IsMapPackageAsset`(외부 액터 포함)이면 로드하지 않는다.
- 판단(사용자): 대화·자막 `NextRow`(같은 테이블 행을 가리키는 FName)는 제외한다. 갱신 대상은 `FDataTableRowHandle` 참조뿐이다.
- 판단(사용자): 위험할 수 있으므로 별도 에디터 플러그인으로 분리한다. 켜고 끄는 것은 에디터 개인 설정(Editor Preferences)에서 한다.
- 판단(사용자): 플러그인 이름은 `DataTableRowFixup`으로 한다. 프로젝트 범위를 넘는 범용 플러그인이라 `Wx`를 뺀다.
- 판단(AI): 같은 이유로 플러그인 안의 클래스·모듈·로그 이름에서도 `Wx`를 뺀다. AGENTS.md 코딩 규칙 1(`Wx` 접두사)의 예외다. 저작권 머리말은 유지한다.
- 판단(AI): 기본값은 꺼짐으로 둔다. 사용자가 위험성을 이유로 켜고 끌 수 있기를 요청했기 때문이다.
- 판단(사용자, 위 판단 변경): 기본값을 켜짐으로 바꾼다. 꺼져 있으면 이름 변경 시 참조가 조용히 끊기는 편이 더 위험하다는 검토 뒤의 결정이다. 설정 카테고리는 `Wx`로 한다(Editor Preferences > Wx).
- 설계:
  - 감지: `FDataTableEditorUtils::INotifyOnDataTableChanged` 리스너를 쓴다. RowList 변경 전 행 맵을 보관하고, 변경 후 두 조건을 모두 만족하면 이름 변경으로 판정한다.
    - 행 수가 같고 사라진 이름이 하나다.
    - 새 이름이 옛 이름과 같은 행 메모리를 가리킨다. `RenameRow`는 행 메모리를 그대로 새 키로 옮긴다.
    - 재임포트·시트 전체 붙여넣기는 행을 새로 만들므로 제외한다. 이때는 경고 로그를 남긴다.
  - 대상: Asset Registry `GetReferencers(FAssetIdentifier(테이블, 옛 이름), SearchableName)`와 Dirty 패키지를 합친다. 레지스트리가 디스크 기준이라 저장 전 변경은 Dirty 패키지에서 찾는다. 로드되지 않은 패키지는 로드하되 레벨·외부 액터는 로드하지 않는다.
  - 갱신: 프로퍼티를 끝까지 따라가며(구조체·배열·셋·맵·InstancedStruct) 핸들을 찾는다. 값은 `PropertyAccessUtil::SetPropertyValue_Object`로 설정한다. 파이썬 `set_editor_property`와 같은 경로라 Modify, 변경 통지, 값을 물려받은 로드된 인스턴스로의 전파를 엔진이 처리한다. DataTable 행 안의 핸들은 RowData 변경 알림 사이에서 직접 고친다.
  - StateTree: 수정한 StateTree는 `UStateTreeEditingSubsystem::CompileStateTree`로 다시 컴파일한다. 에디터 데이터만 바꾸면 실행에 쓰는 컴파일 결과에 옛 값이 남기 때문이다.
  - 되돌리기: `RenameRow`의 트랜잭션 안에서 실행되므로 Ctrl+Z 한 번에 이름 변경과 참조 갱신이 함께 되돌아간다.
  - 보고: 결과는 알림 1개와 출력 로그(`LogDataTableRowFixup`)로 보고한다. 로그에는 갱신한 프로퍼티, 건너뛴 레벨, 편집할 수 없는 프로퍼티, 기록은 있으나 고치지 못한 패키지를 남긴다.

## 구현 · 2026-09-23

- 기준은 HEAD `0b6a82877`이다. 작업 트리에 다른 세션의 미커밋 변경이 함께 있어, 이 작업의 파일과 `tasks/index.md`의 한 줄만 골라 커밋했다.
- 신규 플러그인 `Plugins/DataTableRowFixup`(Editor 전용, PostEngineInit, .uplugin 카테고리 Editor)을 추가했다. 처음에는 `WxDataTableRowRename`으로 만들었다가 이름을 바꿨다.
  - `DataTableRowFixupModule`: 로그 카테고리를 정의하고, 리스너의 수명을 관리한다.
  - `DataTableRowFixupSettings`: `UDeveloperSettings`를 상속하고 `Config = EditorPerProjectUserSettings`이다. 이 Config면 엔진 기본 `GetContainerName()`이 이미 `"Editor"`를 돌려주므로(`DeveloperSettings.cpp:28`) 오버라이드하지 않는다. 카테고리는 `Wx`이다. 설정 항목은 `bUpdateReferencesOnRowRename`(기본 true)이다.
  - `DataTableRowReferenceUpdater`: 감지와 갱신을 맡는다.
- `Wx.uproject`에 플러그인 활성화 항목을 추가했다. WxEditor는 변경하지 않았다.

## 검증 · 2026-09-23

- build-doctor(WxEditor Win64 Development): 성공. 기본값·카테고리 변경 후 최종 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_133144_700_45400.log`이다.
  - 첫 빌드는 `FStateTreeCompilerLog` 소멸자 링크 오류로 실패했다. `PropertyBindingUtils` 의존성을 추가해 해결했다.
  - 중간 실패 2회는 다른 세션의 작업 때문이었다. 퀘스트 VM 이동 중 헤더 중복과, `WxEffectComponent_Hit.h`를 수정하는 도중의 상태였다. 이번 변경과는 무관하다.
- 임시 자동화 테스트 `Wx.DataTableRowRename.Verify`를 헤드리스 에디터(`-nullrhi`)에서 실행했다. 결과는 Success이고 ensure·오류 로그는 없었다. 테스트 파일과 빌드 중간 산출물은 삭제했다. 이 검증은 이름을 바꾸기 전(`WxDataTableRowRename`)에 했다. 이름 변경은 식별자만 바꿨고 동작은 그대로다. 에셋은 저장하지 않았고, 설정도 ini에 기록되지 않았다.
  - `DT_Reward` `Gold100` 이름 변경: 레지스트리 경로로 `BP_Template`·`BP_Soldier` CDO를 로드해 갱신했고 둘 다 Dirty가 됐다. `LV_DevCombat` 외부 액터는 건너뛰고 경고했다.
  - 저장 전에 한 번 더 이름 변경: Dirty 패키지 경로로 두 CDO를 다시 갱신했다.
  - `UndoTransaction` 1회: 행 이름과 CDO 값이 함께 직전 상태로 돌아왔다.
  - `Gold1000` 이름 변경: `ST_Quest_Main1`·`ST_Quest_Main2`의 에디터 데이터(`Tasks` InstancedStruct)를 갱신했다. 옛 이름은 남지 않았고, 재컴파일한 기본 인스턴스 데이터도 새 이름을 가리켰다.
  - `DT_Damage` `AM_Template_Attack_L` 이름 변경: 몽타주 노티파이 스테이트 `WxAnimNotifyState_WeaponAttack_0.DamageDataRow`를 갱신했다.
  - 설정이 꺼져 있을 때: 이름을 바꿔도 CDO는 변하지 않았고 로그도 없었다.
- 확인하지 못한 것(인간 확인 필요):
  - 실제 에디터 UI에서의 동작: 목록 인라인 이름 변경·행 편집기 이름 변경, 알림 표시, 진행 창
  - Editor Preferences > Wx > DataTable Row Fixup에 설정이 켜진 상태로 표시되는지
  - 열린 레벨의 배치 액터 오버라이드 갱신과, 값을 물려받은 로드된 인스턴스로의 전파

## 남은 제약

- 엔진에 이름 변경 이벤트가 없어 `RenameRow`가 행 메모리를 그대로 옮기는 동작에 의존한다. 엔진이 복사 방식으로 바뀌면 갱신은 멈추고 경고 로그만 남는다(오탐은 없다).
- 열려 있지 않은 레벨에서 값을 직접 지정한 배치 액터는 갱신하지 않는다. 레벨을 열어 직접 고쳐야 한다. BP 기본값을 그대로 쓰는 배치 액터는 저장 데이터에 값이 없어 영향이 없다.
- 레지스트리는 디스크 기준이다. 저장한 직후 1~2초, 즉 디렉터리 감시가 레지스트리를 다시 스캔하기 전에 같은 행 이름을 또 바꾸면 방금 저장한 에셋을 놓칠 수 있다.
- BP 그래프 핀 리터럴은 SearchableName 기록이 없어 갱신하지 못한다. 현재 프로젝트에는 이런 사용처가 없다.
- 로드된 인스턴스로의 전파는 트랜잭션에 기록되지 않는다. 엔진 디테일 패널·파이썬과 같은 동작이다.
- 이름을 바꾸는 순간 참조 에셋을 동기로 로드한다. 몽타주라면 스켈레톤·메시까지 함께 로드된다.
- 새 모듈이라 Visual Studio 솔루션에 보이게 하려면 프로젝트 파일을 다시 생성해야 한다(generate-project-files). 빌드에는 영향이 없다.
- 완료 단계에서 할 일: Wiki `editor-tools.md` 모듈 표에 DataTableRowFixup을 추가하고, 감지·갱신 방식과 제약을 반영한다.
