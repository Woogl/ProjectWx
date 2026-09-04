# MinionManager 서브오브젝트 이름 이관

## 계획

- `AWxCharacterBase`의 `UWxMinionManagerComponent` 네이티브 서브오브젝트 내부 이름을 `MinionComponent`에서 `MinionManagerComponent`로 변경한다.
- 같은 클래스의 기본 서브오브젝트 이름을 옛 값에서 새 값으로 매핑하는 `ClassRedirects.ValueChanges`를 추가한다.
- 기존 Minion 클래스·프로퍼티 리디렉트와 `AWxMinion`, `UWxMinionComponent` 호환 타입은 에셋 재리세이브가 끝날 때까지 유지한다.
- 잔여 설정과 공백 오류를 검사하고 UE 5.8 `WxEditor` Win64 Development 빌드로 이관 경로를 검증한다.
- 이번 단계는 확장·이관 단계에서 멈추며, 리디렉트 및 호환 타입의 최종 제거는 사용자의 재리세이브 확인 후 별도 단계로 진행한다.

## 완료

- `MinionManagerComponent`의 네이티브 서브오브젝트 내부 이름을 `MinionManagerComponent`로 변경했다.
- `AWxCharacterBase`의 기존 `MinionComponent` 서브오브젝트 이름을 새 이름으로 이관하는 `ClassRedirects.ValueChanges`를 추가했다.
- 기존 Minion 클래스·프로퍼티 리디렉트와 `AWxMinion`, `UWxMinionComponent` 호환 타입이 유지되는 것을 확인했다.
- `git diff --check`를 통과했다. 줄 끝 변환 예정 경고만 있었고 공백 오류는 없었다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다. 로그: `Saved/Logs/BuildDoctor/build_2026-09-04_234703_268_16584.log`
- 사용자의 캐릭터 BP 재리세이브 전 단계에서 멈췄다. 완료 확인 후 Minion 관련 리디렉트와 호환 타입을 제거한다.
