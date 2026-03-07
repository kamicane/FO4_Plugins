#pragma once

#include "project.h"
#include "internal.hpp"

namespace LoadHandler {

	using OnLoadGameKey = std::pair<RE::TESForm*, std::string>;

	struct OnLoadGameKeyHash {
		std::size_t operator() (const OnLoadGameKey& key) const {
			const auto h1 = std::hash<const RE::TESForm*> {}(key.first);
			const auto h2 = std::hash<std::string> {}(key.second);
			return h1 ^ (h2 << 1);
		}
	};

	struct OnLoadGameKeyEqual {
		bool operator() (const OnLoadGameKey& lhs, const OnLoadGameKey& rhs) const {
			return lhs.first == rhs.first && lhs.second == rhs.second;
		}
	};

	inline std::unordered_map<OnLoadGameKey, std::string, OnLoadGameKeyHash, OnLoadGameKeyEqual> g_onLoadGameFunctionMap {};

	inline void LoadPluginIniFiles () {
		auto iniDir = Util::GetGameDir() / "Data" / Project::ID;
		if (!std::filesystem::exists(iniDir)) return;
		spdlog::debug("[LoadPluginIniFiles] scanning ini directory {}", iniDir.string());

		// collect available ini files into a lookup (stem -> path)
		std::unordered_map<std::string, std::filesystem::path> iniStemToPath;
		for (const auto& entry : std::filesystem::directory_iterator(iniDir)) {
			if (!entry.exists()) continue;
			auto path = entry.path();
			if (Util::ToLower(path.extension().string()) != ".ini") continue;

			auto stem = path.stem().string();
			iniStemToPath[Util::ToLower(stem)] = path;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			spdlog::error("FATAL: Failed to get datahandler");
			return;
		}

		for (auto* file : dataHandler->files) {
			if (!file || file->compileIndex == 0xFF) continue;
			auto fileName = file->GetFilename();
			spdlog::debug("[LoadHandler] {}", fileName);

			auto pluginStem = std::filesystem::path(fileName).stem().string();
			auto lowerStem = Util::ToLower(pluginStem);
			auto it = iniStemToPath.find(lowerStem);
			if (it == iniStemToPath.end()) continue;

			auto path = it->second;
			auto iniName = path.filename().string();

			CSimpleIniA ini;
			ini.SetUnicode(true);
			if (ini.LoadFile(path.string().c_str()) < 0) {
				spdlog::warn("[LoadPluginIniFiles] failed to parse {}", iniName);
				continue;
			}

			spdlog::debug("[LoadPluginIniFiles] Found ini file {} (matched {})", iniName, fileName);

			std::string scriptName = ini.GetValue("OnLoadGame", "Script", "");
			std::string functionName = ini.GetValue("OnLoadGame", "Function", "");

			if (scriptName.empty()) {
				spdlog::warn("[LoadPluginIniFiles] empty Script key in [OnLoadGame]: {}", iniName);
				continue;
			}
			if (functionName.empty()) {
				spdlog::warn("[LoadPluginIniFiles] empty Function key in [OnLoadGame]: {}", iniName);
				continue;
			}

			std::string editorId = ini.GetValue("OnLoadGame", "Form", "");
			RE::TESForm* formPtr = nullptr;
			if (!editorId.empty()) {
				formPtr = RE::TESForm::GetFormByEditorID(RE::BSFixedString(editorId));
				if (!formPtr) {
					spdlog::warn("[LoadPluginIniFiles] could not resolve editorID '{}' in {}", editorId, iniName);
					continue;
				}
			}

			spdlog::debug(
				"[LoadPluginIniFiles] ({}) {}#{}.{}",
				iniName,
				editorId.empty() ? "<global>" : editorId,
				scriptName,
				functionName
			);
			g_onLoadGameFunctionMap[{ formPtr, scriptName }] = functionName;
		}
	}

	inline void RunLoadFunctions () {
		for (const auto& [key, functionName] : g_onLoadGameFunctionMap) {
			const auto& [formPtr, scriptName] = key;

			if (!formPtr) {
				spdlog::debug("[RunLoadFunction] calling global function {}.{}", scriptName, functionName);
				Internal::CallGlobalFunctionNoWait(scriptName, functionName);
				continue;
			}

			spdlog::debug("[RunLoadFunction] calling function {}#{}.{}", formPtr->GetFormEditorID(), scriptName, functionName);
			Internal::CallFunctionNoWait(formPtr, scriptName, functionName);
		}
	}

}
