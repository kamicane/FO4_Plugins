#pragma once

#include "util.hpp"

namespace Internal {
	using BSLM = RE::BGSSaveLoadManager;

	inline const std::regex colon_re(R"(:+)");

	inline std::unordered_map<std::string, std::string> CallerNameMap {};

	template <typename T = RE::TESForm>
	inline T* GetBaseForm(RE::TESObjectREFR* objectRef) {
		if (!objectRef) return nullptr;

		auto* baseObj = objectRef->GetBaseObject();
		if (!baseObj) return nullptr;
		return baseObj->As<T>();
	}

	template <typename T = RE::TESForm>
	inline T* GetForm(RE::TESForm* baseForm) {
		if (!baseForm) return nullptr;
		return baseForm->As<T>();
	}

	template <typename T = RE::TESForm>
	inline T* GetForm(RE::TESFormID formId) {
		auto* baseForm = RE::TESForm::GetFormByID(formId);
		if (!baseForm) return nullptr;
		return baseForm->As<T>();
	}

	inline void ShowNotification (std::string_view message, bool asWarning) {
		if (message.empty()) return;

		std::string localMessage = std::string { message };
		RE::SendHUDMessage::ShowHUDMessage(localMessage.c_str(), nullptr, false, asWarning);
	}

	inline std::string RegisterCallerName (std::string_view scriptName, std::string_view mapName) {
		auto scriptName_s = std::string(scriptName);
		auto mapName_s = std::string(mapName);

		auto sanitized = std::regex_replace(mapName_s, colon_re, "＿");
		CallerNameMap[scriptName_s] = sanitized;
		return sanitized;
	}

	inline void SetPlayerId (std::uint32_t low) {
		auto* bslm = BSLM::GetSingleton();
		auto id = static_cast<std::uint64_t>(low);
		bslm->currentPlayerID = id;
	}

	inline std::uint32_t GetPlayerId () {
		auto* bslm = BSLM::GetSingleton();

		auto id = bslm->currentPlayerID;
		auto low = static_cast<std::uint32_t>(id & 0xFFFFFFFFULL);

		return low;
	}

	inline std::uint32_t EnsurePlayerId () {
		auto playerId = Internal::GetPlayerId();
		if (playerId == 0) {
			playerId = Util::RandomUInt32();
			Internal::SetPlayerId(playerId);
			spdlog::warn("Player Id was 0. New Player Id: {}", playerId);
		}

		return playerId;
	}

	inline std::string GetCallerNameRaw (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId) {
		RE::BSTSmartPointer<RE::BSScript::Stack> stack;
		static std::string fallback = "Unknown";

		auto* vmInternal = reinterpret_cast<RE::BSScript::Internal::VirtualMachine*>(&vm);
		if (!vmInternal) {
			return fallback;
		}

		if (!vmInternal->GetStackByID(stackId, stack) || !stack || !stack->top) {
			return fallback;
		}

		auto* frame = stack->top;
		if (!frame) {
			return fallback;
		}

		auto* caller = frame->previousFrame;
		if (caller && caller->owningObjectType) {
			std::string name = caller->owningObjectType->GetName();
			if (!name.empty()) return name;
		}

		return fallback;
	}

	inline std::string GetCallerName (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId) {
		auto scriptName = GetCallerNameRaw(vm, stackId);
		auto it = CallerNameMap.find(scriptName);
		if (it == CallerNameMap.end()) {
			return RegisterCallerName(scriptName, scriptName);
		}
		return it->second;
	}

	inline std::string ToString (RE::TESForm* form) {
		if (form) {
			std::string formType = RE::TESForm::GetFormTypeString(form->GetFormType());
			auto nameOpt = RE::TESFullName::GetFullName(form);
			auto name = std::string(nameOpt ? *nameOpt : "");
			return fmt::format("<{}:{} {:08X} '{}'>", formType, form->GetFormEditorID(), form->GetFormID(), name);
		}

		return "<None>";
	}

	inline std::string ToString (RE::TESObjectREFR* objectRef) {
		if (objectRef) {
			auto baseStr = ToString(objectRef->GetBaseObject());
			std::string name;

			if (auto* extraList = objectRef->extraList.get()) {
				if (auto* xText = extraList->GetByType<RE::ExtraTextDisplayData>()) {
					name = std::string(xText->displayName);
				}
			}

			// if (name.empty()) {
			// 	auto nameOpt = RE::TESFullName::GetFullName(objectRef->GetBaseObject());
			// 	name = nameOpt ? *nameOpt : "";
			// }

			return fmt::format("<REFR{} {:08X} '{}'>", baseStr, objectRef->GetFormID(), name);
		}

		return "<None>";
	}

	inline std::string ToString (const RE::BSScript::Variable& var) {
		auto rawType = var.GetType().GetRawType();

		switch (rawType) {
			case RE::BSScript::TypeInfo::RawType::kString:
				return RE::BSScript::get<RE::BSFixedString>(var).c_str();
			case RE::BSScript::TypeInfo::RawType::kFloat:
				return std::to_string(RE::BSScript::get<float>(var));
			case RE::BSScript::TypeInfo::RawType::kInt:
				return std::to_string(RE::BSScript::get<std::int32_t>(var));
			case RE::BSScript::TypeInfo::RawType::kNone:
				return "<None>";
			case RE::BSScript::TypeInfo::RawType::kBool:
				return RE::BSScript::get<bool>(var) ? "true" : "false";
			case RE::BSScript::TypeInfo::RawType::kObject: {
				auto scriptObj = RE::BSScript::get<RE::BSScript::Object>(var);
				if (scriptObj) {
					if (auto* objectRef = scriptObj->Resolve<RE::TESObjectREFR>()) {
						return ToString(objectRef);
					}
					if (auto* form = scriptObj->Resolve<RE::TESForm>()) {
						return ToString(form);
					}
				}
				return "<Object>";
			}
			case RE::BSScript::TypeInfo::RawType::kStruct:
				return "<Struct>";
			case RE::BSScript::TypeInfo::RawType::kVar:
				return "<Var>";
			case RE::BSScript::TypeInfo::RawType::kArrayObject:
				return "<Array[object]>";
			case RE::BSScript::TypeInfo::RawType::kArrayString:
				return "<Array[string]>";
			case RE::BSScript::TypeInfo::RawType::kArrayInt:
				return "<Array[int]>";
			case RE::BSScript::TypeInfo::RawType::kArrayFloat:
				return "<Array[float]>";
			case RE::BSScript::TypeInfo::RawType::kArrayBool:
				return "<Array[bool]>";
			case RE::BSScript::TypeInfo::RawType::kArrayVar:
				return "<Array[var]>";
			case RE::BSScript::TypeInfo::RawType::kArrayStruct:
				return "<Array[struct]>";
			default:
				return "<Unknown>";
		}
	}

	template<class... VarPtrs>
	inline std::string Format (std::string_view fmtString, VarPtrs... vars) {
		fmt::dynamic_format_arg_store<fmt::format_context> store;
		const std::array<const RE::BSScript::Variable*, sizeof...(vars)> args { vars... };
		for (const auto* varPtr : args) {
			RE::BSScript::Variable none;
			const auto& varVal = varPtr ? *varPtr : none;

			switch (varVal.GetType().GetRawType()) {
				case RE::BSScript::TypeInfo::RawType::kInt:
					store.push_back(RE::BSScript::get<std::int32_t>(varVal));
					break;
				case RE::BSScript::TypeInfo::RawType::kString:
					store.push_back(std::string(RE::BSScript::get<RE::BSFixedString>(varVal).c_str()));
					break;
				case RE::BSScript::TypeInfo::RawType::kBool:
					store.push_back(RE::BSScript::get<bool>(varVal));
					break;
				case RE::BSScript::TypeInfo::RawType::kFloat:
					store.push_back(RE::BSScript::get<float>(varVal));
					break;
				default:
					store.push_back(ToString(varVal));
					break;
			}
		}

		return fmt::vformat(fmtString, fmt::basic_format_args<fmt::format_context>(store));
	}

	inline bool SaveGame (
		std::string_view saveName,
		std::string_view successMessage,
		std::string_view failureMessage,
		std::function<void(bool)> callback = nullptr
	) {
		auto playerId = Internal::EnsurePlayerId();

		auto sanitized = Util::SanitizeSaveName(saveName);
		auto finalName = fmt::format("{} [_{:08X}]", sanitized, playerId);

		F4SE::GetTaskInterface()->AddTask(
			[saveName = std::string(saveName),
			 successMessage = std::string(successMessage),
			 failureMessage = std::string(failureMessage),
			 finalName = std::move(finalName),
			 callback = std::move(callback)] () mutable {
				auto* bslm = RE::BGSSaveLoadManager::GetSingleton();
				bslm->BufferSceneScreenShot();

				const bool wasSaved = bslm->SaveGame(finalName.c_str(), 0, 0, false);

				if (wasSaved && !successMessage.empty()) {
					auto fmtSuccessMessage = fmt::format(fmt::runtime(successMessage), saveName);
					ShowNotification(fmtSuccessMessage, false);
				}

				if (!wasSaved && !failureMessage.empty()) {
					auto fmtFailureMessage = fmt::format(fmt::runtime(failureMessage), saveName);
					ShowNotification(fmtFailureMessage, true);
				}

				if (callback) callback(wasSaved);
			}
		);

		return true;
	}

	inline std::string GetCurrentLocationName () {
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return {};
		}

		auto* cell = player->GetParentCell();

		if (cell && cell->IsInterior()) {
			auto cellStrOpt = RE::TESFullName::GetFullName(cell);
			if (cellStrOpt && !cellStrOpt->empty()) {
				spdlog::debug("Cell name is {}", *cellStrOpt);
				return std::string(*cellStrOpt);
			}
		}

		auto* loc = player->GetCurrentLocation();

		if (loc) {
			auto locStrOpt = RE::TESFullName::GetFullName(loc);
			if (locStrOpt && !locStrOpt->empty()) {
				spdlog::debug("Location name is {}", *locStrOpt);
				return std::string(*locStrOpt);
			}
		}

		return {};
	}

	inline std::string GetPlayerName () {
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			spdlog::warn("GetPlayerName: PlayerCharacter::GetSingleton returned null");
			return {};
		}

		std::string playerName = player->GetDisplayFullName();
		if (playerName.empty()) {
			spdlog::warn("GetPlayerName: GetDisplayFullName returned empty string");
			return {};
		}

		return playerName;
	}

	template<class... Args>
	bool CallGlobalFunctionNoWait (std::string_view scriptName, std::string_view functionName, Args&&... args) {
		auto* gameVM = RE::GameVM::GetSingleton();
		auto vm = gameVM ? gameVM->GetVM() : nullptr;
		if (!vm) return false;

		RE::BSFixedString scriptNameBS(scriptName);
		RE::BSFixedString functionNameBS(functionName);

		return vm->DispatchStaticCall(scriptNameBS, functionNameBS, nullptr, std::forward<Args>(args)...);
	}

	template<class... Args>
	bool CallFunctionNoWait (RE::TESForm* self, std::string_view scriptName, std::string_view functionName, Args&&... args) {
		if (!self) return false;

		auto* gameVM = RE::GameVM::GetSingleton();
		auto vm = gameVM ? gameVM->GetVM() : nullptr;
		if (!vm) return false;

		RE::BSFixedString scriptNameBS(scriptName);
		RE::BSFixedString functionNameBS(functionName);

		auto& handlePolicy = vm->GetObjectHandlePolicy();
		auto objectTypeID = static_cast<std::uint32_t>(self->GetFormType());
		auto objectHandle = handlePolicy.GetHandleForObject(objectTypeID, self);
		if (objectHandle == handlePolicy.EmptyHandle()) return false;

		RE::BSTSmartPointer<RE::BSScript::Object> boundObj;
		if (!vm->FindBoundObject(objectHandle, scriptNameBS.c_str(), false, boundObj, false) || !boundObj) {
			return false;
		}

		return vm->DispatchMethodCall(boundObj, functionNameBS, nullptr, std::forward<Args>(args)...);
	}
}
