# PlayerCharacter Resolver WxUI 이동

## 계획

- 사용자 요청에 따라 PlayerCharacter Resolver를 WxUI Public/Private MVVM 경로로 이동하고 API export를 WXUI_API로 변경한다.
- AWxPlayerCharacter 의존을 제거하고 OwningPlayer의 Pawn에서 엔진 AbilitySystemBlueprintLibrary로 ASC를 조회한다. 공유 VM 및 IWxUIData 초기화를 유지한다.
- 기존 WBP의 WxGame 클래스 참조를 WxUI로 연결하는 클래스 리다이렉트를 추가하고 관련 README 경로를 갱신한다.
- UE 5.8 WxEditor Development 빌드와 기존 플레이어 WBP의 Resolver 로드·컴파일을 검증한다. 현재 작업 트리에서 제거된 이전 테스트 파일은 복원하지 않는다.

## 완료

- PlayerCharacter Resolver의 헤더·구현을 WxUI Public/Private MVVM으로 옮기고 WXUI_API로 변경했다.
- AWxPlayerCharacter 캐스팅과 include를 제거했다. OwningPlayer의 Pawn에서 AbilitySystemBlueprintLibrary로 ASC를 찾고 기존 공유 Character ViewModel 및 IWxUIData 초기화 흐름을 유지한다.
- WxGame → WxUI 클래스 리다이렉트를 추가했다. 관련 README의 경계 설명 및 경로를 갱신했다.
- WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-12_161707_922_32832.log.
- WBP_Nameplate_Player와 WBP_PlayerSkills를 기존 에셋 그대로 로드하여 Resolver 클래스가 /Script/WxUI.WxViewModelResolver_PlayerCharacter임을 확인하고 두 WBP 모두 컴파일 성공: Saved/Logs/VerifyPlayerResolverWxUI.log.
- 변경 경로 git diff --check 통과. 이동된 Resolver에서 WxGame 타입 의존성이 남지 않음을 확인했다. 에디터 플레이 검증은 실행하지 않았다.

