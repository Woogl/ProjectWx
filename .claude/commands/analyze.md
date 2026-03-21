# 프로젝트 분석

이 Unreal Engine 5 프로젝트의 현재 상태를 종합적으로 분석하라.

## 분석 절차

### 1단계: 모듈 구조 파악

각 모듈의 `.Build.cs` 파일을 읽어 의존성을 확인하라:
- `Source/WxGame/WxGame.Build.cs`
- `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`
- `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`

CLAUDE.md의 모듈 규칙(WxCore 외 플러그인 간 직접 의존 금지)이 지켜지고 있는지 검증하라.

### 2단계: 소스 코드 전수 조사

`Plugins/*/Source/` 및 `Source/WxGame/` 하위의 모든 `.h`, `.cpp` 파일을 읽어라. Intermediate 디렉터리는 제외한다.

각 클래스에 대해 다음을 파악하라:
- 클래스명, 상속 관계
- 핵심 멤버 변수와 함수
- 다른 클래스와의 상호작용
- Gameplay Tag 사용 현황

### 3단계: Gameplay Tag 정리

`WxGameplayTags.h/cpp`를 읽고 프로젝트에서 선언된 모든 Native Gameplay Tag를 카테고리별로 정리하라:
- State 태그
- Event 태그
- ANS(AnimNotifyState) 태그
- Ability 태그
- Input 태그
- UI Layer 태그

### 4단계: 핵심 시스템 흐름 분석

다음 시스템의 데이터 흐름을 추적하라:

**데미지 파이프라인**: 무기/투사체 충돌 → DamageEffect → ExecCalc → AttributeSet → 사망 처리

**어빌리티 활성화 흐름**: Input → PlayerCharacter → ASC → Ability 활성화

**UI 바인딩 흐름**: AttributeSet 변경 → ViewModel → UI 갱신

### 5단계: 코딩 컨벤션 검증

CLAUDE.md에 정의된 코딩 규칙 준수 여부를 확인하라:
1. Copyright 헤더
2. Wx 접두사
3. .h 파일 inline 함수 금지
4. 함수 선언 줄바꿈 금지
5. Handle 접두사 콜백
6. UFUNCTION/UPROPERTY 사이 빈 줄
7. 중괄호 사용
8. Super:: 호출
9. Native Gameplay Tag

### 6단계: 현재 작업 상태

`git status`와 `git log`으로 최근 변경 사항과 진행 중인 작업을 파악하라.

### 7단계: 개선 제안

분석 결과를 바탕으로 다음을 제안하라:
- 코딩 컨벤션 위반 사항
- 모듈 의존성 규칙 위반
- 잠재적 버그나 문제점
- 구조적 개선 가능 영역

## 출력 형식

분석 결과를 다음 구조로 출력하라:

```
## 프로젝트 개요
## 모듈 구조 및 의존성
## 클래스별 상세 분석
### WxCore
### WxCombat
### WxUI
### WxGame
## Gameplay Tag 현황
## 핵심 시스템 흐름
## 코딩 컨벤션 검증 결과
## 최근 변경 사항
## 개선 제안
```
