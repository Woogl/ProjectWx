---
title: "60FPS 강연 검토 후 적용한 프로젝트 기본 설정 3건"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, config, ui, foundation, decision]
summary: "위젯 속성 바인딩 Prevent, 스태틱 메시 기본 충돌 복잡도 Simple as Complex, 에디터 스케일러빌리티 High 세 가지만 적용했다. 나머지 강연 권장은 미결정이다."
---

# 60FPS 강연 검토 후 적용한 프로젝트 기본 설정 3건

2026-09-25 사용자가 Unreal Fest 2026 강연 「60 FPS로 시작해서 유지하기」(Matt Oztalay)를 프로젝트 현황과 대조하게 했다. 그중 아래 세 가지만 적용했다. 디바이스 프로필·AnimationBudgetAllocator 등 나머지 권장은 미결정이다.

| 커밋 | 파일 | 설정 |
|---|---|---|
| `be1c832e4` | `Config/DefaultEditor.ini` | `[/Script/UMGEditor.UMGEditorProjectSettings]`·`[/Script/Blutility.EditorUtilityWidgetProjectSettings]`의 `DefaultCompilerOptions`에 `PropertyBindingRule=Prevent` |
| `fbf8d89e9` | `Config/DefaultEngine.ini` | `[/Script/Engine.PhysicsSettings] DefaultShapeComplexity=CTF_UseSimpleAsComplex` |
| `ccf16da7b` | `Config/DefaultEditorSettings.ini`(신규) | `[ScalabilityGroups]`의 `sg.*` 11개를 2(High)로. `sg.ResolutionQuality`는 디바이스 프로필에서 정할 값이라 넣지 않았다. |

## 엔진 의미 (UE 5.8 설치본 소스)

- **PropertyBindingRule=Prevent:** 새 속성 바인딩을 만들 수 없다. 이미 바인딩이 있는 위젯은 계속 편집할 수 있고, 바인딩이 없는 위젯에서는 바인딩 버튼이 사라진다. 기존 바인딩을 컴파일할 때 경고·오류를 내는 것은 `PreventAndWarn`·`PreventAndError`다(`WidgetEditingProjectSettings.h`의 `EPropertyBindingPermissionLevel`). 이전에는 미설정(Allow)이었다. Lyra도 UMG에 같은 값을 쓴다.
- **DefaultShapeComplexity:** 충돌 복잡도가 Project Default(`CTF_UseDefault`)인 BodySetup이 이 값을 쓴다(`BodySetupEnums.h`, `BodySetupCore.cpp`). 미설정 시 엔진 기본은 `CTF_UseSimpleAndComplex`다(`PhysicsSettingsCore.cpp:59`). `CTF_UseSimpleAsComplex`는 단순 도형만 만들고 모든 씬 쿼리·충돌 판정에 쓴다. 그래서 Project Default 메시는 complex 쿼리도 단순 도형으로 판정되고, 단순 충돌이 없는 메시는 complex 쿼리에도 맞지 않는다. 명시적으로 복잡도를 지정한 메시는 영향이 없다.
- **에디터 스케일러빌리티:** 에디터(`GIsEditor`, PIE 포함)는 `[ScalabilityGroups]`를 EditorSettings 계층에서 읽고 쓴다. 에디터 밖 게임은 GameUserSettings 계층을 쓴다(`Scalability.cpp:806`). 사용자가 저장한 값(엔진 공용 사용자 폴더)이 프로젝트 기본값을 덮으므로, 이미 품질을 바꿔 저장한 PC에는 적용되지 않는다.

## 적용 전 점검

- 위젯 바인딩: 2026-09-25 재확인에서 WBP_·EUW_ uasset 41개 중 레거시 바인딩(`DelegateEditorBinding`) 문자열이 있는 파일은 0개였다. 바이너리 문자열 검색이며 에셋 로드 검증은 아니다.
- 충돌 복잡도: 적용 당시 에디터 조회에서 StaticMesh 158개 중 Project Default이면서 단순 충돌이 없는 메시는 38개였다. Niagara 샘플 파티클 메시, 참조되지 않는 Megascans LOD, MetaHuman 컨트롤, 풀(`Grass_Clumps`), `SM_Quinn_Simple`(스케일 참조), `SM_Valve`(공성포 레벨 인스턴스), Nodachi 데모 무기다. 명시적 ComplexAsSimple 25개·SimpleAndComplex 34개는 영향이 없다.
- C++(`Source`·`Plugins`)에 complex 트레이스(`bTraceComplex = true` 등)는 0건이라 코드 쪽 영향은 없다고 판단했다. 에셋 쪽 쿼리(PCG 레이 쿼리·애님 노드 트레이스)와 게임 실행·프레임 측정은 확인하지 않았다.

## 근거

- [에디터 설정](../../../Config/DefaultEditor.ini)
- [엔진 설정](../../../Config/DefaultEngine.ini)
- [에디터 스케일러빌리티 기본값](../../../Config/DefaultEditorSettings.ini)
