# ItemUse 클래스 WxInventory 이동

## 계획

- 승인된 단순 이동: UWxItemUseComponent와 UWxAnimNotify_UseItem을 WxGame에서 WxInventory의 Public/Private 디렉터리로 옮기고 API 매크로를 WXINVENTORY_API로 변경한다.
- UWxAbility_UseItem과 캐릭터의 기존 호출 API, 서버 권한 검사, Mesh 소유자 검사, 중복 소비 방지 및 이벤트 구독 수명을 유지한다. Subsystem 전환이나 어빌리티 흡수는 하지 않는다.
- 기존 에셋 참조 보존을 위해 두 클래스의 Core Redirect를 추가한다. 기존 include 경로와 모듈 의존성을 유지한다.
- 이동 전후 소스 비교로 구현 보존을 확인하고, 런처 정보에서 UE 5.8 설치 경로를 조회하여 WxEditor Win64 Development를 빌드한다. 에디터는 재실행하지 않으며 런타임 검증 여부를 완료 기록에 명시한다.

## 완료

- 두 클래스의 헤더/구현을 WxInventory의 Public/Private로 이동하고 헤더의 API 매크로를 WXINVENTORY_API로 변경했다. include 경로와 기존 어빌리티·캐릭터 호출부, 모듈 의존성은 그대로 유지했다.
- Config/DefaultEngine.ini에 두 클래스의 WxGame → WxInventory Core Redirect를 추가했다.
- 이동 전후 네 파일을 비교하여 API 매크로 외 구현이 동일함을 확인했다. 작업 범위 git diff --check 통과.
- 최초 샌드박스 내 빌드는 컴파일 진단 없이 종료 코드 1로 종료됐다. build-doctor를 샌드박스 외부에서 실행해 UE 5.8 WxEditor Win64 Development 컴파일·링크 성공(종료 코드 0)을 확인했다.
- 빌드 로그: C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-12_165031_951_10212.log
- 에디터 재실행 및 PIE 사용/취소·기존 에셋 로딩 검증은 수행하지 않았다.
