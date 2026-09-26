# 배포 파일

페이지는 외부 스크립트를 불러오지 않는다. Export-Wiki.ps1이 아래 파일을 HTML에 포함하며, 실행 중 외부 다운로드는 하지 않는다.
갱신할 때는 배포 파일·라이선스·버전 참조·해시를 함께 바꾸고, Export-Wiki.ps1 실행 뒤 TestWikiViewer.cjs와 화면 확인을 한다.

## Mermaid

- 버전: 11.12.0 (고정)
- 파일 출처: https://cdn.jsdelivr.net/npm/mermaid@11.12.0/dist/mermaid.min.js
- 라이선스: MIT, mermaid-LICENSE에 원문 보존
- SHA-256: 07E37DFA97B337CCC85365D57EDDF99B9706F09DB3B59B260D0333B23B343C4B

## marked

- 버전: 18.0.14 (고정)
- 파일 출처: https://cdn.jsdelivr.net/npm/marked@18.0.14/lib/marked.umd.js (npm 공식 배포본, 받은 파일이 jsdelivr 공개 해시와 일치함을 2026-09-26 확인)
- 라이선스: MIT, marked-LICENSE에 원문 보존
- SHA-256: 21568877A938D2C4E7D74E27F18E60DA96BB73A68809610CA39216E1EFEBAE62
- 사용: 문서와 작업 탭의 구현 계획을 그린다. 설정은 index.html의 `marked.use` 한 줄(취소선은 `~~`만)뿐이다.
