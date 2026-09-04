# ProjectileManagerComponent 이름 변경

## 계획

- `WxProjectileComponent.h/.cpp` 파일과 `UWxProjectileComponent` 클래스를 `WxProjectileManagerComponent.h/.cpp`, `UWxProjectileManagerComponent`로 변경한다.
- `AWxCharacterBase`의 전방 선언, include, 멤버, 기본 서브오브젝트 이름을 `ProjectileManagerComponent`로 변경한다.
- 기존 캐릭터 블루프린트의 직렬화 호환을 위해 클래스·프로퍼티·서브오브젝트 리다이렉트를 추가한다.
- 기존 작업 중인 `WxCharacterBase`와 `DefaultEngine.ini` 변경은 보존한다.
- 이전 이름 잔존 여부를 검사하고 UE 5.8 `WxEditor Development` 타겟을 빌드한다.

## 완료

- `WxProjectileComponent.h/.cpp`와 `UWxProjectileComponent`를 `WxProjectileManagerComponent.h/.cpp`, `UWxProjectileManagerComponent`로 변경했다.
- `AWxCharacterBase`의 전방 선언, include, 멤버, 기본 서브오브젝트 이름을 `ProjectileManagerComponent`로 변경했다.
- 기존 캐릭터 BP가 이전 클래스·프로퍼티·서브오브젝트 이름을 계속 불러올 수 있도록 `CoreRedirects`를 추가했다.
- 관련 주석과 모듈 리뷰 문서의 파일명을 새 이름에 맞췄다.
- 이전 이름은 호환 리다이렉트와 변경 이력을 기록한 이 작업 로그에만 남는 것을 확인했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다. Build Doctor 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_210621_040_7424.log`.
