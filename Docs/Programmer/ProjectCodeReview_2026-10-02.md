# WX 프로젝트 전체 코드 리뷰 — 2026-10-02

> **수정 전 검토 기록:** 아래 코드 위치·수치·미실행 항목은 리뷰 당시 기준이다. 이후 R1~R6·C1 및 추가 확인 항목인 대화 복구를 수정했다. 현재 변경 내용과 검증 결과는 [수정 기록](C:/Wx/Docs/Programmer/ProjectCodeReview_Fixes_2026-10-02.md)에 있다.

## 결과

합의한 **텍스트 코드·설정 529개 파일, 40,834줄을 모두 읽고 관련 에셋 구성을 대조**했다. 기능상 수정 권고는 **P2 6건**, 코딩 규칙 위반은 **P3 1건**이다. Editor Development 증분 빌드는 성공했다. 이 문서는 이전의 주요 경로 중심 리뷰를 전수 리뷰 결과로 갱신한 것이다.

| ID | 우선순위 | 문제 | 확인 방법 |
|---|---|---|---|
| R1 | P2 | 부활 성공 경로에 회복병 충전 복구가 없음 | 코드·기획·관련 에셋 참조 대조 |
| R2 | P2 | BP_Minion 전용 초기 속성을 뒤의 공용 AbilitySet이 덮어씀 | 적용 순서·에셋의 초기값 대조 |
| R3 | P2 | 시간 배율 값만 비교해 다른 슬로모션 요청을 해제함 | 시작·종료 순서 추적 |
| R4 | P2 | 에셋 목록 추출기가 음수 int8/FText None을 읽으면 예외 발생 | 실제 함수에 합성 바이트 입력, 예외 재현 |
| R5 | P2 | 짧은 이름의 Core Redirect는 텍스트 참조가 남아도 SAFE로 판정 | 실제 검사 함수에 임시 파일 입력, 오판정 재현 |
| R6 | P2 | 패리로 공격이 취소돼도 이미 수집한 Sweep의 나머지 타격을 처리 | 무기·GAS·몽타주 종료 호출 경로 추적 |
| C1 | P3 | FWxDamageStatics 생성자를 구조체 내부에 정의 | 프로젝트의 인라인 함수 금지 규칙 대조 |

P2는 해당 조건에서 기능 오류를 일으켜 수정을 권고하는 항목이고, P3는 낮은 우선순위의 규칙 준수 항목이다. P0/P1로 확정할 문제는 발견하지 못했다. **정적 리뷰 완료는 무결함 보증이나 모든 플레이 상황의 재현 완료를 뜻하지 않는다.** R4·R5는 실행 재현했고, R1·R2·R3·R6의 게임 내 재현은 수행하지 않았다.

## 검토 기준과 범위

- 전수 리뷰 시작 HEAD: `f2645c062caa772371bbdac5cb33ec3c53391916`.
- 문서 작성 시 확인한 HEAD: `f21dbb0d907716447337e8a54efa2238fac3eb87`.
- 커밋 차이만 본 것이 아니라 실제 작업 트리 파일 전체를 읽었다. 두 시점 사이 HEAD가 바뀌었지만, **529개 파일의 SHA-256 변경 0건, 대상 목록의 추가·누락 0건**을 확인했다. 재검증 기준은 커밋보다 [파일별 검토 목록](C:/Wx/Docs/Programmer/ProjectCodeReview_Coverage_2026-10-02.md)의 해시다.
- `Source/` 전체, 자체 플러그인 3개 전체, 프로젝트·플러그인 기술 파일, `Config/`, `BatchFiles/`, `.agents/`의 빌드·운영 스크립트를 포함했다. 529개 모두 본문을 읽었고, 주요 기능 사이 호출·수명·데이터 흐름도 추적했다.
- 추가로 저장소·도구 설정 6개(`.gitignore`, `.gitattributes`, `.mcp.json`, `.codex/config.toml`, `.claude/settings.json`, `.gemini/settings.json`)를 읽었다. 529개 집계에는 중복 포함하지 않았다.
- 엔진 소스, 외부 라이브러리, 자동 생성 코드, 바이너리와 캐시, 원본 기획·교육 HTML 6개의 내장 표시용 번들은 프로젝트 코드 529개에 포함하지 않았다. HTML은 문서·번들 구조만 확인했으며 내장 프로그램 전체를 리뷰했다고 집계하지 않았다. 엔진 코드는 판정 근거가 필요한 부분만 대조했다.
- 관련 에셋은 어빌리티·몽타주 노티파이·AbilitySet·캐릭터 초기 속성·GE·피해 행의 구성을 검토했다. **모든 블루프린트 그래프, StateTree 전이, 레벨 배치를 전수 조사한 것은 아니다.** 이는 사용자가 선택한 “텍스트 코드 전체와 관련 에셋 구성 검토” 범위다.
- 성능 프로파일링, PIE 플레이, 패키징, 멀티플레이 실행, 전체 클린 빌드는 수행하지 않았다. 소스·에셋 수정, 커밋, 푸시도 수행하지 않았다.

### 영역별 집계

| 영역 | 파일 | 줄 | 주요 검토 내용 |
|---|---:|---:|---|
| WxGame | 458 | 32,827 | GAS·전투·AI·캐릭터·장치·퀘스트·인벤토리·UI·저장·스폰·타기팅 전체 |
| WxEditor | 17 | 1,627 | 커스터마이징·검증·에디터 전용 기능 |
| Source 타겟 정의 | 2 | 31 | Game/Editor 타겟 |
| BoxComponentVisualizer | 6 | 309 | 에디터 조작·컴포넌트 변경 |
| DataTableRowFixup | 8 | 503 | 행 변경 감지·참조 수정·모듈 수명 |
| WxToolset | 16 | 2,403 | DataTable·몽타주·MVVM 도구 |
| .agents 스크립트 | 9 | 1,818 | 에셋 추출·보고·빌드·Git 운영 |
| BatchFiles | 6 | 913 | 빌드·프로젝트 생성·리다이렉트 검사 |
| Config | 6 | 300 | 엔진·게임·입력·플랫폼 설정 |
| Wx.uproject | 1 | 103 | 모듈·플러그인 구성 |
| **합계** | **529** | **40,834** | **모든 대상 파일 정적 검토 완료** |

기능 폴더별 상세 집계와 529개 개별 파일은 [검토 목록](C:/Wx/Docs/Programmer/ProjectCodeReview_Coverage_2026-10-02.md)에 기록했다. 별도 결함이 기재되지 않은 파일도 검토 대상이며, 무결함으로 보증하는 의미는 아니다.

### 관련 에셋 조사

[Wiki 색인](C:/Wx/Wiki/index.md)부터 프로젝트 맥락을 확인하고, 결함은 현재 소스와 기획 원문을 대조해 판단했다. [Export-AbilitySystemLists.ps1](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1)을 실행한 뒤 생성된 [어빌리티 목록](C:/Wx/Saved/AbilitySystemLists/ability-list.md), [캐릭터 목록](C:/Wx/Saved/AbilitySystemLists/character-list.md), [이펙트 목록](C:/Wx/Saved/AbilitySystemLists/effect-list.md)을 읽었다.

추출은 성공했고, 어빌리티 40개·세트 9개·캐릭터 7개·몽타주 54개·C++ 이펙트 26개·GE 에셋 6개·피해 테이블 1개를 확인했다. 이 수치는 추출기가 수집한 관련 구성의 수이며 프로젝트의 모든 바이너리 에셋 수가 아니다. `Saved/`의 목록과 빌드 로그는 로컬 생성물이다.

## R1. [P2] 부활 성공 시 회복병 충전을 복구해야 함

**발생 조건:** 충전형 아이템을 사용한 뒤, 사망 화면에서 `RequestRespawn()`으로 같은 월드에 부활한다.

인벤토리는 [PlayerController 생성자](C:/Wx/Source/WxGame/Player/WxPlayerController.cpp:23)에서 생성한다. [부활 함수](C:/Wx/Source/WxGame/Player/WxRespawnLibrary.cpp:44)는 같은 컨트롤러를 유지하고 Pawn을 교체하므로 인벤토리와 아이템 인스턴스가 유지된다. 성공 뒤 [회복 처리](C:/Wx/Source/WxGame/Player/WxRespawnLibrary.cpp:63)는 HP·MP만 최대치로 설정한다. 인벤토리의 [시작 아이템 지급](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:221)은 다시 실행되지 않는다.

[RefillItemCharges](C:/Wx/Source/WxGame/Inventory/WxInventoryComponent.cpp:545)의 C++ 호출자는 [충전 StateTree 태스크](C:/Wx/Source/WxGame/Inventory/WxStateTreeTask_RefillItemCharges.cpp:43)이며 부활 함수는 이 태스크를 실행하지 않는다. 관련 사망 화면 에셋에서 `RequestRespawn` 참조를 확인했고 별도의 리필 참조는 확인되지 않았다. 모든 BP 그래프를 실행 검증한 것은 아니다.

**영향:** 소모한 충전량이 부활 후에도 남는다. [에스트병 기획서](<C:/Wx/Docs/CombatDesign/에스트병 기획서.md:128>)의 사망 후 재시작 시 최대 충전 규칙과 다르다.

**수정 방향:** 새 Pawn 생성·빙의가 성공한 뒤 유지 중인 인벤토리의 아이템을 순회해 `RefillItemCharges(Item)`을 호출한다. 기존 StateTree 태스크와 같은 리필 처리를 공용 함수로 묶어도 된다. 시작 아이템을 다시 지급할 필요는 없다.

**수정 후 확인:** 충전량 0·일부 소모 상태에서 부활해 최대치 및 HUD 복구를 확인한다. 비충전형 아이템과 획득 내역은 유지되고, Pawn 생성 실패 시에는 충전량이 변경되지 않아야 한다.

## R2. [P2] BP_Minion의 공용 AbilitySet이 전용 초기 속성을 덮어씀

**발생 조건:** 현재 [BP_Minion](C:/Wx/Content/Character/Minion/BP_Minion.uasset)의 ASC가 초기화된다.

[캐릭터 목록](C:/Wx/Saved/AbilitySystemLists/character-list.md:28)의 배열 순서는 `ABS_Minion → ABS_Shared_Enemy`다. [ASC 초기화 루프](C:/Wx/Source/WxGame/AbilitySystem/WxAbilitySystemComponent.cpp:82)는 순서대로 세트를 적용하고, [AbilitySet 적용](C:/Wx/Source/WxGame/AbilitySystem/WxAbilitySet.cpp:19)은 `AttributeInitRow`에 있는 공통 속성 전체를 다시 쓴다.

[ABS_Minion](C:/Wx/Content/Character/Minion/ABS_Minion.uasset)은 `DT_CharacterAttribute.Minion`, [ABS_Shared_Enemy](C:/Wx/Content/Character/Template/Shared/ABS_Shared_Enemy.uasset)는 `DT_CharacterAttribute.TemplateEnemy`를 지정한다. 공용 세트에도 속성 초기화가 들어 있다.

| 속성 | Minion 행 | 나중에 적용하는 TemplateEnemy 행 | 세트 적용 직후 기본값 |
|---|---:|---:|---:|
| ATK | 40 | 20 | 20 |
| CritRate | 5 | 0 | 0 |
| CritDMG | 50 | 0 | 0 |
| HP / MaxHP | 0 / 0 | 100 / 100 | 100 / 100 |
| MaxGP | 0 | 50 | 50 |

수치는 [추출한 초기값](C:/Wx/Saved/AbilitySystemLists/character-list.md:50)에 근거한다. 이후 효과·플레이 중 변경까지 포함한 최종 전투 수치는 아니다.

**영향:** 전용 행에서 ATK·치명타를 조정해도 해당 캐릭터의 초기값에 반영되지 않는다. ATK 기본값이 40에서 20이 된다는 뜻이며, 최종 피해가 반드시 절반이라는 뜻은 아니다.

**수정 방향:** 속성 초기화를 담당할 세트를 하나로 정하거나, 공용 어빌리티와 속성 초기화를 분리한다. **단순히 배열 순서만 바꾸면 현재 Minion 행의 HP/MaxHP 0까지 마지막에 적용된다.** 전용 세트를 마지막에 적용하려면 먼저 전용 행에 유효한 전체 초기값을 채워야 한다. 공용 세트의 다른 사용자도 함께 확인해야 한다.

**수정 후 확인:** 스폰 직후 ATK·CritRate·CritDMG·HP·MaxHP·MaxGP를 의도한 초기값과 비교한다. Minion뿐 아니라 공용 세트를 사용하는 Soldier·Template와 별도 구성을 가진 Doppelganger도 확인한다.

## R3. [P2] 배율 값 비교만으로 슬로모션 요청의 소유권을 판단함

**발생 조건:** 같은 월드에서 두 SlowTime 태스크가 같은 실제 시간 배율을 요청하고, 먼저 시작한 태스크가 먼저 종료된다.

[Activate](C:/Wx/Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_SlowTime.cpp:40)는 적용한 배율만 기록한다. [OnDestroy](C:/Wx/Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_SlowTime.cpp:20)는 현재 배율이 자기 값과 같으면 전역 배율을 `1.f`로 되돌린다.

| 순서 | 동작 | 전역 배율 | 활성 요청 |
|---|---|---:|---|
| 1 | A 시작 | 0.2 | A |
| 2 | B 시작 | 0.2 | A, B |
| 3 | A 종료, 값이 같아 복구 | 1.0 | B |

**영향:** B가 유효한 동안 슬로모션이 해제된다. 서로 다른 요청값이 엔진 제한으로 동일한 실제 배율에 보정돼도 소유권을 구별할 수 없다.

[몽타주 구간 시작](C:/Wx/Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp:252)과 [종료](C:/Wx/Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp:166)에서 태스크를 생성·종료한다. 회피·가드 반응 몽타주에 SlowTime 노티파이가 있는 것은 확인했지만, 실제 레벨의 중첩 빈도는 측정하지 않았다.

**수정 방향:** 월드 단위로 활성 요청과 식별자를 관리하고 자기 요청만 제거한다. 마지막 요청 우선 또는 최소 배율 우선 등 연출 정책에 따라 남은 요청에서 배율을 다시 계산한다. [스킬 컷신의 전역 배율 변경](C:/Wx/Source/WxGame/Combat/WxSkillCutsceneComponent.cpp:141)도 같은 관리 경로에 포함한다.

**수정 후 확인:** 같은 배율·다른 배율의 중첩, 양쪽 종료 순서, 취소, 컷신 중첩을 검사한다. 종료한 요청의 값이 나중에 되살아나지 않아야 한다.

## R4. [P2] 에셋 목록 추출기의 signed byte 변환이 정상 입력에서 예외를 던짐

**발생 조건:** 추출 대상 속성에 음수 `Int8Property`가 있거나 `ETextHistoryType::None`인 FText를 읽는다.

[Read-Value](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:280)와 [Read-Text](C:/Wx/.agents/scripts/Export-AbilitySystemLists.ps1:318)는 `byte`를 `[sbyte]`로 직접 변환한다. PowerShell의 이 변환은 비트 재해석이 아니라 범위 검사이므로, `128..255`에 예외를 던진다. UE 5.8의 [TextHistory.h](<C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Private/Internationalization/TextHistory.h:24>)에서 `None = -1`이며 직렬화 바이트는 `0xFF`다. 따라서 코드에 있는 `history == -1` 처리에 도달하지 못한다.

원본 스크립트의 함수 정의를 그대로 가져와 호출했다. 스크립트 전체의 상위 실행부는 실행하지 않았다.

| 실제 함수 호출 입력 | 기대 | 실제 결과 |
|---|---|---|
| `Read-Text`: `00 00 00 00 FF 00 00 00 00`, 시작 위치 0 | None 이력의 빈 문자열 | `255`를 `System.SByte`로 변환할 수 없다는 예외 |
| `Read-Value`: `Int8Property`, 바이트 `FF`, 크기 1 | `-1` | 같은 변환 예외 |

**영향:** 정상적인 직렬화 값 때문에 목록 갱신이 실패할 수 있다. 상위 예외 처리에서 추출이 중단되거나 일부 복합 속성이 대체 표기로 빠질 수 있다. 이번 실제 에셋 목록 추출은 성공했으므로 현재 출력 전체가 잘못됐다고 판단하지는 않는다.

**수정 방향:** 바이트를 정수로 읽고 128 이상이면 256을 빼는 방식 등으로 signed 8비트 값을 명시적으로 복원한다. FText의 None 판정만 필요하다면 원시 바이트 `0xFF`와 비교하는 방법도 있다.

**수정 후 확인:** `00`, `7F`, `80`, `FF` 경계값과 Base/None FText를 확인한 뒤 실제 에셋 목록을 다시 생성해 비교한다.

## R5. [P2] 짧은 이름의 Core Redirect가 살아 있는 코드 참조를 놓침

**발생 조건:** `OldName="LegacyActor"`처럼 패키지 경로를 생략한 리다이렉트가 있고, 소스에는 `/Script/WxGame.LegacyActor` 문자열 참조가 남아 있으나 바이너리 참조는 없다.

[텍스트 검사](C:/Wx/BatchFiles/CheckRedirects.bat:438)는 GameplayTag/GameName 이외에는 OldName이 `/`로 시작할 때만 검색 패턴을 만든다. 짧은 이름은 다음 줄에서 검사를 건너뛴다. UE의 [Core Redirect 이름 매칭](<C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/CoreUObject/Private/UObject/CoreRedirects.cpp:677>)은 생략된 패키지 이름을 허용하므로 짧은 이름 자체는 유효하다.

임시 프로젝트에 다음 설정과 소스를 작성하고 원본의 `Get-Redirects`, `Invoke-Audit`, `Get-Status`를 호출했다. `Saved` 등 제외 디렉터리 밖에서 검사했으며 실제 검색 대상이 Config·Source 2개 파일임을 확인했다.

```ini
[CoreRedirects]
+ClassRedirects=(OldName="LegacyActor",NewName="/Script/WxGame.WxCharacterBase")
```

```cpp
LoadClass<AActor>(nullptr, TEXT("/Script/WxGame.LegacyActor"));
```

| 설정의 OldName | 읽은 텍스트 파일 | 발견한 참조 | 판정 |
|---|---:|---:|---|
| `LegacyActor` | 2 | 0 | **SAFE — 오판정** |
| `/Script/WxGame.LegacyActor` | 2 | 1 | REVIEW |

[Get-Status](C:/Wx/BatchFiles/CheckRedirects.bat:449)는 텍스트·바이너리 증거가 없으면 SAFE를 반환하고, [Remove-SafeRedirects](C:/Wx/BatchFiles/CheckRedirects.bat:624)는 이를 삭제 대상으로 사용한다.

**영향:** 사용자가 도구의 정리 확인에 동의하면 여전히 필요한 리다이렉트를 제거해 문자열 기반 로딩을 깨뜨릴 수 있다. 실제 프로젝트의 리다이렉트를 삭제하거나 현재 콘텐츠의 로드 실패를 재현한 것은 아니다. 결함은 입력에 따른 검사·삭제 후보 판정에 있다.

**수정 방향:** 짧은 이름도 정규화한 오브젝트 이름·경로를 찾아야 한다. 참조 여부를 확정할 수 없는 표기는 SAFE 대신 REVIEW로 유지한다. 삭제 여부를 결정할 때 “검사하지 않음”과 “검사 결과 없음”을 구분해야 한다.

**수정 후 확인:** 짧은 이름·전체 경로 각각의 코드/ini 문자열 참조를 검사한다. 참조가 있는 항목이 SAFE에 들어가지 않고, 참조가 없는 지원 형식만 정리 대상이 되는지 확인한다.

## R6. [P2] 타격 콜백에서 공격이 종료돼도 현재 Sweep 처리를 계속함

**발생 조건:** 활성 근접 공격의 같은 Tick에 적대 대상 A·B가 Sweep 결과에 잡히고, 먼저 처리한 A의 퍼펙트 가드가 공격자의 패리 반응 또는 그로기를 발동해 해당 공격을 취소한다.

[Tick 진입](C:/Wx/Source/WxGame/Weapons/WxWeaponBase.cpp:145)에서는 `ActiveAttackCount`를 검사하지만, [수집한 Hits 순회](C:/Wx/Source/WxGame/Weapons/WxWeaponBase.cpp:183) 중에는 공격이 계속 유효한지 다시 확인하지 않는다. [ProcessHit](C:/Wx/Source/WxGame/Weapons/WxWeaponBase.cpp:224)에도 공격 활성 여부 검사가 없다.

호출 경로는 다음과 같다.

1. `ProcessHit(A)`가 피해를 적용한다.
2. [퍼펙트 가드 처리](C:/Wx/Source/WxGame/AbilitySystem/Effects/WxEffectComponent_PerfectGuard.cpp:40)가 반사 GP와 패리 이벤트를 공격자에게 보낸다.
3. [패리 피격 반응](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbility_HitReact.cpp:24)은 공격·스킬을, [그로기](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbility_Groggy.cpp:31)는 액션을 취소한다. 각 반응의 발동 조건이 충족되는 경우다.
4. [어빌리티 종료](C:/Wx/Source/WxGame/AbilitySystem/Abilities/WxAbilityBase.cpp:297)가 몽타주 이벤트 태스크를 끝내고, [구간 정리](C:/Wx/Source/WxGame/AbilitySystem/Tasks/WxAbilityTask_MontageEvents.cpp:162)가 `Weapon->EndAttack()`을 호출한다.
5. [마지막 공격 구간 종료](C:/Wx/Source/WxGame/Weapons/WxWeaponBase.cpp:89)는 충돌·Tick을 끄지만, 이미 실행 중인 C++ 루프를 중단하지 않는다. 돌아온 루프가 다음 `ProcessHit(B)`를 호출한다.

**영향:** 공격이 취소된 뒤에도 같은 프레임의 남은 대상에게 피해가 들어간다. 종료 시 피격 집합도 비우므로 다음 충돌 형상에서 이미 처리한 대상의 중복 타격을 막던 기록까지 사라질 수 있다. 실제 게임 내 다중 대상 배치로 재현하지 않았으며, 위 조건에서 성립하는 호출 흐름상의 결함으로 분류했다.

현재 에셋에는 [Parry 몽타주 섹션](C:/Wx/Saved/AbilitySystemLists/ability-list.md:153)과 [패리 가능한 피해 행](C:/Wx/Saved/AbilitySystemLists/effect-list.md:61)이 있다. 모든 적 패턴이 패리로 취소된다는 뜻은 아니다.

**수정 방향:** 각 타격 전후에 공격 활성 상태와 공격 구간 식별자가 여전히 같은지 확인해 종료·교체 시 남은 결과 처리를 중단한다. 콜백에서 새로운 공격이 시작될 가능성까지 고려하면 단순 카운트 검사보다 공격 세대/ID 비교가 안전하다. 여러 형상과 Overlap 경로도 같은 수명 규칙을 따라야 한다.

**수정 후 확인:** 첫 대상의 퍼펙트 가드로 공격을 취소했을 때 두 번째 대상에 피해가 없는지, 반사 GP 그로기에서도 같은지 검사한다. 취소가 없으면 정상적인 다중 타격이 유지되고, 다음 공격은 새 피격 기록을 가져야 한다.

## C1. [P3] 구조체 내부 생성자 정의가 인라인 금지 규칙과 충돌

[FWxDamageStatics 생성자](C:/Wx/Source/WxGame/AbilitySystem/Effects/WxEffect_Damage.cpp:44)의 본문이 구조체 내부에 정의돼 있어 C++ 규칙상 암시적 inline이다. 파일이 `.cpp`여도 해당된다. 템플릿이나 StateTree 예외가 아니며 생성자 자체는 엔진 매크로가 생성한 것도 아니다.

같은 `.cpp`에서 구조체 내부에는 선언만 두고 밖에 `FWxDamageStatics::FWxDamageStatics()`를 정의하면 된다. 기능 오류로 집계하지 않았다. 검토 대상 C++·헤더·C# 파일의 첫 줄 저작권 문구는 일치했다.

## 추가 확인: 대화 중단 이후 퀘스트의 회복 전이

이 항목은 확정 결함에 포함하지 않는다. [대화 태스크](C:/Wx/Source/WxGame/Dialogue/WxStateTreeTask_PlayDialogue.cpp:50)는 정상 완료일 때만 성공한다. [Pawn 교체](C:/Wx/Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp:167), 다른 대화로 교체, 다음 행 해석 실패 시 대화가 `false`로 종료되고 [델리게이트도 정리](C:/Wx/Source/WxGame/Dialogue/WxDialogueSessionComponent.cpp:234)된다.

별도 회복 전이가 없으면 기존 태스크가 Running에 남지만, [헤더의 계약](C:/Wx/Source/WxGame/Dialogue/WxStateTreeTask_PlayDialogue.h:25)은 회복 책임을 트리 저작에 둔다. 실제 퀘스트 StateTree의 모든 전이를 조사하지 않았으므로 설계 의도를 버그로 바꾸어 집계하지 않았다.

`Play Dialogue`를 사용하는 트리에서 사망·부활, 대화 교체 후 재진입·재개 가능 여부를 확인할 필요가 있다. 단순히 중단을 Failed로 변경하면 [퀘스트 실행 종료 시 저널 정리](C:/Wx/Source/WxGame/Quest/WxQuestComponent.cpp:117)에 영향을 주므로 회복 정책과 함께 검토해야 한다.

## 빌드 결과

- **성공:** `WxEditor / Win64 / Development`, UE 5.8.
- 실행기: [build-doctor](C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1).
- 로그: [build_2026-10-02_024933_938_37808.log](C:/Wx/Saved/Logs/BuildDoctor/build_2026-10-02_024933_938_37808.log).

## 원인 요약

증분 빌드에서 WxGame의 unity 컴파일 3개, WxToolset 컴파일, 관련 라이브러리·DLL 링크 및 메타데이터 기록을 포함한 9개 작업이 성공했다. 전체 실행 시간 22.48초, 종료 코드 0이다. 전체 클린 빌드 결과는 아니다.

## 근거 로그

```text
Result: Succeeded
Total execution time: 22.48 seconds
BUILD_DOCTOR_RESULT=success
BUILD_DOCTOR_EXIT_CODE=0
```

## 수정 방법

빌드 오류에 대한 조치는 필요 없다. R1~R6과 C1은 이 리뷰에서 수정하지 않았다. 각 항목의 수정 방향·후속 검증을 따라 별도 변경으로 처리할 수 있다. 검증용 스크립트와 합성 입력 파일은 최종 제출물에서 제외하고 제거했다.

## 재실행 명령

검토에 사용한 실행기:

```powershell
& '.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:\Wx'
```

실행기가 출력한 실제 빌드 명령:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```
