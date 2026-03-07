#pragma once

#include "internal.hpp"
#include "util.hpp"
#include "menu-text-input.hpp"
#include "events-viewcaster.hpp"

namespace Papyrus {

	inline void TestSearchRefs (std::monostate mono, RE::TESObjectREFR* origin, float range) {
		spdlog::debug("[Papyrus:testSearchRefs] called origin={} range={}", Internal::ToString(origin), range);
		if (!origin) return;

		auto* tes = RE::TES::GetSingleton();
		if (!tes) return;

		tes->ForEachRefInRange(origin, range, [&] (RE::TESObjectREFR* objectRef) {
			if (objectRef) spdlog::debug("[Papyrus:testSearchRefs] ref {}", Internal::ToString(objectRef));
			return RE::BSContainer::ForEachResult::kContinue;
		});
	}

	inline void ShowMessage (
		std::monostate mono,
		std::string_view fmtMessage,
		const RE::BSScript::Variable* var1,
		const RE::BSScript::Variable* var2,
		const RE::BSScript::Variable* var3,
		const RE::BSScript::Variable* var4,
		const RE::BSScript::Variable* var5,
		const RE::BSScript::Variable* var6,
		const RE::BSScript::Variable* var7,
		const RE::BSScript::Variable* var8,
		const RE::BSScript::Variable* var9
	) {
		std::string fmtString = Internal::Format(fmtMessage, var1, var2, var3, var4, var5, var6, var7, var8, var9);

		Internal::ShowNotification(fmtString, false);
	}

	inline void ShowWarning (
		std::monostate mono,
		std::string_view fmtMessage,
		const RE::BSScript::Variable* var1,
		const RE::BSScript::Variable* var2,
		const RE::BSScript::Variable* var3,
		const RE::BSScript::Variable* var4,
		const RE::BSScript::Variable* var5,
		const RE::BSScript::Variable* var6,
		const RE::BSScript::Variable* var7,
		const RE::BSScript::Variable* var8,
		const RE::BSScript::Variable* var9
	) {
		std::string fmtString = Internal::Format(fmtMessage, var1, var2, var3, var4, var5, var6, var7, var8, var9);

		Internal::ShowNotification(fmtString, true);
	}

	RE::TESObjectREFR* GetCrosshairRef (std::monostate mono) {
		auto formId = Events::GetViewcasterFormId();
		if (!formId) return nullptr;

		auto* form = RE::TESForm::GetFormByID(formId);
		if (!form) return nullptr;

		return form->As<RE::TESObjectREFR>();
	}

	inline std::string TestGetCallerName (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId, std::monostate mono) {
		auto name = Internal::GetCallerName(vm, stackId);
		return name;
	}

	// inline std::string Register (
	// 	RE::BSScript::IVirtualMachine& vm,
	// 	std::uint32_t stackId,
	// 	std::monostate mono,
	// 	std::string_view mapName
	// ) {
	// 	auto scriptName = Internal::GetCallerNameRaw(vm, stackId);
	// 	if (scriptName.empty() || scriptName == "Unknown") return {};
	// 	return Internal::RegisterCallerName(scriptName, mapName);
	// }

	inline std::string GetPlayerId (std::monostate mono) {
		return fmt::format("{:08X}", Internal::GetPlayerId());
	}

	inline std::string SetPlayerId (std::monostate mono, std::string_view playerIdHex) {
		std::uint32_t playerId = 0;

		if (playerIdHex.empty()) {
			playerId = Util::RandomUInt32();
		} else {
			playerId = Util::Uint32FromHex(playerIdHex);
			if (playerId == 0) {
				spdlog::error("[SetPlayerId]: invalid player id provided: {}", playerIdHex);

				playerId = Util::RandomUInt32();
			}
		}

		Internal::SetPlayerId(playerId);
		spdlog::debug("[SetPlayerId]: new player id is {:08X}", playerId);

		return fmt::format("{:08X}", playerId);
	}

	inline bool UpdatePowerArmor3d (std::monostate mono, RE::TESObjectREFR* ref) {
		if (!ref) return false;
		RE::PowerArmor::SyncFurnitureVisualsToInventory(ref, true, nullptr, false);
		return true;
	}

	inline RE::TESForm* GetFormByEditorId (std::monostate m, std::string_view editorId) {
		return RE::TESForm::GetFormByEditorID(RE::BSFixedString(editorId));
	}

	inline bool TestTransferItems (std::monostate mono, RE::TESObjectREFR* src, RE::TESObjectREFR* dst, RE::BGSKeyword* keyword) {
		spdlog::debug("[Papyrus:TransferItems] called");
		if (!src || !dst || !keyword) return false;

		struct Entry {
			RE::TESBoundObject* base {};
			std::uint32_t stackIndex {};
			std::uint32_t count {};
			RE::BSTSmartPointer<RE::ExtraDataList> extra;
		};

		if (!src->inventoryList) return false;

		std::unordered_map<RE::BGSInventoryItem*, std::vector<Entry>> groups;

		// collect all matching stacks under read lock
		{
			const auto& inv = src->inventoryList;
			RE::BSAutoReadLock readLock { inv->rwLock };

			for (auto& it : inv->data) {
				if (!it.object) continue;

				auto* base = it.object;
				bool baseHasKeyword = false;
				if (auto* baseKwd = base->As<RE::BGSKeywordForm>()) {
					baseHasKeyword = baseKwd->HasKeyword(keyword);
				}
				if (!baseHasKeyword) continue;

				RE::BGSInventoryItem* itemPtr = std::addressof(it);
				RE::BGSInventoryItem::Stack* stack = it.stackData.get();
				std::uint32_t idx = 0;
				while (stack) {
					if (stack->flags.any(RE::BGSInventoryItem::Stack::Flags::kEquipStateLocked)) {
						++idx;
						stack = stack->nextStack.get();
						continue;
					}

					Entry e;
					e.base = base;
					e.stackIndex = idx;
					e.count = stack->GetCount();
					e.extra = stack->extra;

					groups[itemPtr].push_back(e);

					++idx;
					stack = stack->nextStack.get();
				}
			}
		}

		// remove stacks per item in descending index order to avoid shifting
		for (auto& [itemPtr, list] : groups) {
			std::sort(list.begin(), list.end(), [] (const Entry& a, const Entry& b) {
				return a.stackIndex > b.stackIndex;
			});
			for (auto& e : list) {
				RE::TESObjectREFR::RemoveItemData removeData(e.base, static_cast<std::int32_t>(e.count));
				removeData.stackData.push_back(e.stackIndex);
				removeData.a_otherContainer = dst;

				src->RemoveItem(removeData);

				dst->AddInventoryItem(e.base, e.extra, e.count, src, nullptr, nullptr);

				dst->SendContainerChangedEvent(src, dst, e.base, static_cast<std::int32_t>(e.count), e.base->formID, 0);
			}
		}

		return true;
	}

	// bool IsSavingAllowed (std::monostate mono) {
	//  auto bslm = BSLM::GetSingleton();
	//  logger->debug("IsSaveAllowed called : {}", bslm->savingAllowed);
	//  return bslm->savingAllowed;
	// }

	inline bool OpenTextInputMenu_ (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId, std::monostate mono) {
		spdlog::debug("OpenTextInputMenu_ called");

		// Events::TextInput.once(Events::TextInputType::AcceptCancel, [vmPtr = &vm, stackId] (std::string_view textValue) {
		// 	if (vmPtr) {
		// 		const bool waiting = vmPtr->IsWaitingOnLatent(stackId);
		// 		spdlog::debug("TextInputMenuSink::ProcessEvent closing: stackID={}, waitingOnLatent={}", stackId, waiting);

		// 		RE::BSScript::Variable ret;
		// 		RE::BSScript::PackVariable(ret, true);
		// 		vmPtr->ReturnFromLatent(stackId, ret);
		// 	}

		// 	return true;
		// });

		Menu::TextInput::Open("foo", "bar");

		return true;
	}

	inline std::string GetLastTextInputResult_ (std::monostate mono) {
		return {};
	}

	inline void TestPrintInventory (std::monostate mono, RE::TESObjectREFR* a_ref, RE::BGSKeyword* a_keyword) {
		spdlog::debug("[PrintInventory] called");
		if (!a_ref) return;

		auto* inv = a_ref->inventoryList;
		if (!inv) {
			spdlog::debug("[PrintInventory] no inventory");
			return;
		}

		const auto lock = RE::BSAutoReadLock { inv->rwLock };

		for (auto& item : inv->data) {
			auto* base = item.object;

			bool baseHasKeyword = false;
			if (a_keyword && base) {
				if (auto* baseKwd = base->As<RE::BGSKeywordForm>()) {
					baseHasKeyword = baseKwd->HasKeyword(a_keyword);
				}
			}

			spdlog::debug("[PrintInventory] {} hasKeyword={}", Internal::ToString(base), baseHasKeyword);

			std::uint32_t stackIndex = 0;
			for (auto* stack = item.stackData.get(); stack; stack = stack->nextStack.get(), ++stackIndex) {
				const char* name = item.GetDisplayFullName(stackIndex);
				auto count = stack->GetCount();

				// bool extraListHasKeyword = false;
				bool isLegendary = false;

				if (a_keyword && stack->extra) {
					if (auto* extraList = stack->extra.get()) {
						if (extraList->GetLegendaryMod()) isLegendary = true;

						// if (auto* extraKeywords = extraList->GetByType<RE::ExtraKeywords>()) {
						// 	for (auto keyword : extraKeywords->extraKeywords) {
						// 		if (keyword == a_keyword) {
						// 			extraListHasKeyword = true;
						// 			break;
						// 		}
						// 	}
						// }
					}
				}

				bool instanceDataHasKeyword = false;
				if (a_keyword) {
					if (auto* inst = item.GetInstanceData(stackIndex)) {
						if (auto* instKwd = inst->GetKeywordData()) {
							if (instKwd->HasKeyword(a_keyword)) {
								instanceDataHasKeyword = true;
							}
						}
					}
				}

				bool equipLocked = stack->flags.any(RE::BGSInventoryItem::Stack::Flags::kEquipStateLocked);

				bool isQuest = item.IsQuestObject(static_cast<std::int32_t>(stackIndex));

				spdlog::debug(
					"[PrintInventory] {} x{} instanceDataHasKeyword={} equipLocked={} legendary={} quest={}",
					name ? name : "<unnamed>",
					count,
					instanceDataHasKeyword,
					equipLocked,
					isLegendary,
					isQuest
				);
			}
		}
	}
}
