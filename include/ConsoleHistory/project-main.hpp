#pragma once

#include "console.hpp"
#include "events.hpp"

inline void InitializeConsoleInterface () {
	auto* ui = RE::UI::GetSingleton();
	if (!ui) {
		spdlog::error("Could not initialize interface");
		return;
	}

	auto isConsoleOpen = ui->GetMenuOpen(RE::Console::MENU_NAME);

	if (!isConsoleOpen) {
		spdlog::debug("Console was not open: Registering MenuOpenClose");

		Events::MenuOpenClose.once(RE::Console::MENU_NAME, [] (bool opening) {
			if (opening) {
				Console::RegisterHistoryHandlers();
				return true;
			}
			return false;
		});

	} else {
		spdlog::debug("Console was open: Registering");
		Console::RegisterHistoryHandlers();
	}
}

namespace Project {
	inline void main () {
		Events::Messaging.once(F4SE::MessagingInterface::kGameDataReady, [] () {
			InitializeConsoleInterface();
			return true;
		});
	}
}
