# WX 코드 리뷰 수정 기록 — 2026-10-02

[전체 리뷰](C:/Wx/Docs/Programmer/ProjectCodeReview_2026-10-02.md)의 **R1~R6 및 C1을 모두 수정**했다. 추가 확인 항목이었던 대화 중단 후 퀘스트 복구도 실제 StateTree 구성을 조사해 수정했다. Unreal 자동화 검사 6개와 PowerShell 7·5.1의 스크립트 검사 각 23개가 통과했고, 임시 검증 코드를 제거한 최종 Editor Development 증분 빌드도 성공했다.

## 수정 내용

| 항목 | 적용한 변경 | 상태 |
|---|---|---|
| R1 부활 시 충전 복구 | 부활 성공 후 컨트롤러 인벤토리의 충전형 아이템을 최대 충전. 체크포인트 태스크와 공용 함수 사용 | 완료 |
| R2 미니언 속성 덮어쓰기 | 공용 적 AbilitySet의 속성 초기화를 제거하고, 미니언 전용 초기값 행을 분리 | 완료 |
| R3 슬로모션 소유권 | 월드 단위 요청 핸들 관리. 태스크·컷신이 각자 자기 요청만 해제 | 완료 |
| R4 signed byte 파싱 | Int8의 음수 값을 명시적으로 복원하고 FText None의 원시 바이트 `255` 처리 | 완료 |
| R5 리다이렉트 오판정 | 짧은 이름도 텍스트 검색. 대소문자 차이와 읽기 실패로 필요한 리다이렉트가 SAFE가 되는 경로 차단 | 완료 |
| R6 취소 이후 추가 타격 | 공격 구간 ID와 세대로 취소·교체를 구별하고 이전 Sweep 처리를 중단 | 완료 |
| C1 구조체 내부 생성자 | `FWxDamageStatics` 생성자 본문을 구조체 밖의 같은 cpp로 이동 | 완료 |
| 추가 확인: 대화 복구 | 중단된 퀘스트 대화를 살아 있는 폰과 빈 대화 세션이 준비되면 다시 시작 | 확인 후 수정 완료 |

### R1: 부활과 체크포인트의 충전 처리 통합

[RefillAllItemCharges](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:567)를 추가하고 [부활 성공 경로](C:/Wx/Source/WxGame/Player/WxRespawnLibrary.cpp:63)와 충전 StateTree 태스크에서 호출한다. 새 폰 생성·빙의 실패 경로에서는 충전하지 않는다. 기존 아이템 인스턴스와 수량을 유지하며 실제 충전 변화만 알린다. 알림 콜백에서 인벤토리를 수정할 수 있어 목록 복사본을 순회하고 이미 제거된 아이템은 건너뛴다.

### R2: 미니언의 전용 속성과 공용 어빌리티 분리

- [ABS_Shared_Enemy](C:/Wx/Content/Character/Template/Shared/ABS_Shared_Enemy.uasset)의 `AttributeInitRow`를 비웠다. 공용 어빌리티 구성은 유지했다.
- [DT_CharacterAttribute](C:/Wx/Content/DesignerTables/DT_CharacterAttribute.uasset)에 `SummonedMinion` 행을 추가했다. ATK 40, DEF 40, CritRate 5, CritDMG 50, MOV 400에 기존 정상 생존값인 HP/MaxHP 100, MaxGP 50을 합쳤다. 나머지 속성은 0이다.
- [ABS_Minion](C:/Wx/Content/Character/Minion/ABS_Minion.uasset)이 새 행을 사용하도록 변경했다. BP의 세트 적용 순서는 유지했다.

기존 `Minion` 행은 Doppelganger가 사용하므로 변경하지 않았다. 기존 테이블 행 전체가 수정 전과 같음을 비교했다. 공용 세트의 나머지 사용자인 Soldier와 Enemy Template은 자신의 전용 세트에서 `TemplateEnemy`를 적용하므로 기존 초기값이 유지된다. 갱신한 [캐릭터 목록](C:/Wx/Saved/AbilitySystemLists/character-list.md)에서 참조와 값을 확인할 수 있다.

에셋은 Unreal의 에셋 편집·저장 API로 변경했다. 바이너리를 직접 치환하지 않았다.

### R3: 요청별 슬로모션 수명 관리

[UWxTimeDilationSubsystem](C:/Wx/Source/WxGame/Combat/WxTimeDilationSubsystem.cpp:7)이 서버의 활성 요청을 관리한다. **가장 최근의 활성 요청이 우선**하며, 해당 요청이 끝나면 남은 요청 중 가장 최근 배율을 적용한다. 마지막 요청이 끝나면 첫 요청 직전의 배율로 돌아간다. 같은 배율이나 엔진에서 같은 값으로 제한된 요청도 별도의 핸들로 구별한다. 반환값과 해제 인자는 `Handle`, 호출부의 보관 변수는 `TimeDilationHandle`로 명명했다.

SlowTime 태스크와 스킬 컷신을 모두 이 경로로 연결했다. 종료한 요청이 나중에 되살아나지 않으며, 중복 해제는 무시한다. 배율 제한과 네트워크 복제는 기존처럼 엔진 WorldSettings가 담당한다.

### R4·R5: 운영 스크립트의 경계값과 참조 검사

[에셋 추출기](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:280)는 `128..255`를 음수 Int8로 복원하며, FText None을 읽을 때 범위 검사 예외가 발생하지 않는다.

[CheckRedirects](C:/Wx/BatchFiles/CheckRedirects.bat:438)는 짧은 OldName도 전체 경로에 포함된 문자열에서 찾고 대소문자를 무시한다. 검색 패턴이 없으면 REVIEW로 남기고 파일 읽기 실패는 검사를 중단시킨다. 짧은 이름의 부분 문자열 검색은 보수적으로 REVIEW를 늘릴 수 있다. 실제 프로젝트의 리다이렉트는 삭제하지 않았다.

### R6: 취소·교체된 공격의 충돌 결과 폐기

[무기 공격](C:/Wx/Source/WxGame/Weapons/WxWeaponBase.cpp:48)은 몽타주 구간마다 `FGuid`를 받고 해당 구간만 종료한다. 공격이 취소·교체되면 세대가 바뀌며, 타격 콜백에서 돌아온 Sweep 루프는 남은 결과를 버린다. 비활성 공격의 Overlap도 피해를 적용하지 않는다.

충돌 활성화 자체가 즉시 Overlap과 어빌리티 종료를 일으킬 수 있으므로, 몽타주 태스크의 해제 정보와 무기의 활성 구간을 충돌보다 먼저 등록했다. 이전 구간의 늦은 종료가 새 공격을 끄지 않는다. 피해 적용에는 행 핸들의 복사본을 전달해 콜백 중 공격 교체로 설정이 바뀌는 것을 막았다.

### 추가 확인: 중단된 퀘스트 대화 재개

퀘스트 StateTree 5개의 구성과 전이를 C++ 에디터 API로 읽었다. `ST_Quest_Main1`과 `ST_Quest_Main2`에는 중단된 대화의 회복 경로가 없음을 확인했다. 같은 상태에는 보상 태스크도 있어 상태 전체를 다시 진입하는 방식은 보상 중복 위험이 있다.

[PlayDialogue 태스크](C:/Wx/Source/WxGame/Dialogue/WxStateTreeTask_PlayDialogue.cpp:79)가 중단을 기록하고, 살아 있는 폰과 비어 있는 대화 세션을 기다려 시작 행부터 재개하도록 수정했다. 다른 대화가 진행 중이면 기다린다. 정상 완주할 때만 성공하며, 상태를 떠날 때는 종료 델리게이트를 해제한다. 상태 재진입 없이 대화만 재개하므로 같은 상태의 보상을 다시 실행하지 않는다.

[ST_Quest_Main1](C:/Wx/Content/Quest/ST_Quest_Main1.uasset)과 [ST_Quest_Main2](C:/Wx/Content/Quest/ST_Quest_Main2.uasset)를 새 태스크 정의로 재컴파일하고 저장했다. 조사 기록은 [CodeReviewQuestTrees.txt](C:/Wx/Saved/Logs/CodeReviewQuestTrees.txt)에 있다.

## 검증 결과

| 검증 | 결과 | 확인한 동작 |
|---|---|---|
| Unreal `InventoryRefill` | 통과 | 0·부분 충전 회복, 충전 알림, 수량 유지, 재리필 시 중복 알림 없음 |
| Unreal `MinionAttributes` | 통과 | 실제 Minion→Shared 세트 적용 후 ATK·치명타·HP·MaxHP·MaxGP |
| Unreal `TimeDilation` | 통과 | 같은/다른 배율, 양쪽 종료 순서, 중복 해제, 이전 배율 0.8 복구, 제한된 배율의 소유권 |
| Unreal `WeaponCancellation` | 통과 | 정상 다중 타격, 첫 피해 콜백에서 취소, 취소 직후 새 공격, 이전 구간 종료, 시작 순간 Overlap에서 취소 |
| Unreal `DialogueRecovery` | 통과 | 폰 부재 시 대기, 재빙의 후 재개, 다른 대화 종료까지 대기, 자기 대화 완주 후 성공 |
| Unreal `QuestInspection` | 통과 | 퀘스트 5개 구성 조사, 영향받는 2개 재컴파일·저장 |
| PowerShell 7 | 23개 통과 | Int8 경계값, Base/None FText, 짧은/전체 리다이렉트 참조 및 삭제 후보 검사 |
| Windows PowerShell 5.1 | 23개 통과 | 같은 원본 함수와 입력으로 호환성 확인 |
| 실제 에셋 목록 재생성 | 성공 | 수정된 추출기로 어빌리티·캐릭터·이펙트 목록 생성 |
| 최종 Editor Development 증분 빌드 | 성공 | 임시 테스트 연결과 의존성을 제거한 상태 |
| 변경 검사 | 통과 | `git diff --check`, 테스트 파일·스테이징 정리 |

Unreal 검사 결과는 **6개 성공, 오류 0개**다. 로그에는 임시 캐릭터의 스켈레탈 메시 부재, 테스트 월드 정리, 배율 경계 입력에 대한 경고가 남는다. 퀘스트 컴파일에는 기존 `Quest.Fail` 태그 경고 2개가 남지만 컴파일과 저장은 성공했다.

검증 근거:

- [Unreal 자동화 결과 JSON](C:/Wx/Saved/Logs/CodeReviewAutomation/index.json), [실행 로그](C:/Wx/Saved/Logs/CodeReviewAutomation.log)
- [PowerShell 7 결과](C:/Wx/Saved/Logs/CodeReviewScriptTests.log), [PowerShell 5.1 결과](C:/Wx/Saved/Logs/CodeReviewScriptTests_PS51.log)
- [에셋 변경 및 기존 행 보존 로그](C:/Wx/Saved/Logs/CodeReviewAssetFix.log): 에셋 저장·행 비교 이후 Python에 노출되지 않은 StateTree 속성 조회가 실패했다. 해당 조사는 C++ API로 완료했고 위 `QuestInspection` 검증으로 확인했다.

자동화는 헤드리스 Unreal 월드와 실제 게임 코드를 사용했다. 무기 취소는 실제 피해 GE의 콜백에서 취소·교체를 발생시켰다. 실제 레벨의 패리 몽타주 재생, 사망 화면부터 부활까지의 UI 조작, 컷신 영상, 멀티플레이 실행, 패키징은 수행하지 않았다. 이 범위의 수동 플레이 결과까지 확인했다고 간주하면 안 된다.

임시 C++·PowerShell·Python 검증 코드와 입력 데이터는 제거했다. `WxEditor.cpp`와 `WxEditor.Build.cs`의 임시 연결도 제거했으며, 로그와 수정 문서만 보존했다. 로그는 로컬 생성물이고 제출 대상에는 소스·에셋 수정과 이 리뷰 문서만 포함한다.

## 빌드 결과

- 상태: **성공** — `WxEditor / Win64 / Development`, UE 5.8.
- 로그: [build_2026-10-02_033945_951_22252.log](C:/Wx/Saved/Logs/BuildDoctor/build_2026-10-02_033945_951_22252.log).

## 원인 요약

수정된 게임 코드의 컴파일·링크 및 자동화 검증을 완료했다. 임시 검증 연결 제거와 요청 핸들 명칭 정리 후 마지막 증분 빌드에서 11개 작업이 성공했고 종료 코드는 0이다. 핸들 명칭 변경은 동작을 바꾸지 않았으며, 기존 자동화 검사를 다시 실행하지 않고 빌드와 참조 검색으로 확인했다. 전체 클린 빌드는 아니다.

## 근거 로그

```text
Result: Succeeded
Total execution time: 35.28 seconds
BUILD_DOCTOR_RESULT=success
BUILD_DOCTOR_EXIT_CODE=0
```

## 수정 방법

빌드 오류에 대한 추가 조치는 필요 없다. 위 수정은 현재 작업 트리에 반영되어 있다.

## 재실행 명령

빌드 실행기:

```powershell
& '.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:\Wx'
```

실행기가 출력한 실제 명령:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```
