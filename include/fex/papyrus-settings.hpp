#pragma once

#include "util.hpp"
#include "internal.hpp"

namespace Papyrus::Settings {
	inline void GetSettingsStruct (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		const RE::BSScript::Variable* structVar
	) {
		spdlog::debug("GetSettingsStruct");

		auto name = Internal::GetCallerName(vm, stackId);
		spdlog::debug("GetSettingsStruct called ({}): section={}", name, section);

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

	inline void SetSettingsStruct (
		RE::BSScript::IVirtualMachine& vm,
		std::uint32_t stackId,
		std::monostate mono,
		std::string_view section,
		const RE::BSScript::Variable* structVar
	) {
		auto name = Internal::GetCallerName(vm, stackId);
		spdlog::debug("SetSettingsStruct called ({}): section={}", name, section);

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
