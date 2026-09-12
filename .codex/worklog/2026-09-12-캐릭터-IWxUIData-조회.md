# 캐릭터 표시 데이터 IWxUIData 조회

## 계획

- 승인된 설계에 따라 AWxCharacterBase가 기존 IWxUIData를 구현하여 이름과 초상화를 제공한다. 기존 BP 저장 필드는 유지한다.
- WxUI의 Character ViewModel은 대상 객체의 IWxUIData를 읽어 초기화하고 기존 이미지 비동기 로드 및 공유 수명을 유지한다.
- NameplateComponent의 표시 데이터 필드와 주입 API를 제거하고 네임플레이트·플레이어·보스 호출부를 대상 객체 전달로 갱신한다.
- 기존 Wx.UI 테스트에 Owner 표시 데이터 조회, 인터페이스 미구현 및 재초기화 검증을 반영한다. UE 5.8 WxEditor Development 빌드와 자동화 테스트를 실행한다.

## 완료

- AWxCharacterBase가 기존 IWxUIData를 구현했다. GetTitle/GetIcon은 기존 CharacterName/Portrait 저장 필드를 반환하며 GetDescription은 빈 텍스트를 반환한다. BP 데이터 이전은 필요 없다.
- WxUI Character ViewModel은 Initialize(ASC, DisplaySource)에서 IWxUIData를 조회한다. 기존 FieldNotify와 비동기 이미지 로드·취소 경로를 유지한다.
- NameplateComponent의 Name/Portrait 저장 필드 및 SetDisplayData/GetCharacterName/GetPortrait를 제거했다. Enemy BeginPlay의 주입 호출도 제거했다.
- 네임플레이트 Resolver는 Owner만 전달한다. 플레이어 Resolver와 보스 ViewModel의 Character 초기화 호출도 대상 객체 전달 방식으로 갱신했다. Character ViewModel과 네임플레이트 Resolver는 WxUI에 유지했다.
- 실제 WBP로 Owner 인터페이스의 이름·초상화 조회, 공유 VM 재사용, 서로 다른 Owner, 빈 이미지, 대상 전환, 미구현/null 대상, null ASC를 검증했다. 기존 Scale·가시성·이펙트 지연 생성 검증도 유지했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-12_155416_624_19820.log.
- Wx.UI 자동화 테스트 2개 성공, 경고·오류 0: Saved/Automation/CharacterUIData/index.json.
- git diff --check 통과. 기존 표시 데이터 주입 호출이 남아 있지 않음을 검색으로 확인했다.

