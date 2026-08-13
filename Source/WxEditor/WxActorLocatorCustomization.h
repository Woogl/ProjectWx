// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyEditorModule.h"

class AActor;
class IPropertyHandle;
class SComboButton;
class SWidget;
class UClass;

// 레벨 액터를 가리키는 FUniversalObjectLocator 필드를 AllowedLocators="Actor" 메타로 식별한다.
// 단일 멤버와 배열 원소 모두 걸린다 — 배열 원소 프로퍼티에는 메타가 없으므로 소유 배열 프로퍼티까지 본다.
// 파라미터 백(퀘스트 스텝 루트 파라미터·링크 상태 오버라이드)이 생성한 프로퍼티도 desc 메타 전파로 같은 조건에 걸린다.
class FWxActorLocatorTypeIdentifier : public IPropertyTypeIdentifier
{
public:
	virtual bool IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const override;
};

// 액터 지정 UOL 필드의 한 줄 픽커 — 현재 액터 라벨 콤보(씬 아웃라이너 검색·Use Selected·Clear)와 스포이드.
// 기본 UOL 편집기(로케이터 타입 콤보+중첩 행)를 대체해 표준 액터 픽커와 같은 조작감을 낸다.
// AllowedClasses 메타(클래스 경로 1개)가 있으면 픽커 후보를 그 클래스로 제한한다.
// CustomizeHeader 는 ST 인스턴스 데이터 직속 행에서 두 번 돈다 — 엔진이 버려질 행에 한 번 더 실행하므로(FDetailPropertyRow::GetDefaultWidgets), 호출마다 위젯과 멤버를 통째로 다시 만들어야 한다.
// 엔진 기본 UOL 편집기가 그 자리에서 값 위젯을 못 그리는 것도 같은 이유다 — 재빌드 요청 플래그가 버려진 위젯에 걸린 채 남아 진짜 행의 재빌드가 무시된다.
// 저장이 UOL 인 이유: 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증 밖이고 PIE·WP 해석이 내장돼 있다(소프트 참조는 에셋 저장이 거부됨).
class FWxActorLocatorCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedRef<SWidget> HandleGetPickerMenu();
	FText HandleGetCurrentActorText() const;
	bool HandleShouldFilterActor(const AActor* Actor) const;
	void HandleActorSelected(AActor* Actor);
	void HandleGetAllowedClasses(TArray<const UClass*>& OutClasses);
	void HandleUseSelectedActor();
	void HandleCloseMenu();

	/** 현재 값을 편집 중인 레벨 기준으로 해석한다. 미해석(언로드 등)이면 null. */
	AActor* GetCurrentActor() const;

	/** 값 기입 공통 경로. null 이면 Clear 로 동작하며, 트랜잭션·변경 통지를 포함한다. */
	void SetLocatorToActor(AActor* Actor);

	TSharedPtr<IPropertyHandle> PropertyHandle;

	TSharedPtr<SComboButton> PickerCombo;

	/** AllowedClasses 메타가 없으면 AActor. 네이티브 클래스만 상정하므로 GC 소유 표기가 필요 없다. */
	UClass* AllowedClass = nullptr;
};
