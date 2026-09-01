// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Templates/SubclassOf.h"
#include "WxViewModel.generated.h"

struct FStreamableHandle;

/** Pending 은 완료 콜백이 로드된 오브젝트를 해석하는 데 쓰고, Handle 은 취소용이다. */
struct FWxImageRequest
{
	TSoftObjectPtr<UObject> Pending;

	TSharedPtr<FStreamableHandle> Handle;
};

/**
 * 표시용 이미지(아이콘/초상화 등)의 비동기 스트리밍을 공통 제공한다.
 * 이미지는 텍스처와 머터리얼 양쪽이 될 수 있어 UObject 로 다룬다 — 종착지인 UImage 의 브러시 리소스가 둘 다 받는다.
 * VM 은 소프트 참조를 UMG 에 노출하지 않고 로드된 하드 참조만 노출한다.
 */
UCLASS(Abstract, BlueprintType)
class WXUI_API UWxViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/**
	 * Source 를 Outer 로 이미 만들어 둔 뷰모델을 찾는다 — 데이터 소스가 곧 공유 키다.
	 * 클래스는 정확히 일치해야 한다. 보스 뷰모델이 캐릭터 뷰모델을 상속해 IsA 로는 서로를 오매칭한다.
	 */
	static UWxViewModel* FindSharedViewModel(const UObject* Source, TSubclassOf<UWxViewModel> ViewModelClass);

	virtual void BeginDestroy() override;

	/**
	 * 구독과 소스 참조를 놓는 자리다. 파생은 자기 정리 후 Super 를 호출한다.
	 * 진행 중인 이미지 요청은 여기서 일괄 취소된다.
	 *
	 * 표시 필드 변경은 브로드캐스트하지 않는다 — BeginDestroy 에서도 불려, 함께 수거될 뷰의 바인딩으로 통지가 들어간다.
	 * 값을 조용히 되돌리는 것은 자유이며, 표시가 바뀌어야 하면 재초기화 경로나 호출자가 세터로 알린다.
	 */
	virtual void Deinitialize();

protected:
	/**
	 * FieldName 슬롯의 이미지(텍스처 또는 머터리얼)를 비동기 스트리밍하고, 완료 시 ApplyLoadedImage(FieldName, ...) 를 호출한다.
	 * 취소는 같은 FieldName 의 이전 요청만 대상이라 서로 다른 필드끼리는 간섭하지 않는다.
	 * 비어 있거나 이미 로드돼 있으면 스트리밍 없이 즉시 반영한다.
	 */
	void RequestImageAsync(FName FieldName, const TSoftObjectPtr<UObject>& InImage);

	/**
	 * RequestImageAsync 의 결과 수신점.
	 * 파생 VM 이 FieldName 에 해당하는 자기 표시 필드에 세팅한다.
	 */
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage);

private:
	void HandleImageLoaded(FName FieldName);

	TMap<FName, FWxImageRequest> ImageRequests;
};
