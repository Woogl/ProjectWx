# WxCombat DeveloperSettings 추가

## 계획

- `Public/System/WxCombatDeveloperSettings.h`에 `UDeveloperSettings`를 상속하는 `UWxCombatDeveloperSettings`를 추가한다.
- 클래스는 `Config = Game`, `DefaultConfig`, `WXCOMBAT_API`를 사용하고 설정 화면 표시명을 `Wx Combat Settings`로 지정한다.
- `Private/System/WxCombatDeveloperSettings.cpp`에서 기존 Wx 설정 클래스와 동일하게 카테고리를 `Wx`로 지정한다.
- `DefenseConstant`를 `Damage` 카테고리의 설정 변수로 추가하고 기본값을 기존 상수와 같은 `100.f`로 지정한다.
- `WxEffect_Damage`의 방어 배율 계산이 `UWxCombatDeveloperSettings`의 `DefenseConstant`를 사용하도록 변경한다.
- `WxCombat`에 `DeveloperSettings` 모듈 의존성을 추가한다.
- build-doctor로 UE 5.8의 WxEditor Development 빌드를 실행해 컴파일을 검증한다.

## 완료

- `UWxCombatDeveloperSettings`를 추가하고 프로젝트 설정의 `Wx > Wx Combat Settings` 섹션에 노출했다.
- `DefenseConstant`를 기본값 `100.f`인 `Damage` 설정으로 추가했다.
- `WxEffect_Damage`의 방어 배율 계산이 설정값을 사용하도록 변경하고 비정상적인 0 이하 설정값에 런타임 하한을 적용했다.
- `WxCombat`의 공개 의존성에 `DeveloperSettings`를 추가했다.
- Build Doctor로 `WxEditor Win64 Development` 빌드를 실행했으며 성공했다.
