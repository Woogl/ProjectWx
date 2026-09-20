# Copyright Woogle. All Rights Reserved.
"""Extract planning text locally; never execute embedded document content."""
import io
import json
import sys
import zipfile
import posixpath
import xml.etree.ElementTree as ET


def extract(data, extension):
    warnings = []
    if extension == '.pdf':
        from pypdf import PdfReader
        reader = PdfReader(io.BytesIO(data))
        if reader.is_encrypted:
            raise ValueError('암호화된 PDF는 암호를 해제한 뒤 첨부하세요.')
        if len(reader.pages) > 500:
            raise ValueError('PDF는 500페이지 이하로 나누어 첨부하세요.')
        parts = []
        for number, page in enumerate(reader.pages, 1):
            text = page.extract_text() or ''
            if not text.strip():
                warnings.append(f'{number}페이지에서 텍스트를 읽지 못했습니다.')
            parts.append(f'[페이지 {number}]\n{text}' if text.strip() else '')
        warnings.append('PDF의 이미지·스캔 글자·도표는 해석하지 않습니다. 표와 읽기 순서를 확인하세요.')
    else:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            if len(archive.infolist()) > 10000 or sum(i.file_size for i in archive.infolist()) > 100 * 1024 * 1024:
                raise ValueError('압축을 푼 문서 크기가 너무 큽니다.')
            def xml(name):
                raw = archive.read(name)
                if b'<!DOCTYPE' in raw.upper() or b'<!ENTITY' in raw.upper():
                    raise ValueError('지원하지 않는 XML 문서입니다.')
                return ET.fromstring(raw)
            def paragraphs(root):
                return '\n'.join(''.join(n.text or '' for n in p.iter() if n.tag.rsplit('}', 1)[-1] == 't')
                                 for p in root.iter() if p.tag.rsplit('}', 1)[-1] == 'p')
            if extension == '.docx':
                parts = [paragraphs(xml('word/document.xml'))]
                warnings.append('Word 본문·표의 텍스트를 읽었습니다. 이미지·머리말·각주와 표 배치는 포함하지 않습니다.')
            else:
                relations = {r.attrib['Id']: r for r in xml('ppt/_rels/presentation.xml.rels')}
                parts = []
                presentation = xml('ppt/presentation.xml')
                for slide in presentation.iter('{http://schemas.openxmlformats.org/presentationml/2006/main}sldId'):
                    relation = relations[slide.attrib['{http://schemas.openxmlformats.org/officeDocument/2006/relationships}id']]
                    if relation.attrib.get('TargetMode') == 'External':
                        raise ValueError('외부 슬라이드 참조는 지원하지 않습니다.')
                    target = relation.attrib['Target']
                    name = posixpath.normpath(target.lstrip('/') if target.startswith('/') else 'ppt/' + target)
                    text = paragraphs(xml(name))
                    parts.append(f'[슬라이드 {len(parts) + 1}]\n{text}' if text.strip() else '')
                warnings.append('슬라이드 순서대로 텍스트를 읽었습니다. 이미지·도표·발표자 노트는 포함하지 않습니다.')
    text = '\n\n'.join(part for part in parts if part.strip()).strip()
    if not text:
        raise ValueError('읽을 수 있는 텍스트가 없습니다. 스캔·이미지 문서는 텍스트로 변환한 뒤 첨부하세요.')
    if len(text) > 120000:
        raise ValueError('추출한 내용이 120,000자를 초과합니다. 문서를 나누어 첨부하세요.')
    return {'text': text, 'warnings': warnings}


if __name__ == '__main__':
    try:
        result = extract(sys.stdin.buffer.read(20 * 1024 * 1024 + 1), sys.argv[1])
    except ImportError:
        result = {'error': 'PDF를 읽으려면 Python 환경에 pypdf가 필요합니다.'}
    except ValueError as error:
        result = {'error': str(error)}
    except Exception:
        result = {'error': '문서를 읽지 못했습니다. 손상·암호 설정과 파일 형식을 확인하세요.'}
    print(json.dumps(result, ensure_ascii=True))
