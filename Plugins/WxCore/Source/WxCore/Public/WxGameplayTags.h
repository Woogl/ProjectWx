// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 프로젝트 전체에서 사용하는 Gameplay Tag 선언부.
 * 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성.
 *
 * 네이밍: 점(.) 구분자를 언더스코어(_)로 치환하여 변수명 사용.
 * 예) "State.Guard" → State_Guard
 *
 * 어빌리티 태그 규칙:
 * 1. 어빌리티는 자신을 가리키는 식별 태그 Ability.X를 정확히 하나 갖고, AssetTags와 ActivationOwnedTags 양쪽에 넣는다. 곧 "Ability.X = 그 어빌리티가 지금 돌고 있다"가 성립한다.
 * 2. 분류 마커(Ability.Exclusive)는 AssetTags에만 넣는다. 후딜 진입(StartRecovery)이 차단만 풀고 ActivationOwnedTags는 EndAbility까지 남으므로, owner로 올리면 두 진실이 어긋난다.
 * 3. State.X는 어빌리티 활성과 어긋날 수 있는 조건에만 쓴다. 활성과 1:1인 State는 만들지 않는다.
 * 4. 네임스페이스는 발행 주체가 아니라 의미가 정한다. 어빌리티·ANS·GE 중 무엇이 부여하든 조건이면 State.X다.
 */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────

	/** 래그돌 상태. 사망 어빌리티가 서버에서 부여하며, 전 머신의 캐릭터가 감지해 물리 전환. 해제 없이 시체에 유지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Ragdoll);

	/** 락온 피대상 표시. 로컬 플레이어가 이 액터를 락온 중일 때 로컬로만 부여(복제 안 함). 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockedOn);

	/** 전투 상태. 적 AI가 플레이어를 인식하면 서버에서 부여, 추적 종료 시 제거. 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);

	/** 무적 상태. WxAnimNotifyState_Invincible이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);

	/** 방어 유효 상태. 가드 어빌리티가 활성 중 부여하되, SP 고갈로 가드가 깨지면 어빌리티가 도는 중에도 뗀다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guard);

	/** 질주 상태. WxAbility_Sprint가 활성 중 실제로 이동할 때만 부여. SP 소모 GE의 발동 조건이자 SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprint);

	/** 탈진 상태. SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Exhausted);

	/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectGuard);

	/** 슈퍼 아머 상태. 소유 중인 어빌리티는 HitReact로 캔슬되지 않음 (HitReact의 ActivationBlockedTags) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SuperArmor);

	/** 처형(앞잡·뒤잡) 피대상 표시. 연출 동안 WxAbility_Finisher가 대상 ASC에 권위 발행하며, 대상이 이 태그로 자기 처형 어포던스를 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_BeingFinished);

	/** 콤보 입력 수용 구간. WxAnimNotifyState_ComboWindow가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ComboWindow);

	/** 대화 세션 진행 상태. 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행. WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);

	// ── Movement ──────────────────────────────────────────────────────────

	/** 공중 체공 상태. WxCharacterMovementComponent가 낙하 모드 진입·이탈에 맞춰 각 머신에서 부여/제거. 점프 공격의 콤보 세트 진입 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InAir);

	// ── Event ─────────────────────────────────────────────────────────────

	/** 피격 이벤트 부모 카테고리. ExecCalc 필터링용이자 HitReact·Guard 어빌리티가 자식 태그를 모두 수신하기 위해 구독하는 태그 (직접 dispatch 금지 — 자식 없이 오면 일반 피격으로 폴백된다) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	/** 일반 피격 이벤트. HitReact 어빌리티가 NormalHitReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Normal);

	/** 넉백 피격 이벤트. HitReact 어빌리티가 Knockback 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockBack);

	/** 넉다운 피격 이벤트. HitReact 어빌리티가 Knockdown 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockDown);

	/** 넉업 피격 이벤트. HitReact 어빌리티가 Knockup 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockUp);

	/** 패리 피격 이벤트. 공격이 퍼펙트 가드로 막힌 공격자에게 송출, HitReact 어빌리티가 ParryReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Parry);

	/** 피니셔(앞잡) 짝 피격 이벤트. 피니셔 대상 적에게 송출, HitReact 어빌리티가 FinisherHitReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Finisher);

	/** 백스탭(뒤잡) 짝 피격 이벤트. 백스탭 대상 적에게 송출, HitReact 어빌리티가 BackstabHitReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Backstab);

	/** 회피 성공 시 발생하는 이벤트. 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	/** 퍼펙트 가드 성공 시 발생하는 이벤트. Guard 어빌리티가 MP 회복·슬로우 타임·퍼펙트 가드 몽타주를 처리 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	/** 소비 아이템 사용 이벤트. 마시기 몽타주의 노티파이가 송출, UseItem 어빌리티가 수신해 아이템을 실제 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	/** 상호작용 발동 이벤트. 상호작용 입력 시 소유 클라가 선택 대상을 실어 서버에 요청하고, 서버가 플레이어 ASC로 송출(OptionalObject=선택 컴포넌트), WxAbility_Interact가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 피니셔(앞잡) 발동 이벤트. 그로기 적 상호작용 시 플레이어 ASC로 송출(Target=적), WxAbility_Finisher가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	/** 백스탭(뒤잡) 발동 이벤트. 미인지 적 후방 상호작용 시 플레이어 ASC로 송출(Target=적), WxAbility_Finisher가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Backstab);

	/** 사망 발동 이벤트. HP가 0에 닿을 때 AttributeSet이 송출, WxAbility_Death가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	/** 그로기 발동 이벤트. DP가 MaxDP에 닿을 때 AttributeSet이 송출, WxAbility_Groggy가 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	// ── Gimmick ───────────────────────────────────────────────────────────
	// 각 태그는 ST 에셋의 상태에 붙는 라벨(상태 디테일의 Tag 필드)이며, 그 값이 곧
	// 세이브 슬롯에 담기는 기믹의 상태다. 코드가 읽거나 쓰는 값이 아니다.
	//
	// 코드가 안 읽어도 태그는 여기서만 만든다 — 신규 기믹의 상태 이름도 이 파일에 추가한다.

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Close);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtStart);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtEnd);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Unlit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Lit);

	// ── StateTree ─────────────────────────────────────────────────────────

	/**
	 * 상호작용 발동 이벤트. 기믹 컴포넌트가 권위 측에서 상호작용을 받아 전 피어의 ST 에 발행하며, 페이로드로 눌린 영역 메시와 주체를 나른다.
	 * 어느 상태로 갈지는 이 이벤트를 받는 각 상태의 전이가 정한다 — 영역이 여럿이면 전이 조건에서 페이로드의 Source 를 대상 메시와 비교해 가른다.
	 * 상태가 아니라 발동 신호라 Gimmick 계층 밖에 둔다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StateTree_Interact);

	// ── Quest ─────────────────────────────────────────────────────────────

	/** 퀘스트 실패 이벤트. 퀘스트 ST의 InProgress 전이(On Event → Failure)가 수신. 송출은 UWxQuestLibrary::SendQuestEvent 경유 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Fail);

	// ── GameplayCue ──────────────────────────────────────────────────────

	/** 데미지 플로터 Cue. 수치·크리 여부가 서버 판정이라 대미지 확정 후 서버가 발행 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage);

	/** 타격 임팩트 Cue. 대미지 GE가 들고 다녀 공격자 클라에서 예측 재생된다. GameplayCue.Damage의 자식으로 두면 플로터까지 딸려 오므로 형제로 유지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);

	/** 퍼펙트 가드 성공 Cue. 임팩트 위치에 스파크/사운드 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);

	/** Exceed 버프 지속 Cue. 활성 동안 캐릭터의 무기에 Niagara 이펙트를 부착 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	/** 화상 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Burn);

	/** 공격 텔레그래프(선딜 표시) Cue. 색상별로 나뉘며, 순정 GameplayCue (Looping) 노티파이가 몽타주 구간에서 로컬 발행 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);

	// ── Damage ────────────────────────────────────────────────────────────

	/** 치명타 판정 결과 태그 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	/** 치명타 허용 공격 태그. 붙어 있지 않으면 치명타 판정 자체를 건너뛴다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanCritical);

	/** 가드 불가 공격 태그. 가드·퍼펙트 가드를 무시하고 풀 대미지 적용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	/** 패리 피격 유발 공격 태그. 이 공격이 퍼펙트 가드로 막히면 공격자에게 Event.HitReact.Parry 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_ParryHitReact);

	// ── Ability ───────────────────────────────────────────────────────────

	/**
	 * 어빌리티 식별 태그의 루트. 차단·캔슬은 이 루트가 아니라 Ability.Exclusive 만 지목한다.
	 * owner 질의(ActivationBlockedTags·ActivationRequiredTags·GE TagRequirements)에는 쓰지 않는다 — loose 태그는 부모까지 카운트되므로 아무 어빌리티나 돌면 참이 된다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	/**
	 * 액션 슬롯 점유 표식이자 차단(BlockAbilitiesWithTag)·캔슬(CancelAbilitiesWithTag)이 지목하는 유일한 태그.
	 * 이 태그를 가진 어빌리티끼리만 서로 막고 끊으므로, 어빌리티가 서로를 이름으로 참조하지 않는다.
	 * 반응·상태형 어빌리티(피격·그로기·사망·처형·락온)는 이 태그를 갖지 않아 무엇에도 막히거나 끊기지 않는다.
	 *
	 * 식별 태그와 달리 ActivationOwnedTags 에는 넣지 않는다 (파일 상단 규칙 2).
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Exclusive);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseItem);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);

	/** 그로기 상태를 겸한다 — DP가 MaxDP에 닿아 발동하고 DP가 0이 되면 어빌리티가 스스로 끝난다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Groggy);

	/**
	 * 사망 상태를 겸한다 — 사망 어빌리티는 종료하지 않으므로 이 태그가 시체에 그대로 남는다.
	 * 사망 판정(어빌리티 차단·피해 차단·락온 제외·AI 추적 중단 등) 전부가 이 수명에 기대므로 어빌리티를 끝내면 안 된다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Death);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Finisher);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_5);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_6);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_7);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_8);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_9);

	// ── SetByCaller ──────────────────────────────────────────────────────

	/** 지속시간 Duration SetByCaller 키. NoCooldown/InfiniteMP/DrainDP 등 Duration 모디파이어에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** UP 회복량 SetByCaller 키. WxEffect_RecoverResource의 UP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_UP);

	/** MP 회복량 SetByCaller 키. WxEffect_RecoverResource의 MP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_MP);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** 원시 대미지 SetByCaller 키. 양수일 때 ATK/DEF/Coeff/크리를 우회하고 평탄 값으로 대미지 처리 (환경 대미지). 가드/퍼펙트 가드/HitReact는 정상 동작 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_RawDamage);

	/** 이동 속도 배율 SetByCaller 키. WxEffect_MoveSpeedScale이 SPD 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);

	// ── UI ────────────────────────────────────────────────────────────────

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	/** 메뉴 레이어 (인벤토리, 설정 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	/** 모달 레이어 (확인 창 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/** CommonUI 액션: 인벤토리 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Inventory);

	/** CommonUI 액션: 메인 메뉴 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_MainMenu);

	/** CommonUI 액션: 자유 커서 홀드. HUD가 Pressed/Released로 나눠 수신해 입력 모드를 전환한다. 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_FreeCursor);
}
