#pragma once

#include "project.h"
#include "internal.hpp"
#include "load-order.hpp"

namespace LoadHandler {

	using OnLoadGameKey = std::pair<LoadOrder::FormKey, std::string>;

	struct OnLoadGameKeyHash {
		std::size_t operator() (const OnLoadGameKey& key) const {
			const auto h1 = std::hash<std::string> {}(std::string(key.first));
			const auto h2 = std::hash<std::string> {}(key.second);
			return h1 ^ (h2 << 1);
		}
	};

	struct OnLoadGameKeyEqual {
		bool operator() (const OnLoadGameKey& lhs, const OnLoadGameKey& rhs) const {
			return lhs.first.id == rhs.first.id && lhs.first.modKey == rhs.first.modKey && lhs.second == rhs.second;
		}
	};

	inline std::unordered_map<OnLoadGameKey, std::string, OnLoadGameKeyHash, OnLoadGameKeyEqual> g_onLoadGameFunctionMap {};

	inline void LoadPluginIniFiles () {
		auto iniDir = Util::GetGameDir() / "Data" / Project::ID;
		if (!std::filesystem::exists(iniDir)) return;

		auto* idNameMap = LoadOrder::GetIDNameFileMap();
		for (const auto& [fileID, pluginName] : *idNameMap) {
			auto pluginStem = std::filesystem::path(pluginName).stem().string();
			auto iniName = pluginStem + ".ini";
			auto iniPath = iniDir / iniName;
			if (!std::filesystem::exists(iniPath)) continue;

			CSimpleIniA ini;
			ini.SetUnicode(true);
			if (ini.LoadFile(iniPath.string().c_str()) < 0) continue;

			spdlog::debug("[LoadPluginIniFiles] Found ini file {}", iniName);

			const auto scriptName = Util::Trim(ini.GetValue("OnLoadGame", "Script", ""));
			const auto functionName = Util::Trim(ini.GetValue("OnLoadGame", "Function", ""));

			if (scriptName.empty()) {
				spdlog::warn("[LoadPluginIniFiles] empty Script key in [OnLoadGame]: {}", iniName);
				continue;
			}
			if (functionName.empty()) {
				spdlog::warn("[LoadPluginIniFiles] empty Function key in [OnLoadGame]: {}", iniName);
				continue;
			}

			auto formKeyStr = Util::Trim(ini.GetValue("OnLoadGame", "FormKey", ""));
			auto formKey = LoadOrder::FormKey {};
			if (!formKeyStr.empty()) {
				formKey = LoadOrder::FormKey::From(Util::Trim(ini.GetValue("OnLoadGame", "FormKey", "")));
				if (!formKey) {
					spdlog::warn("[LoadPluginIniFiles] invalid FormKey '{}' in {}", std::string(formKey), iniName);
					continue;
				}
			}

			spdlog::debug("[LoadPluginIniFiles] ({}) {} # {}->{}", iniName, std::string(formKey), scriptName, functionName);
			g_onLoadGameFunctionMap[{ formKey, scriptName }] = functionName;
		}
	}

	inline void RunLoadFunctions () {
		// for (const auto& [key, functionName] : g_onLoadGameFunctionMap) {
		// 	const auto& [formKey, scriptName] = key;

		// 	if (!formKey) {
		// 		spdlog::info("[RunLoadFunction] calling global function {}.{}", scriptName, functionName);
		// 		Internal::CallGlobalFunctionNoWait(scriptName, functionName);
		// 		continue;
		// 	}

		// 	auto* form = formKey.Resolve();
		// 	if (!form) continue;

		// 	spdlog::info("[RunLoadFunction] calling local function {}#{}.{}", std::string(formKey), scriptName, functionName);
		// 	Internal::CallFunctionNoWait(form, scriptName, functionName);
		// }
	}

}
