# WxCore — 코드 리뷰

> 공용 태그와 인터페이스가 도메인 구현과 분리되어 있고, 이번 정적 검토에서 실행 로직의 확정적 결함은 발견하지 못했다. 새 소환물 계약과 로케이터 표시 헬퍼를 깊게 검토하고, 태그 선언·정의 및 기존 발견을 현재 작업 트리 기준으로 재검증했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

현재 검토 범위에서 수정이 필요한 확정적 결함은 발견하지 못했다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`.
- **훑은 파일**: `Plugins/WxCore/README.md`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`.
- **검증 사항**: Native Tag 114개의 선언·정의가 일치하고 심볼의 `_`를 `.`으로 치환한 문자열도 모두 일치한다. 소환물 인터페이스 기본값은 상한 1·어그로 제외 false이며 cpp에 구현되어 있다. `BlueprintNativeEvent`는 금지된 `BlueprintCallable` 지정자가 아니다. 로케이터 목록은 최대 3개만 해석하고 빈 목록·빈 로케이터·해석 실패를 처리한다. 에디터 가드와 조건부 모듈 의존이 일치하며 다른 Wx 플러그인 참조는 없다. 콜리전 채널 값은 `Config/DefaultEngine.ini:39`와 일치한다.
- **기존 발견 재판정**: `State.Minion.Active`는 `Content/Character/HGTest/Abilities/Skill_1/GA_HGTest_Skill_1.uasset`과 `Skill_2/GA_HGTest_Skill_2.uasset`에서 문자열 참조가 확인되어 미소비 지적을 제거했다. `Ability.Skill.2`도 실제 스킬 에셋 참조가 생겼으므로 기존 미사용 태그 8개 묶음은 유지하지 않았다. 나머지 예약 슬롯은 결함의 근거가 부족하다. README의 주요 네임스페이스 목록에서 `Movement.*`가 빠진 것만으로 실행 가능한 수정 필요성을 입증하기 어려워 별도 발견에서 제외했다.
- **기존 타게팅 주석 지적의 한계**: `WxGameplayTags.h:24`의 `State.Ragdoll` 타게팅 설명은 C++의 발행·구독만으로 옳고 그름을 판정할 수 없다. 현재 타게팅 에셋에서 `Ability.Death` 문자열은 확인했지만 문자열 검색만으로 `IgnoreTags`의 실제 설정을 입증할 수 없어 발견에서 제외했다. 에디터에서 프리셋 설정을 확인해야 한다.
- **미검토 / 한계**: 커밋 `1fab89cf4`와 현재 미커밋 변경을 포함한 작업 트리의 정적 리뷰이다. 빌드·PIE·패키징은 실행하지 않았고 이번에는 엔진 내부 구현을 재검토하지 않았다. BP/WBP 내부 그래프는 범위 밖이며 에셋 검색은 문자열의 존재만 확인한다. 전체 Content 검색에서 일부 외부 리소스에 접근 거부가 발생하여 프로젝트 전체 에셋 참조의 부재를 단정하지 않았다. 소환물 소비 코드에서는 인터페이스 호출과 태그 발행 지점을 검색했으며 해당 도메인 전체 동작을 리뷰한 것은 아니다.

---
*문서 기준 커밋 `1fab89cf4` · 리뷰일 2026-09-12 · 소스 13파일 — `/module-review`로 갱신*
