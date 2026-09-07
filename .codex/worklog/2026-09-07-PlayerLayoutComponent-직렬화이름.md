# PlayerLayoutComponent 직렬화 이름 정리

## 계획

- 사용자 요청에 따라 기본 서브오브젝트 이름 HUDComponent를 PlayerLayoutComponent로 변경한다.
- 기존 컴포넌트 인스턴스 이름을 CoreRedirects ValueChanges로 변환하고 관련 Controller BP만 재저장한다.
- 전투·프론트엔드 LayoutClass 및 컴포넌트 이름을 저장 전후와 별도 프로세스 재로드에서 검증한다. UE 5.8 WxEditor Development를 빌드한다.

## 완료

- 기본 서브오브젝트 식별자를 PlayerLayoutComponent로 변경하고 이전 이름 유지 주석을 제거했다.
- 이전/현재 컴포넌트 클래스에 ValueChanges를 적용해 HUDComponent 인스턴스를 변환한다. 이전 에셋 호환용 리다이렉트는 유지한다.
- BP_PlayerController 및 BP_FrontEndPlayerController만 재저장했다. 저장 전·별도 프로세스 재로드 후 모두 새 이름 및 WBP_GameHUD/WBP_FrontEnd 설정을 확인했다. 두 에셋의 HUDComponent 문자열 검색도 0건이다.
- UE 5.8 WxEditor Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-07_220548_191_21612.log. 재로드 검증: Saved/Logs/VerifySerializedPlayerLayoutName.log. git diff --check 통과.
