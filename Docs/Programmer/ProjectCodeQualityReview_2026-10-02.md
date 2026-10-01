# WX 전체 코드 리뷰 품질 개선 제안

작성일: 2026-10-02 KST. 근거 코드: `7fe9a0bc01236299061fa05fd4e7b5f1507d630a`.

앞서 완료한 **529개 파일, 40,834줄의 전체 코드 리뷰**에서 후속으로 검토할 코드 품질 개선안을 정리했다. 게임플레이뿐 아니라 UI, WxEditor, 자체 플러그인 3개, 설정과 빌드·운영 스크립트까지 원래 리뷰 범위를 기준으로 삼았다. 아래 후보의 구현과 위치는 수정 사항이 제출된 현재 코드에서 다시 확인했다.

**개선 제안은 10개다.** 사용자 결정에 따라 데이터 구성 오류 검증은 보류하고, 현재 검토할 개선은 도구의 결과 전달과 코드 구조 정리에 집중한다. 어빌리티의 비동기 처리, 인벤토리 변경 알림, UI 표시 계약을 정리할 가치가 있다. 함수 길이나 파일 수만으로 분리를 권하지 않았고, 기존 동작을 보존하면서 변경 이유가 분명한 작업을 골랐다.

이 문서의 우선순위는 유지보수상 권장 순서다. 기존 버그 보고서의 P0~P3 심각도와 다르며, 모든 제안이 현재 재현된 결함이라는 뜻도 아니다. **문서 작성 이후 Q5의 검사 실행부 분리, Q9의 비용 조회 책임 분리, Q10의 반복 주석 정리를 반영했으며, 나머지 제안은 아직 구현하지 않았다.**

## 현재 진행 방침

**2026-10-02 사용자 결정: 데이터 구성에 대한 새로운 오류 검증은 아직 추가하지 않는다.** Q1의 AbilitySet 조합 검증과 Q2의 StateTree 저작 검증은 전부 보류한다. Q4의 새로운 입력 제약과 Q9의 미지원 비용 구성 진단도 현재 범위에서 제외한다. 기존 코드의 검증과 동작은 유지한다.

Q3의 기존 컴파일 결과 전달, Q6의 바이너리 읽기·파싱 진단, 그리고 미반영 코드 구조 정리는 별도 개선 후보로 남긴다. 이때 게임 데이터의 조합·필수값·허용 규칙을 새로 검사하는 기능으로 확대하지 않는다. 보류한 항목은 아래에 참고용으로 보존하며, 반영한 Q5·Q9의 구조 정리와 Q10을 제외한 나머지 항목도 아직 구현 전 제안이다.

**2026-10-02 사용자 결정: ViewModel은 프레젠테이션 로직, Ability는 모델로서 게임 로직을 담당한다.** Q9에서는 비용 GE 해석을 Ability로 옮기고, ViewModel은 반환된 값의 표시와 자원 변경 구독을 담당하도록 반영했다.

함수는 이름과 호출 흐름이 쉽고 직관적이어야 한다. 기존 명칭과 반환 방식으로 책임을 분리할 수 있으면 이를 유지하고, 코드 파악과 유지보수에 필요한 만큼만 구조를 추가한다.

## 전체 리뷰와의 관계

- 원래 범위와 결함 근거: [전체 코드 리뷰](C:/Wx/Docs/Programmer/ProjectCodeReview_2026-10-02.md)
- 529개 파일별 기록: [검토 목록](C:/Wx/Docs/Programmer/ProjectCodeReview_Coverage_2026-10-02.md)
- 이미 반영한 R1~R6·C1과 대화 복구: [수정 및 검증 기록](C:/Wx/Docs/Programmer/ProjectCodeReview_Fixes_2026-10-02.md)

원래 529개는 수정 전 검토 목록의 수치다. 이후 추가된 TimeDilationSubsystem도 현재 구현 맥락에 포함하되, 원래 목록의 해시와 수치를 새 상태로 덮어쓰지 않는다. 이번 문서는 전체 리뷰에 대한 품질 후속 제안이며 별도의 전수 리뷰 완료 목록은 아니다.

| 전체 리뷰 영역 | 이번 문서에서의 정리 |
|---|---|
| GAS·전투·몽타주·캐릭터 구성 | Q1 구성 검증은 보류, Q7 방향 동기화와 재생 수명 분리 |
| AI·입력·타기팅·무기·미니언 | 동기 콜백과 종료 순서 보존을 리팩터링 조건에 포함. 기존 수정은 완료 기록으로 연결 |
| 장치·퀘스트·대화·스포너·보상 | Q2 StateTree 저작 검증은 보류, Q8 알림 계약, Q10 반복 주석 정리 |
| 인벤토리·플레이어·저장·부활 | Q8 변경 결과와 알림의 일관성. 부활 리필은 이미 완료 |
| UI·MVVM·프런트엔드 | Q9 비용 표시 계약. 기존 이벤트 기반 갱신과 이미지 비동기 로딩 유지 |
| WxEditor | Q1의 프로젝트 전용 검증 추가는 보류. 기존 디테일·썸네일·비주얼라이저 등록 구조 유지 |
| WxToolset | Q4 에셋 편집의 실패 보장, Q3 결과 전달 방식 참고 |
| DataTableRowFixup | Q3 참조 수정 결과와 재컴파일 결과 통합 |
| BoxComponentVisualizer | 현재 역할이 좁고 등록·해제가 대응하므로 별도 구조 분리 제안 없음 |
| BatchFiles·.agents | Q5 실행부와 검사 로직 분리, Q6 파싱 진단 개선 |
| 모듈·타겟·프로젝트·Config | 현재 기능 폴더와 모듈 경계를 유지. 품질 정리만을 위한 GameFeature·Experience 도입이나 모듈 증설은 제안하지 않음 |

## 개선 후보 요약

규모는 변경 범위에 대한 상대 평가다. 일정 추정은 아니다.

| 항목 | 상태 | 우선순위 | 규모 | 제안 | 기대 효과 |
|---|---|---|---|---|---|
| Q1 | 보류 | 미정 | 보통 | 캐릭터의 AbilitySet 조합 검증 | 개별 에셋 검사로 찾지 못하는 구성 충돌 예방 |
| Q2 | 보류 | 미정 | 보통 | StateTree 필수 입력을 컴파일 단계에서 검증 | 잘못된 조립을 플레이 전에 발견 |
| Q3 | 후보 | 높음 | 작음 | 행 참조 수정과 기존 StateTree 컴파일 결과를 함께 보고 | 일부 단계 실패를 정확히 진단 |
| Q4 | 구조 정리만 후보 | 보통 | 큼 | 일괄 에셋 편집의 기존 검사·계획·적용 단계 분리 | 실패 시 에셋 상태와 복구 범위를 명확히 함 |
| Q5 | 완료 | 보통 | 보통 | 리다이렉트 검사기의 PowerShell 실행부 분리 | 검사 로직 재사용과 비대화형 검증 개선 |
| Q6 | 후보 | 높음 | 보통 | 바이너리 파서의 부분 해석 결과와 진단 분리 | 목록의 정보 손실과 미지원 형식을 구별 |
| Q7 | 후보 | 보통 | 큼 | 어빌리티의 방향 동기화와 몽타주 재생 수명 분리 | 기본 어빌리티 수정 시 영향 범위 축소 |
| Q8 | 후보 | 보통 | 보통 | 인벤토리 변경 결과를 통한 알림 발행 통일 | 서버·클라이언트 알림 순서의 유지보수 개선 |
| Q9 | 구조 정리 완료·구성 검증 보류 | 보통 | 보통 | 기존 어빌리티 비용 표시 계약과 조회 책임 정리 | UI와 게임플레이 비용 해석의 결합 완화 |
| Q10 | 완료 | 낮음 | 작음 | 코딩 규칙을 재서술한 반복 주석 정리 | 기능 설명의 가독성 향상 |

## Q1 캐릭터 단위 AbilitySet 조합 검증

**상태: 보류. 아래 제안과 완료 기준은 추후 재검토용이다.**

**관찰.** [UWxAbilitySet::IsDataValid](C:/Wx/Source/WxGame/AbilitySystem/WxAbilitySet.cpp:90)는 속성 행의 존재, 세트 내부 어빌리티 중복, 입력 조건과 쿨다운 충돌을 이미 검사한다. 반면 [GiveAbilitySets](C:/Wx/Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp:71)는 캐릭터에 지정한 여러 세트를 순서대로 적용한다. 개별 세트가 유효해도 세트 사이의 속성 초기화·입력·쿨다운 관계는 별도 문제다. R2 수정은 현재 미니언 구성을 바로잡았지만, 같은 유형의 조립 실수를 예방하는 검사까지 추가한 것은 아니다.

**제안.** WxEditor의 프로젝트 전용 검증에서 캐릭터 CDO의 AbilitySets를 적용 순서대로 펼쳐 검사한다. 여러 속성 초기화 행이 있으면 어떤 세트가 어떤 속성을 덮는지 경고하고, 세트 경계를 넘는 입력 조건·쿨다운 설정도 비교한다. 기존 AbilitySet의 비교 규칙은 작은 함수로 재사용해 두 검증 경로가 달라지지 않게 한다.

의도적인 공통 기본값→전용 값 적용은 허용하고, 적용 순서와 덮는 속성을 보여 주는 방식이 적절하다. HP 0 같은 값을 모든 캐릭터에 일괄 금지하면 분신 등 역할별 구성을 잘못 막을 수 있으므로 역할별 규칙은 근거가 있을 때 추가한다.

**완료 기준.** 세트 하나만 볼 때는 정상인 충돌을 캐릭터 구성 검사에서 찾는다. 의도적인 초기화 순서는 유지되고, 기존 정상 에셋의 실행 결과가 달라지지 않는다.

## Q2 StateTree 저작 오류의 조기 검증

**상태: 보류. 아래 제안과 완료 기준은 추후 재검토용이다.**

**관찰.** [WaitMoveToTarget](C:/Wx/Source/WxGame/Quest/WxStateTreeTask_WaitMoveToTarget.cpp:18)은 빈 목표를 진입 시 경고하고 Running으로 대기한다. [PlayDialogue](C:/Wx/Source/WxGame/Dialogue/WxStateTreeTask_PlayDialogue.cpp:30)는 시작 행과 세션을 실행 시 확인한다. 한편 스포너 태스크에는 이미 [Compile 훅](C:/Wx/Source/WxGame/Spawner/WxStateTreeTask_TriggerSpawners.cpp:57)과 [공용 검증 함수](C:/Wx/Source/WxGame/Spawner/WxSpawnerLocatorUtils.cpp:17)가 있다.

**제안.** 기존 Compile 검증 방식으로 필수 목표의 누락, 시작 대화 행의 부재, 잘못된 컴포넌트 지정처럼 저작 시 판별 가능한 조건을 검사한다. 노드마다 일반 검증 프레임워크를 새로 만들기보다 행 핸들·로케이터 등 실제 중복이 있는 검사만 공유한다.

런타임 바인딩으로 채우는 입력은 바인딩 정보를 함께 확인해야 한다. World Partition에서 언로드된 액터는 이름이나 로케이터가 잘못된 경우와 구별한다. 보상 행처럼 비워 두는 것이 명시적으로 허용된 입력도 유지한다. 실행 시 폰·컴포넌트의 수명 검사는 계속 필요하다.

**완료 기준.** 비어 있는 필수 입력은 컴파일 결과에 노드 위치와 함께 나타난다. 동적 바인딩, 언로드 대상, 선택 입력에 대한 오탐이 없다.

## Q3 행 참조 갱신 결과에 재컴파일 결과 포함

**관찰.** [DataTableRowReferenceUpdater::UpdateReferences](C:/Wx/Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.cpp:92)는 참조 수집, 패키지 로드, 값 교체, StateTree 재컴파일, 알림을 한 흐름에서 처리한다. 재컴파일 루프는 `FStateTreeCompilerLog`를 만들지만 반환 성공 여부와 진단을 최종 `ProblemCount`에 반영하지 않는다. [WxStateTreeToolset의 컴파일 함수](C:/Wx/Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp:577)는 성공 여부와 메시지를 반환하는 참고 사례다.

**제안.** 기존 플러그인 안에서 단계별 결과를 기록한다. 최소한 변경 패키지, 읽지 못한 참조, 변경 실패 프로퍼티, 컴파일 실패 StateTree를 구분하고 최종 알림에 합산한다. 컴파일 실패 시 해당 에셋과 컴파일러 메시지를 Output Log에 남긴다. 성공한 참조 수정과 컴파일 실패를 동시에 표현할 수 있어야 한다.

컴파일 실패를 숨기지 않도록 하는 진단 개선이며, 현재 정상 퀘스트의 컴파일 실패를 새로 재현했다는 주장은 아니다. 이 작업만으로 자동 저장·전체 롤백 정책을 바꾸지는 않는다.

**완료 기준.** 참조 수정은 성공했지만 컴파일은 실패하는 임시 입력에서 최종 결과가 부분 실패로 표시되고, 문제가 있는 StateTree를 즉시 찾을 수 있다.

## Q4 일괄 에셋 편집의 실패 보장 명확화

**관찰.** [AppendMontage](C:/Wx/Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.cpp:219)는 사전 검사를 충실히 수행하지만, `Modify()` 뒤 트랙·길이를 변경하고 섹션을 추가하는 도중에도 실패를 반환하는 경로가 있다. `Modify()` 호출만으로 해당 함수의 실패 시 원상 복구가 보장되는 것은 아니다. 반면 [SetReferenceParameterValues](C:/Wx/Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.cpp:544)는 복사본을 수정한 뒤 성공할 때 반영한다.

**현재 제안 범위.** 몽타주 변경을 기존 입력 검사→변경 계획 작성→적용으로 나눈다. 계획에 섹션 매핑, 오프셋, 보존할 링크·노티파이를 담는다. 새로운 데이터 구성 제약의 추가는 보류한다. 적용 도중 실패할 수 있는 단계에는 보존한 값으로 복구하는 경로 또는 명확한 부분 변경 결과를 둔다. UObject 복제와 링크까지 포함하므로 얕은 복사로 에셋 전체를 되돌릴 수 있다고 가정하지 않는다.

이 항목은 실패 계약과 복구 가능성을 강화하는 제안이다. 현재 에셋에서 해당 섹션 추가 실패를 재현한 것은 아니다.

**완료 기준.** 실패 전후 대상 에셋을 비교해 약속한 범위가 보존된다. 정상 편집에서는 섹션 링크, 시작·끝 노티파이, GUID와 소유 객체가 기존 결과와 같다. 저장 여부는 기존 API 계약을 유지한다.

## Q5 리다이렉트 검사 실행부와 로직 분리

**관찰 당시.** `CheckRedirects.bat`은 배치 파일 안의 PowerShell 본문을 문자열로 잘라 실행했다. 같은 파일에 패키지 해석, 참조 검사, 보고, 재저장·삭제, `Read-Host`가 들어 있었다. [ExportAbilitySystemLists.bat](C:/Wx/BatchFiles/ExportAbilitySystemLists.bat:5)은 이미 얇은 실행기와 `.agents/scripts/`의 실제 로직을 분리한 구조였다.

**반영 완료 — 2026-10-02.** 기존 함수 24개를 내용 변경 없이 [Check-Redirects.ps1](C:/Wx/.agents/scripts/Check-Redirects.ps1)로 옮겼다. [배치 실행기](C:/Wx/BatchFiles/CheckRedirects.bat:15)는 스크립트를 `-Interactive`로 호출하고, 기존처럼 결과를 보여 준 뒤 종료 코드 0·1·2를 전달한다.

- [Invoke-RedirectCheck](C:/Wx/.agents/scripts/Check-Redirects.ps1:694)는 검사·보고와 결과 코드를 담당한다. 스크립트 직접 실행의 기본값은 읽기 전용이다.
- [Invoke-RedirectCleanup](C:/Wx/.agents/scripts/Check-Redirects.ps1:668)은 대화형 재저장·삭제를 담당한다. 각 단계는 기존처럼 명시적인 `y` 또는 `yes` 입력을 기다리고, 재저장 실패 시 삭제를 차단한다.
- [직접 실행 분기](C:/Wx/.agents/scripts/Check-Redirects.ps1:717)로 함수만 dot-source할 때는 검사·입력 대기·파일 변경·호출자 종료가 발생하지 않는다. 패키지 파서 공통화와 판정 규칙 변경은 이번 범위에 포함하지 않았다.

읽기 전용 실행:

```powershell
& '.agents/scripts/Check-Redirects.ps1' -ProjectRoot 'C:\Wx'
```

종료 코드는 기존처럼 `0`(리다이렉트 없음 또는 모두 제거 가능), `1`(재저장·수동 검토 필요), `2`(처리 오류)다. 프로젝트 경로를 생략하면 스크립트 위치에서 루트를 찾으며, 대화형 정리가 필요하면 `-Interactive`를 지정한다.

**검증.** Windows PowerShell 5.1과 PowerShell 7에서 각각 **19개 검사**를 통과했다. 구문과 함수 로딩, 기본 루트·한글·공백 경로, 읽기 전용 실행 전후 파일 해시·수정 시각 보존, 짧은 이름의 대소문자 무시 참조, SAFE·RESAVE·REVIEW·오류 결과, 구·신 배치의 종료 코드 0·1·2, 빈 응답 시 보존·승인 시 임시 INI 정리·재저장 실패 시 삭제 차단을 확인했다. 재저장 실패 검사는 함수 대체를 사용했으며 실제 Unreal 커맨드릿은 실행하지 않았다.

- [PowerShell 5.1 검증 로그](C:/Wx/Saved/Logs/Q5RedirectSplit_PS51.log), [PowerShell 7 검증 로그](C:/Wx/Saved/Logs/Q5RedirectSplit_PS7.log)
- 실제 프로젝트도 두 버전에서 읽기 전용으로 실행했고 모두 `No redirects found.`, 종료 코드 0이었다. 현재 리다이렉트가 없어 실제 에셋 참조 순회는 수행하지 않았다. [5.1 실행 로그](C:/Wx/Saved/Logs/Q5RedirectAudit_PS51.log), [7 실행 로그](C:/Wx/Saved/Logs/Q5RedirectAudit_PS7.log)

임시 검증 코드와 입력 데이터는 제거하고 로그만 보존했다. Q5는 스크립트 변경이므로 C++ 빌드는 다시 실행하지 않았다.

## Q6 에셋 파서의 부분 해석 진단 개선

**관찰.** [Read-Value](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:269)는 지원하지 않는 네이티브 구조체를 형식명으로 표시하며, 추정한 태그 해석의 예외도 같은 대체 표현으로 돌려준다. [Read-Array](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:334)는 해석 실패·미지원 원소·크기 불일치를 모두 `(항목 수 items)`로 축약한다. 의도적으로 내부를 생략한 값과 예상한 형식을 읽지 못한 값의 이유가 출력에서 구별되지 않는다. 패키지 전체 실패는 상위에서 파일명을 붙여 처리하므로 그 경로와 구분해야 한다.

**제안.** 표시값과 파싱 진단을 분리한다. `Parsed`, `Unsupported`, `Malformed`와 같은 상태, 파일·프로퍼티·바이트 위치·이유를 수집하고 목록 끝에 부분 해석 건수와 관련 경로를 남긴다. 정상적인 미지원 형식은 설명과 함께 요약하고, 읽어야 하는 필드의 크기 불일치는 명확한 실패로 처리한다.

[Read-Package](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:50)의 버전별 오프셋 계산과 문자열·인덱스 읽기에 공통 경계 검사를 두면 형식 변경 시 원인 파악도 쉬워진다. 기존 cooked 패키지 거부, 태그 범위의 끝 위치 검사 등 이미 있는 방어는 유지한다. 에디터를 매번 켜는 방식으로 전환하기보다 현재 추출 방식의 지원 범위를 명확히 하는 것이 우선이다.

**완료 기준.** 정상 파일의 목록은 유지된다. 미지원 구조체, 잘린 문자열, 범위를 벗어난 이름 인덱스가 서로 다른 원인으로 기록된다. 부분 해석된 결과를 완전한 추출로 오해하지 않는다.

## Q7 방향 동기화와 몽타주 재생 수명 분리

**관찰.** [UWxAbilityBase::PlayMontage](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:358)는 방향 섹션 선택뿐 아니라 클라이언트 입력 전송, 서버 TargetData 대기, 기존 요청 정리, 실제 재생 연결을 담당한다. 관련 상태는 [수신 콜백](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:475), [대기 정리](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:502), [EndAbility](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:330)에도 걸쳐 있다. 각 경로에 필요한 방어가 이미 있지만 한 흐름을 고칠 때 읽어야 할 범위가 크다.

**제안.** 먼저 대기 중 몽타주·섹션·델리게이트·입력 준비 상태를 전용 요청 상태로 묶고 시작·완료·취소 함수를 대응시킨다. 방향 송수신을 독립적으로 재사용할 필요가 확인되면 WxGame 안의 전용 AbilityTask로 옮긴다. `UWxAbilityBase`의 외부 재생 API는 유지해 파생 어빌리티 변경을 줄인다.

TargetData가 등록 직후 동기적으로 도착하는 경우, 기다리는 동안 어빌리티가 끝나는 경우, 재생 실패가 즉시 종료 콜백을 발생시키는 경우를 먼저 상태표로 정리한다. 현재의 몽타주 이벤트 구독 순서와 공격 구간 핸들 수명도 보존한다.

**완료 기준.** 로컬·원격 방향 재생, 데이터 선도착·후도착, 대기 중 취소·요청 교체에서 이전 요청이 새 실행에 영향을 주지 않는다. 이 변경은 빌드만으로 완료 판단하지 않고 해당 실행 경로를 검증해야 한다.

## Q8 인벤토리 변경 알림 계약 통일

**관찰.** [복제 콜백](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:43)과 [서버 아이템 추가](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:256)에서 슬롯 변경·총수량 변경 알림을 각각 같은 순서로 발행한다. [ConsumeByDefinition](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:174)은 이미 변경 결과를 모아 반환하는 구조를 쓴다. [알림 함수](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:580)도 공용화되어 있으므로 남은 관심사는 알림 한 쌍을 언제 어떤 값으로 발행하는지다.

**제안.** 기존 `FWxInventoryChangeResult`를 활용해 슬롯 변화의 입력을 한 형태로 만들고, 슬롯·총수량 알림 발행 규칙을 한 함수에서 관리한다. 먼저 현재의 이벤트 순서와 콜백 안에서 인벤토리를 다시 변경할 수 있는 계약을 명시한다. 값을 바꾸는 단계와 알림 단계를 묶거나 나눌 때는 구독자가 관측하는 총수량이 달라지는지도 확인한다.

FastArray와 아이템 인스턴스 복제는 유지한다. `PreReplicatedRemove`가 제거 전 호출되는 특성, 미해석 인스턴스가 나중에 도착할 때의 전체 갱신, 충전 알림의 독립적인 수명도 보존한다.

**완료 기준.** 동일한 추가·소비·제거에 대해 서버와 수신 클라이언트의 의미상 알림이 일치한다. 중첩 콜백, 지연된 인스턴스 복제, 충전 변화에서도 알림 누락·중복이 없다. 단순히 파일을 나누는 변경은 완료 기준이 아니다.

## Q9 UI 비용 표시의 계약 명시

**관찰 당시.** `WxViewModel_Ability::QueryCost`가 Cost GE의 Spec을 만들고 계산한 뒤 첫 번째 유효한 모디파이어 중 값이 거의 0이 아닌 하나를 비용으로 표시했고, `BindCostAttributes`도 그 속성 하나를 구독했다. [공용 비용 GE](C:/Wx/Source/WxGame/AbilitySystem/Effects/WxEffect_Cost.cpp:37)는 CostResource에 맞는 자원만 비용을 내므로 이 모델과 맞는다. 다만 사용자 정의 Cost GE까지 허용하는 기반 API와 UI의 단일 비용 표현 사이 계약은 코드에서 추론해야 했다.

**반영 범위.** 기존 단일 자원 표시 동작을 계약으로 명시하고, 비용을 해석하는 코드를 Ability의 읽기 전용 질의로 옮겼다. ViewModel은 표시와 구독을 담당한다. 반환값은 비용량, 출력 인자는 자원 속성인 기존 방식을 유지한다. 미지원·다중 자원 비용 구성의 오류 진단과 새로운 제한은 보류한다.

- [UWxAbilityBase::QueryCost](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:34)는 비용량을 반환하고 `OutCostAttribute`에 자원 속성을 기록한다. 비용 GE의 정의 순서, 유효 속성·거의 0인 값 필터, 절댓값 처리, 어빌리티 컨텍스트와 레벨을 사용하는 기존 계산을 유지했다. 해당 비용이 없으면 출력 속성을 무효로 설정하고 0을 반환한다.
- [ViewModel의 비용 연결](C:/Wx/Source/WxGame/UI/MVVM/WxViewModel_Ability.cpp:312)은 어빌리티 연결 시 조회 결과를 한 번 표시하고, 조회한 속성에 변경 알림을 구독한다. 교체 시 기존 구독 해제와 비용 없음의 0 표시를 유지했다.
- [조회 계약](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.h:104)은 첫 유효 비용 항목 하나를 조회하며 복수 비용의 합계가 아님을 명시한다. 조회는 비용을 소모하지 않으며, `Effect.IgnoreCosts`에 따른 면제와 무관하게 정의된 비용을 반환한다.

실제 사용 가능 여부를 판정하는 `Ability->CheckCost` 호출은 유지한다. 표시를 단순화하려고 ViewModel에 별도의 비용 판정 공식을 만들면 오히려 두 규칙이 달라진다.

**현재 범위의 완료 기준.** SP·MP·UP·비용 없음 전환에서 표시와 구독이 일치하고, 사용자 정의 GE에서도 기존 비용 선택·표시 동작을 보존한다. 새로운 구성 검증은 추가하지 않는다. 현재 에셋의 비용 표시 오류를 재현했다는 의미는 아니다.

**반영 완료 — 2026-10-02.** 코드 4개 파일에서 비용 조회 책임과 계약을 정리했다. 변경 전후 계산·구독·판정 흐름을 비교했고, Editor Development 빌드와 `git diff --check`를 통과했다. 실제 UI에서 자원 전환을 실행하는 플레이 검증은 수행하지 않았다.

## Q10 코딩 규칙 반복 주석 정리

**관찰 당시.** `GetInstanceDataType()`의 헤더 정의가 코딩 규칙 예외라는 같은 설명이 StateTree 헤더 **24개**에 반복되어 있었다. 해당 예외는 이미 [AGENTS.md](C:/Wx/AGENTS.md)에 명시되어 있다.

**제안.** 규칙을 재서술한 주석은 중앙 규칙 문서에 맡기고, 각 헤더에는 노드의 동작·입력 계약·엔진 함정만 남긴다. 예를 들어 [ComponentMove](C:/Wx/Source/WxGame/Device/WxStateTreeTask_ComponentMove.cpp:11)와 [SplineMove](C:/Wx/Source/WxGame/Device/WxStateTreeTask_SplineMove.cpp:12)의 서로 다른 재선택 정책은 실제 동작 이유를 설명하므로 보존한다.

**완료 기준.** 반복된 규칙 설명만 줄어들고 코드·기능 계약·저작권 문구는 유지된다. 어긋난 주석을 발견하면 현재 동작에 맞춰 정정한다.

**반영 완료 — 2026-10-02.** 헤더 24개에서 동일한 규칙 설명을 각 1줄씩, 총 24줄 삭제했다. 다른 주석과 코드는 유지했다. 작업 전 사본을 기준으로 `verify_comments.py`를 실행해 24개 파일 모두 `COMMENT_VERIFY_RESULT=success`를 확인했고, 반복 문구 잔여 검색과 `git diff --check`도 통과했다. 주석 전용 변경이므로 빌드·플레이 검증은 실행하지 않았다.

## 유지할 설계와 조건부 검토 사항

현재 코드에는 품질 개선의 기반이 이미 있다. 피해 계산의 작은 계산 함수, 몽타주 이벤트 태스크, 컷신의 서버 상태와 로컬 재생 상태 구분, 인벤토리 FastArray, 스포너 로케이터 검증, 빌드 실행기의 환경 진단 분리가 그 예다. 개선 시 이 경계를 활용한다.

- [컷신 컴포넌트](C:/Wx/Source/WxGame/Combat/WxSkillCutsceneComponent.cpp:201)는 서버 세션 종료와 로컬 재생 종료를 의도적으로 구별한다. 파일 길이만을 이유로 하나의 종료 상태로 합치지 않는다.
- [목표 도달 노드의 계약](C:/Wx/Source/WxGame/Quest/WxStateTreeTask_WaitMoveToTarget.h:28)은 0번 컨트롤러를 사용하는 v1 싱글·리슨 호스트 전제를 명시한다. 다른 플레이어별 퀘스트가 요구될 때 대상 선택을 확장한다.
- [스포너 로케이터 검증](C:/Wx/Source/WxGame/Spawner/WxSpawnerLocatorUtils.cpp:21)은 World Partition의 미해석 상태를 고려한다. 성능 측정 없이 강한 참조나 영구 캐시로 바꾸지 않는다.
- `RequestId → Handle` 명칭 정리와 생성자 인라인 규칙 위반 수정은 이미 반영했다. 새로운 미완료 품질 항목으로 다시 집계하지 않는다.

## 권장 진행 순서와 검증

1. **Q3 → Q6:** 기존 도구 실행 결과와 바이너리 파싱 진단을 개선한다. 데이터 구성의 유효성 규칙은 추가하지 않는다.
2. **Q4의 구조 정리:** 일괄 편집의 실패 보장을 강화한다. 기존 입력 검사·호출자·에셋 저장 계약을 유지한다. Q5의 검사 실행부 분리는 완료했다.
3. **Q8 → Q7:** 변경 알림 계약을 먼저 명확히 한 뒤, 실행 수명이 복잡한 어빌리티 구조를 다룬다. Q9의 비용 표시 계약과 조회 책임 분리는 반영했다.
4. **Q10 — 완료:** 주석만 바꾸는 독립 작업으로 처리했다.

Q1·Q2와 각 항목의 새로운 데이터 구성 검증은 이 진행 순서에서 제외한다.

각 항목은 별도 커밋으로 되돌릴 수 있는 크기로 나누는 편이 좋다. 검증은 해당 항목의 완료 기준에 집중한다. 에셋을 바꾸는 도구는 임시 복사본의 수정 전후 비교와 실패 입력을, 런타임 수명 변경은 종료·취소·재진입 및 필요한 네트워크 경로를 확인한다. 임시 테스트 코드·입력은 프로젝트 규칙에 따라 제출 변경에서 제외하고 작업 후 제거한다.

최초 문서 작성에서는 현재 코드의 근거와 링크를 확인했고 소스·에셋은 변경하지 않았다. 이후 Q10은 주석 정리와 코드 불변 검증을 수행했고, Q9는 비용 조회 책임을 분리했다. Q9의 변경 전후 코드를 비교해 기존 계산·구독·판정 흐름을 확인했으며, 빌드 결과는 아래에 기록한다. Q5는 검사 실행부를 분리하고 두 PowerShell 버전에서 검증했다. 실제 UI·플레이 검증은 실행하지 않았다. 이전 빌드·회귀 검사 결과는 수정 기록의 해당 시점 결과이며, 아직 구현하지 않은 제안의 검증 결과로 사용하지 않는다.

## 빌드 결과

- 상태: **성공** — Q9 최종 코드, `WxEditor / Win64 / Development`, UE 5.8.
- 로그: [build_2026-10-02_041740_549_36368.log](C:/Wx/Saved/Logs/BuildDoctor/build_2026-10-02_041740_549_36368.log).

## 원인 요약

Q9의 비용 조회를 Ability로 옮긴 최종 코드가 컴파일·링크를 통과했다. 증분 빌드 14개 작업이 성공했고 종료 코드는 0이다.

## 근거 로그

```text
Result: Succeeded
Total execution time: 61.08 seconds
BUILD_DOCTOR_RESULT=success
BUILD_DOCTOR_EXIT_CODE=0
```

## 수정 방법

빌드 오류에 대한 추가 조치는 필요 없다.

## 재실행 명령

빌드 실행기:

```powershell
& '.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:\Wx'
```

실행기가 출력한 실제 명령:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```
