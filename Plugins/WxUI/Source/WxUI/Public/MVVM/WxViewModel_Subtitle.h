// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Subtitle.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 화면 자막 한 줄을 노출하는 범용 뷰모델.
 *
 * 자막은 화면당 하나뿐이라 위젯별로 만들지 않고 MVVM 글로벌 컬렉션에 하나를 보관한다 —
 * 표시하는 위젯과 문구를 거는 ST 노드가 같은 인스턴스를 찾아가야 하기 때문이다.
 * 컬렉션은 엔진의 UMVVMGameSubsystem 이 소유하므로 별도 보관처(컴포넌트·매니저 필드)나 변경 통지 델리게이트가 필요 없고,
 * 자막을 아는 것은 본 VM 하나뿐이다.
 *
 * 값은 외부 소스(퀘스트·기믹의 ST 노드 등)가 push 하며, 본 VM 은 누가 왜 띄웠는지 알지 못한다(순수 표시 계약).
 * 슬롯이 하나라 나중 요청이 이긴다. 회수는 발급 핸들이 지금 걸린 자막의 것일 때만 성립하는데,
 * 이 검사가 없으면 앞 자막이 뒤늦게 걷어가며 이미 올라온 뒤 자막을 지운다.
 */
UCLASS()
class WXUI_API UWxViewModel_Subtitle : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * 글로벌 컬렉션에 보관된 자막 뷰모델을 돌려준다. 아직 없으면 만들어 등록한다.
	 * 표시(리졸버)와 요청(ST 노드) 중 어느 쪽이 먼저 와도 같은 인스턴스가 잡힌다.
	 */
	static UWxViewModel_Subtitle* GetOrCreate(const UObject* WorldContextObject);

	/** 자막을 이 문구로 바꾸고, 나중에 회수할 때 쓸 핸들을 발급한다. */
	int32 ShowSubtitle(const FText& InSubtitleText);

	/** 발급 핸들이 지금 걸린 자막의 것일 때만 화면에서 걷어간다. 어긋나면 무시한다. */
	void HideSubtitle(int32 InSubtitleHandle);

	/** 자막이 걸려 있는지 여부. 자막 위젯의 표시/숨김 바인딩용. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Subtitle")
	bool bHasSubtitle = false;

	/** 현재 자막 본문. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Subtitle")
	FText SubtitleText;

private:
	/** 지금 걸린 자막의 핸들. 자막이 없으면 INDEX_NONE. */
	int32 CurrentHandle = INDEX_NONE;

	/** 다음 발급 핸들. 재사용하지 않으므로 뒤늦은 회수 요청이 엉뚱한 자막을 걷어가지 않는다. */
	int32 NextHandle = 0;
};

/**
 * VM_Subtitle 용 View Bindings Resolver.
 *
 * 위젯별로 만들지 않고 글로벌 컬렉션의 자막 뷰모델을 그대로 돌려준다 — ST 노드가 거는 자막이 곧 이 위젯에 뜬다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_Subtitle : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
