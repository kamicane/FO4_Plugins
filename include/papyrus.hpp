#pragma once

#include "internal.hpp"
#include "util.hpp"
#include "menu-text-input.hpp"

namespace Papyrus {

	inline RE::BGSSaveLoadFileEntry g_load_entry {};
	inline std::string g_load_name;

	inline void Test (std::monostate mono) {
		spdlog::debug("TEST THIS SHIT");
	}

	inline void GetSettings (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		const RE::BSScript::Variable* structVar
	) {
		spdlog::debug("GetSettings");
		auto name = Internal::GetCallerName(vm, stackId);
		spdlog::debug("GetSettings called ({}): section={}", name, section);

		if (name.empty()) return;
		if (!structVar) return;
		if (!structVar->is<RE::BSScript::Struct>()) return;

		auto structValue = RE::BSScript::get<RE::BSScript::Struct>(*structVar);
		if (!structValue || !structValue->type) return;

		for (const auto& field : structValue->type->varNameIndexMap) {
			auto fieldName = field.first;
			auto fieldIndex = field.second;
			if (fieldIndex >= structValue->type->variables.size()) continue;

			auto fieldType = structValue->type->variables[fieldIndex].varType.GetRawType();
			auto& fieldValue = structValue->variables[fieldIndex]; // NOLINT

			switch (fieldType) {
				case RE::BSScript::TypeInfo::RawType::kString: {
					auto defaultValue = RE::BSScript::get<RE::BSFixedString>(fieldValue);

					auto value = Util::GetIniString(name, section, fieldName.c_str(), defaultValue.c_str());
					RE::BSScript::PackVariable(fieldValue, RE::BSFixedString(value.c_str()));
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kInt: {
					auto defaultValue = RE::BSScript::get<std::int32_t>(fieldValue);

					auto value = Util::GetIniInt(name, section, fieldName.c_str(), defaultValue);
					RE::BSScript::PackVariable(fieldValue, value);
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kFloat: {
					auto defaultValue = RE::BSScript::get<float>(fieldValue);

					auto value = Util::GetIniFloat(name, section, fieldName.c_str(), defaultValue);
					RE::BSScript::PackVariable(fieldValue, value);
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kBool: {
					auto defaultValue = RE::BSScript::get<bool>(fieldValue);

					auto value = Util::GetIniBool(name, section, fieldName.c_str(), defaultValue);
					RE::BSScript::PackVariable(fieldValue, value);
					break;
				}
				default:
					break;
			}
		}
	}

	inline void SetSettings (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		const RE::BSScript::Variable* structVar
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		spdlog::debug("SetSettings called ({}): section={}", name, section);

		if (name.empty()) return;
		if (!structVar) return;
		if (!structVar->is<RE::BSScript::Struct>()) return;

		auto structValue = RE::BSScript::get<RE::BSScript::Struct>(*structVar);
		if (!structValue || !structValue->type) return;

		auto ini = Util::GetIniPtr(name);
		auto section_s = std::string(section);

		for (const auto& field : structValue->type->varNameIndexMap) {
			auto fieldName = field.first;
			auto fieldIndex = field.second;
			if (fieldIndex >= structValue->type->variables.size()) continue;

			auto fieldType = structValue->type->variables[fieldIndex].varType.GetRawType();
			auto& fieldValue = structValue->variables[fieldIndex]; // NOLINT

			auto key_s = std::string(fieldName.c_str());

			switch (fieldType) {
				case RE::BSScript::TypeInfo::RawType::kString: {
					auto value = RE::BSScript::get<RE::BSFixedString>(fieldValue);

					ini->SetValue(section_s.c_str(), key_s.c_str(), value.c_str());
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kInt: {
					auto value = RE::BSScript::get<std::int32_t>(fieldValue);

					auto value_s = std::to_string(value);
					ini->SetValue(section_s.c_str(), key_s.c_str(), value_s.c_str());
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kFloat: {
					auto value = RE::BSScript::get<float>(fieldValue);

					auto value_s = std::to_string(value);
					ini->SetValue(section_s.c_str(), key_s.c_str(), value_s.c_str());
					break;
				}
				case RE::BSScript::TypeInfo::RawType::kBool: {
					auto value = RE::BSScript::get<bool>(fieldValue);

					ini->SetValue(section_s.c_str(), key_s.c_str(), value ? "true" : "false");
					break;
				}
				default:
					break;
			}
		}
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

		RE::SendHUDMessage::ShowHUDMessage(fmtString.c_str(), nullptr, false, false);
	}

	std::string Format (
		std::monostate mono,
		std::string_view fmtStr,
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
		auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);
		return fmtString;
	}

	inline void ConsoleLog (
		std::monostate mono,
		std::string_view fmtStr,
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
		auto* console = RE::ConsoleLog::GetSingleton();
		if (!console) return;

		auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);

		auto consoleStr = fmtString + "\n";
		console->AddString(consoleStr.c_str());
	}

	inline void LogI (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view fmtStr,
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
		auto scriptName = Internal::GetCallerName(vm, stackId);
		auto logger = Util::GetLogger(scriptName);
		if (logger && logger->level() <= spdlog::level::info) {
			auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);
			logger->info("{}", fmtString);
		}
	}

	inline void LogW (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view fmtStr,
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
		auto scriptName = Internal::GetCallerName(vm, stackId);
		auto logger = Util::GetLogger(scriptName);
		if (logger && logger->level() <= spdlog::level::warn) {
			auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);
			logger->warn("{}", fmtString);
		}
	}

	inline void LogD (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view fmtStr,
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
		auto scriptName = Internal::GetCallerName(vm, stackId);
		auto logger = Util::GetLogger(scriptName);
		if (logger && logger->level() <= spdlog::level::debug) {
			auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);
			logger->debug("{}", fmtString);
		}
	}

	inline void LogE (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view fmtStr,
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
		auto scriptName = Internal::GetCallerName(vm, stackId);
		auto logger = Util::GetLogger(scriptName);
		if (logger && logger->level() <= spdlog::level::err) {
			auto fmtString = Internal::Format(fmtStr, var1, var2, var3, var4, var5, var6, var7, var8, var9);
			logger->error("{}", fmtString);
		}
	}

	inline std::string GetCallerName (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackId, std::monostate mono) {
		auto name = Internal::GetCallerName(vm, stackId);
		spdlog::debug("[GetCallerName] Caller name is {}", name);
		return name;
	}

	inline std::string Register (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view mapName
	) {
		auto scriptName = Internal::GetCallerNameRaw(vm, stackId);
		if (scriptName.empty() || scriptName == "Unknown") return {};
		return Internal::RegisterCallerName(scriptName, mapName);
	}

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
		spdlog::info("[SetPlayerId]: new player id is {:08X}", playerId);

		return fmt::format("{:08X}", playerId);
	}

	inline bool UpdatePowerArmor3d (std::monostate mono, RE::TESObjectREFR* ref) {
		if (!ref) return false;
		RE::PowerArmor::SyncFurnitureVisualsToInventory(ref, true, nullptr, false);
		return true;
	}

	// no workie booo
	// bool IsSavingAllowed (std::monostate mono) {
	// 	auto bslm = BSLM::GetSingleton();
	// 	logger->debug("IsSaveAllowed called : {}", bslm->savingAllowed);
	// 	return bslm->savingAllowed;
	// }

	inline void QueueForceSave (std::monostate mono) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();

		bslm->BufferSceneScreenShot();
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kForceSave);
	}

	inline void QueueAutoSave (std::monostate mono) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();

		bslm->BufferSceneScreenShot();
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kAutoSave);
	}

	inline void QueueLoadGame (std::monostate mono, std::string_view saveName) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();

		auto playerId = Internal::EnsurePlayerId();
		auto sanitizedSaveName = Util::SanitizeSaveName(saveName);
		g_load_name = fmt::format("{} [_{:08X}]", sanitizedSaveName, playerId);

		g_load_entry.fileName = g_load_name.c_str();

		bslm->queuedEntryToLoad = &g_load_entry;
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kLoadGame);
	}

	inline void QueueSaveAndQuitToDesktop (std::monostate mono) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();
		bslm->BufferSceneScreenShot();
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kSaveAndQuitToDesktop);
	}

	inline void QueueLoadLastSave (std::monostate mono) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kLoadMostRecentSave);
	}

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

	inline bool SaveGame (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view saveName,
		std::string_view successMessage,
		std::string_view failureMessage
	) {
		auto fmtSuccessMessage = fmt::format(fmt::runtime(successMessage), saveName);
		auto fmtFailureMessage = fmt::format(fmt::runtime(failureMessage), saveName);

		Internal::SaveGame(saveName, fmtSuccessMessage, fmtFailureMessage, [vmPtr = &vm, stackId] (bool wasSaved) {
			RE::BSScript::Variable ret;
			RE::BSScript::PackVariable(ret, wasSaved);
			if (vmPtr) {
				vmPtr->ReturnFromLatent(stackId, ret);
			}
		});

		return true;
	}

	// needs proper implementation BuildSaveGameList(). currently only AE.
	// inline void ListSaves (std::monostate mono)

	inline bool LoadSettings (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackID, std::monostate mono) {
		auto name = Internal::GetCallerName(vm, stackID);
		spdlog::debug("LoadSettings ({})", name);

		if (name.empty()) return false;

		Util::LoadIni(name);
		return true;
	}

	inline bool SaveSettings (RE::BSScript::IVirtualMachine& vm, std::uint32_t stackID, std::monostate mono) {
		auto name = Internal::GetCallerName(vm, stackID);
		spdlog::debug("SaveSettings ({})", name);

		if (name.empty()) return false;

		Util::SaveIni(name);
		return true;
	}

	inline std::string GetSettingString (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackID,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		std::string_view defaultValue
	) {
		auto name = Internal::GetCallerName(vm, stackID);
		if (name.empty()) return {};

		return Util::GetIniString(name, section, key, defaultValue);
	}

	inline bool GetSettingBool (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackID,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		bool defaultValue
	) {
		auto name = Internal::GetCallerName(vm, stackID);
		if (name.empty()) return {};

		return Util::GetIniBool(name, section, key, defaultValue);
	}

	inline int32_t GetSettingInt (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackID,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		int32_t defaultValue,
		int32_t minValue,
		int32_t maxValue
	) {
		auto name = Internal::GetCallerName(vm, stackID);
		if (name.empty()) return {};

		return Util::GetIniInt(name, section, key, defaultValue, minValue, maxValue);
	}

	inline float GetSettingFloat (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackID,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		float defaultValue,
		float minValue,
		float maxValue
	) {
		auto name = Internal::GetCallerName(vm, stackID);
		if (name.empty()) return {};

		return Util::GetIniFloat(name, section, key, defaultValue, minValue, maxValue);
	}

	inline void SetSettingBool (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		bool value
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		if (name.empty()) return;
		Util::SetIniBool(name, section, key, value);
	}

	inline void SetSettingString (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		std::string_view value
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		if (name.empty()) return;
		Util::SetIniString(name, section, key, value);
	}

	inline void SetSettingInt (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		int32_t value
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		if (name.empty()) return;
		Util::SetIniInt(name, section, key, value);
	}

	inline void SetSettingFloat (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		std::string_view key,
		float value
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		if (name.empty()) return;
		Util::SetIniFloat(name, section, key, value);
	}
}
