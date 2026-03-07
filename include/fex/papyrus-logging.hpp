#pragma once

#include "util.hpp"
#include "internal.hpp"

namespace Papyrus::Logging {

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

	inline void info (
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

	inline void warn (
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

	inline void debug (
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

	inline void error (
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
}
