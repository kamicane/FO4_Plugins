#pragma once

namespace Util {
	inline std::unordered_map<std::string, std::shared_ptr<CSimpleIniA>> g_iniMap;

	std::string Trim (std::string_view input) {
		size_t first = 0;
		size_t last = input.size();

		while (first < last && std::isspace(static_cast<unsigned char>(input[first]))) {
			++first;
		}
		while (last > first && std::isspace(static_cast<unsigned char>(input[last - 1]))) {
			--last;
		}
		return std::string(input.substr(first, last - first));
	}

	std::string ToLower (std::string_view input) {
		auto result = std::string(input);
		for (char& chr : result) {
			chr = static_cast<char>(std::tolower(static_cast<unsigned char>(chr)));
		}
		return result;
	}

	std::uint32_t RandomUInt32 () {
		static std::mt19937 gen(std::random_device {}());
		static std::uniform_int_distribution<std::uint32_t> dist(1U, 0xFFFFFFFFU);
		return dist(gen);
	}

	std::uint32_t Uint32FromHex (std::string_view uint32Hex) {
		if (uint32Hex.empty()) {
			return 0;
		}

		try {
			unsigned long val = std::stoul(std::string(uint32Hex), nullptr, 16);
			return static_cast<std::uint32_t>(val);
		} catch (...) {
			return 0;
		}
	}

	std::filesystem::path GetGameDir () {
		static std::filesystem::path cachedDir;

		if (cachedDir.empty()) {
			std::wstring buf(MAX_PATH, L'\0');
			const DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
			if (len != 0) {
				buf.resize(len);
				cachedDir = (std::filesystem::path(buf).parent_path()).lexically_normal();
			} else {
				cachedDir = std::filesystem::temp_directory_path();
			}
		}

		return cachedDir;
	}

	std::filesystem::path GetMyDocumentsDir () {
		static std::filesystem::path cachedDir;

		if (cachedDir.empty()) {
			std::wstring docsPath(MAX_PATH, L'\0');
			if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, docsPath.data()))) {
				const size_t len = std::wcslen(docsPath.c_str());
				docsPath.resize(len);
				cachedDir = (std::filesystem::path(docsPath) / L"My Games" / L"Fallout4").lexically_normal();
			} else {
				cachedDir = std::filesystem::temp_directory_path();
			}
		}

		return cachedDir;
	}

	std::filesystem::path GetMCMDir () {
		static std::filesystem::path cachedDir;

		if (cachedDir.empty()) {
			cachedDir = GetGameDir() / "Data/MCM";
		}

		return cachedDir;
	}

	std::filesystem::path GetSystemConfigFile (std::string_view stem) {
		auto mcmPath = GetMCMDir();
		return mcmPath / "Config" / stem / "settings.ini";
	}

	std::filesystem::path GetUserConfigFile (std::string_view stem) {
		auto mcmPath = GetMCMDir();

		return mcmPath / "Settings" / (std::string(stem) + ".ini");
	}

	std::filesystem::path GetUserLogFile (std::string_view stem) {
		static std::filesystem::path cachedDir;

		if (cachedDir.empty()) {
			cachedDir = GetMyDocumentsDir() / "F4SE";
		}

		std::string fileNameLog;
		fileNameLog += stem;
		fileNameLog += ".log";

		return cachedDir / fileNameLog;
	}

	spdlog::level::level_enum level_from_str (std::string_view level) {
		auto logLevel = spdlog::level::debug;
		if (level.empty()) return logLevel;

		try {
			logLevel = spdlog::level::from_str(std::string(level));
		} catch (...) {
			logLevel = spdlog::level::debug;
		}

		return logLevel;
	}

	std::shared_ptr<spdlog::logger> CreateLogger (
		const std::string_view userId,
		const std::filesystem::path& logFile,
		std::string_view level
	) {
		auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(std::string(userId), logFile.string(), true);

		auto logLevel = level_from_str(level);

		logger->set_level(logLevel);
		logger->flush_on(logLevel);

		logger->set_pattern("[%H:%M:%S.%f] [%L] %v");

		return logger;
	}

	std::string SanitizeFileName (std::string_view input) {
		static const std::regex invalid_re(R"([<>:"/\\|?*\x00-\x1F])");
		static const std::regex multi_space_re(R"(\s+)");
		static const std::regex trim_re(R"(^\s+|\s+$)");

		std::string safe;

		// First, replace some common ASCII punctuation with visually-similar
		// Unicode alternatives so we preserve the appearance while avoiding
		// filesystem/reserved-character issues. Support: ?  <  >  :
		for (unsigned char ch : input) {
			switch (ch) {
				case '?': safe += "？"; break; // U+FF1F FULLWIDTH QUESTION MARK
				case '<': safe += "＜"; break; // U+FF1C FULLWIDTH LESS-THAN
				case '>': safe += "＞"; break; // U+FF1E FULLWIDTH GREATER-THAN
				case ':': safe += "："; break; // U+FF1A FULLWIDTH COLON
				default: safe.push_back(static_cast<char>(ch)); break;
			}
		}

		// Fallback: replace any remaining invalid characters with a space,
		// then collapse whitespace and trim.
		safe = std::regex_replace(safe, invalid_re, std::string(" "));
		safe = std::regex_replace(safe, multi_space_re, std::string(" "));
		safe = std::regex_replace(safe, trim_re, std::string(""));

		return safe;
	}

	std::string SanitizeSaveName (std::string_view input) {
		static const std::regex underscore_re(R"(_+)");
		static const std::regex hyphen_re(R"(-+)");

		auto safe = SanitizeFileName(input);

		// Replace underscores and ASCII hyphens with visually-similar Unicode
		// characters so save names don't contain '_' or '-'.
		// '_' -> U+FF3F FULLWIDTH LOW LINE (＿)
		safe = std::regex_replace(safe, underscore_re, std::string("＿"));
		// '-' -> U+2010 HYPHEN (‐)
		safe = std::regex_replace(safe, hyphen_re, std::string("‐"));

		return safe;
	}

	std::shared_ptr<CSimpleIniA> GetIniPtr (std::string_view userId) {
		auto [it, inserted] = g_iniMap.emplace(userId, nullptr);
		if (inserted) {
			it->second = std::make_shared<CSimpleIniA>();
			it->second->SetUnicode(true);
		}
		return it->second;
	}

	void LoadIni (std::string_view userId) {
		auto ini = GetIniPtr(userId);
		ini->Reset();

		auto systemPath = GetSystemConfigFile(userId);
		ini->LoadFile(systemPath.c_str());
		auto userPath = GetUserConfigFile(userId);
		ini->LoadFile(userPath.c_str());
	}

	std::string GetIniString (
		std::string_view userId,
		std::string_view section,
		std::string_view key,
		std::string_view defaultValue = ""
	) {
		auto ini = GetIniPtr(userId);

		const std::string section_s { section };
		const std::string key_s { key };

		if (const auto* val = ini->GetValue(section_s.c_str(), key_s.c_str())) return val;

		return std::string(defaultValue);
	}

	bool GetIniBool (std::string_view userId, std::string_view section, std::string_view key, bool defaultValue = false) {
		auto val = GetIniString(userId, section, key);
		if (val.empty()) return defaultValue;

		val = ToLower(val);

		if (val == "1" || val == "true" || val == "yes") return true;
		if (val == "0" || val == "false" || val == "no") return false;

		return defaultValue;
	}

	int32_t GetIniInt (
		std::string_view userId,
		std::string_view section,
		std::string_view key,
		int32_t defaultValue = 0,
		int32_t min = std::numeric_limits<int32_t>::min(),
		int32_t max = std::numeric_limits<int32_t>::max()
	) {
		auto raw = GetIniString(userId, section, key);
		if (raw.empty()) return std::clamp<int32_t>(defaultValue, min, max);

		auto val = ToLower(raw);

		// Handle boolean-like values
		if (val == "1" || val == "true" || val == "yes") return std::clamp<int32_t>(1, min, max);
		if (val == "0" || val == "false" || val == "no") return std::clamp<int32_t>(0, min, max);

		// Prepare numeric parse (support hex 0x...)
		int base = 10;
		std::string parseStr = raw;
		if (parseStr.size() > 2 && parseStr[0] == '0' && (parseStr[1] == 'x' || parseStr[1] == 'X')) {
			base = 16;
		}

		long long parsed = 0;
		try {
			parsed = std::stoll(parseStr, nullptr, base);
		} catch (...) {
			return std::clamp<int32_t>(defaultValue, min, max);
		}

		// Clamp to int32_t range first
		if (parsed < static_cast<long long>(std::numeric_limits<int32_t>::min()))
			parsed = std::numeric_limits<int32_t>::min();
		if (parsed > static_cast<long long>(std::numeric_limits<int32_t>::max()))
			parsed = std::numeric_limits<int32_t>::max();

		auto result = static_cast<int32_t>(parsed);
		if (result < min) return min;
		if (result > max) return max;
		return result;
	}

	float GetIniFloat (
		std::string_view userId,
		std::string_view section,
		std::string_view key,
		float defaultValue = 0.0F,
		float min = std::numeric_limits<float>::lowest(),
		float max = std::numeric_limits<float>::max()
	) {
		auto raw = GetIniString(userId, section, key);
		if (raw.empty()) return std::clamp<float>(defaultValue, min, max);

		auto val = ToLower(raw);

		// Handle boolean-like values
		if (val == "1" || val == "true" || val == "yes") return std::clamp<float>(1.0F, min, max);
		if (val == "0" || val == "false" || val == "no") return std::clamp<float>(0.0F, min, max);

		float parsed = 0.0F;
		try {
			parsed = std::stof(raw);
		} catch (...) {
			return std::clamp<float>(defaultValue, min, max);
		}

		if (parsed < min) return min;
		if (parsed > max) return max;
		return parsed;
	}

	void SetIniString (std::string_view userId, std::string_view section, std::string_view key, std::string_view value) {
		auto ini = GetIniPtr(userId);

		const std::string section_s { section };
		const std::string key_s { key };
		const std::string value_s { value };

		ini->SetValue(section_s.c_str(), key_s.c_str(), value_s.c_str());
	}

	void SetIniBool (std::string_view userId, std::string_view section, std::string_view key, bool value) {
		SetIniString(userId, section, key, value ? "true" : "false");
	}

	void SetIniInt (std::string_view userId, std::string_view section, std::string_view key, int32_t value) {
		SetIniString(userId, section, key, std::to_string(value));
	}

	void SetIniFloat (std::string_view userId, std::string_view section, std::string_view key, float value) {
		SetIniString(userId, section, key, std::to_string(value));
	}

	void SaveIni (std::string_view userId) {
		auto systemIni = std::make_shared<CSimpleIniA>();
		systemIni->SetUnicode(true);

		auto systemPath = GetSystemConfigFile(userId);
		systemIni->LoadFile(systemPath.c_str());

		auto userIni = std::make_shared<CSimpleIniA>();
		userIni->SetUnicode(true);

		CSimpleIniA::TNamesDepend sections;

		auto ini = GetIniPtr(userId);

		ini->GetAllSections(sections);

		for (const auto& sectionEntry : sections) {
			if (!sectionEntry.pItem) continue;

			const char* section = sectionEntry.pItem;
			CSimpleIniA::TNamesDepend keys;
			ini->GetAllKeys(section, keys);

			for (const auto& keyEntry : keys) {
				if (!keyEntry.pItem) continue;

				const char* key = keyEntry.pItem;
				const char* valueRaw = ini->GetValue(section, key);
				if (!valueRaw) continue;
				const char* systemValueRaw = systemIni->GetValue(section, key);

				if (systemValueRaw && std::strcmp(valueRaw, systemValueRaw) == 0) continue;

				userIni->SetValue(section, key, valueRaw);
			}
		}

		auto userPath = GetUserConfigFile(userId);
		std::filesystem::create_directories(userPath.parent_path());
		userIni->SaveFile(userPath.string().c_str());
	}

	std::shared_ptr<spdlog::logger> GetLogger (std::string_view userId) {
		auto logger = spdlog::get(std::string(userId));

		if (!logger) {
			auto logLevel = ToLower(GetIniString(userId, "Logging", "Level"));

			auto fullPath = GetUserLogFile(userId);
			logger = CreateLogger(userId, fullPath, logLevel);
		}

		return logger;
	}

	void SetLogPattern (std::string_view userId, std::string_view pattern) {
		auto logger = GetLogger(userId);
		if (!logger) return;

		logger->set_pattern(pattern.data()); // NOLINT
	}
}
