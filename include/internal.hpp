#pragma once

#include "util.hpp"

namespace Internal {
	using BSLM = RE::BGSSaveLoadManager;

	inline std::unordered_map<std::string, std::string> g_caller_name_map {};

	inline void ShowHUDNotification (std::string_view message) {
		if (message.empty()) return;

		std::string localMessage = std::string { message };
		RE::SendHUDMessage::ShowHUDMessage(localMessage.c_str(), nullptr, false, false);
	}

	inline std::string RegisterCallerName (std::string_view scriptName, std::string_view mapName) {
		auto sanitized = Util::SanitizeFileName(mapName);
		g_caller_name_map[std::string(scriptName)] = sanitized;
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
			const char* name = caller->owningObjectType->GetName();
			return name ? std::string { name } : fallback;
		}

		return fallback;
	}

	inline std::string GetCallerName (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId) {
		auto scriptName = GetCallerNameRaw(vm, stackId);
		auto it = g_caller_name_map.find(scriptName);
		if (it == g_caller_name_map.end()) {
			it = g_caller_name_map.emplace(scriptName, Util::SanitizeFileName(scriptName)).first;
		}
		return it->second;
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
			case RE::BSScript::TypeInfo::RawType::kObject:
				return "<Object>";
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
					ShowHUDNotification(fmtSuccessMessage);
				}

				if (!wasSaved && !failureMessage.empty()) {
					auto fmtFailureMessage = fmt::format(fmt::runtime(failureMessage), saveName);
					ShowHUDNotification(fmtFailureMessage);
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
// 		auto* gameVM = RE::GameVM::GetSingleton();
// 		auto vm = gameVM ? gameVM->GetVM() : nullptr;
// 		if (!vm) return false;

// 		auto scriptNameStr = std::string(scriptName);
// 		auto functionNameStr = std::string(functionName);
// 		RE::BSFixedString scriptNameBS(scriptNameStr.c_str());
// 		RE::BSFixedString functionNameBS(functionNameStr.c_str());

// #if GAME_VERSION == 1
// 		auto packedArgs = RE::BSScript::detail::FunctionArgs { vm.get(), std::forward<Args>(args)... };
// 		return vm
// 			->DispatchStaticCall(scriptNameBS, functionNameBS, RE::BSScript::detail::CreateThreadScrapFunction(packedArgs), nullptr);
// #else
// 		return vm->DispatchStaticCall(scriptNameBS, functionNameBS, nullptr, std::forward<Args>(args)...);
// #endif
	}

	template<class... Args>
	bool CallFunctionNoWait (RE::TESForm* self, std::string_view scriptName, std::string_view functionName, Args&&... args) {
		if (!self) return false;

		auto* gameVM = RE::GameVM::GetSingleton();
		auto vm = gameVM ? gameVM->GetVM() : nullptr;
		if (!vm) return false;

		auto scriptNameStr = std::string(scriptName);
		auto functionNameStr = std::string(functionName);
		RE::BSFixedString scriptNameBS(scriptNameStr.c_str());
		RE::BSFixedString functionNameBS(functionNameStr.c_str());

		auto& handlePolicy = vm->GetObjectHandlePolicy();
		auto objectTypeID = static_cast<std::uint32_t>(self->GetFormType());
		auto objectHandle = handlePolicy.GetHandleForObject(objectTypeID, self);
		if (objectHandle == handlePolicy.EmptyHandle()) return false;

		return vm->DispatchMethodCall(objectHandle, scriptNameBS, functionNameBS, nullptr, std::forward<Args>(args)...);
	}
}
