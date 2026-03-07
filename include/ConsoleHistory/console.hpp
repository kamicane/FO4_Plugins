#pragma once

#include "events.hpp"
#include "project.h"
#include "util.hpp"
#include "internal.hpp"
#include "menu-util.hpp"

namespace Console {

	inline std::shared_ptr<std::fstream> histFile;
	inline std::string lastCommand;

	// names are confusing because names in the action script are confusing
	// Actionscript has a method called AddHistory, which just prints stuff to the console (adds to buffer)
	// there *was no method to add a command to the actual history
	// I added AddCommandToHistory that does just that

	inline void AddCommandToHistory (RE::Scaleform::Ptr<RE::IMenu> console, const std::string& thisCommand) {
		if (thisCommand.empty()) return;
		MenuUtil::InvokeMethod(console, "AddCommandToHistory", thisCommand);

		lastCommand = thisCommand;
	}

	inline void SetCommandPrompt (RE::Scaleform::Ptr<RE::IMenu> console) {
		auto promptName = Internal::GetPlayerName();

		auto promptLocation = Internal::GetCurrentLocationName();
		if (!promptLocation.empty()) promptLocation = "/" + promptLocation;

		MenuUtil::InvokeMethod(console, "SetCommandPromptEx", promptName, promptLocation);
	}

	inline void ExecuteCommand (RE::Scaleform::Ptr<RE::IMenu> console, const std::string& thisCommand) {
		auto spacePos = thisCommand.find(' ');
		auto aliasName = (spacePos == std::string::npos) ? thisCommand : thisCommand.substr(0, spacePos);
		auto aliasArgs = (spacePos == std::string::npos) ? std::string() : thisCommand.substr(spacePos + 1);

		auto alias = Util::GetIniString(Project::ID, "Aliases", aliasName);
		std::string realCommand;

		if (alias.empty()) {
			realCommand = thisCommand;
		} else {
			if (aliasArgs.empty()) {
				realCommand = alias;
			} else {
				realCommand = fmt::format("{} {}", alias, aliasArgs);
			}
		}

		MenuUtil::InvokeMethod(console, "ExecuteCommand", realCommand);
	}

	struct SaveCommandHandler : RE::Scaleform::GFx::FunctionHandler {
		SaveCommandHandler () = default;

		void Call (const RE::Scaleform::GFx::FunctionHandler::Params& params) override {
			if (params.argCount < 1 || !params.args) return;
			const auto& val = *(params.args);

			if (!val.IsString()) return;
			std::string thisCommand = val.GetString() ? val.GetString() : std::string();

			if (thisCommand.empty()) return;

			auto* ui = RE::UI::GetSingleton();
			if (!ui) {
				spdlog::error("SaveCommandHandler: ui not available");
				return;
			}
			auto console = ui->GetMenu(RE::Console::MENU_NAME);
			if (!console) {
				spdlog::error("SaveCommandHandler: console not available");
				return;
			}

			// don't save qqq when typed from the console
			// don't save same commands when typed consecutively (or grabbed from history)
			if (thisCommand != "qqq" && lastCommand != thisCommand) {
				AddCommandToHistory(console, thisCommand);

				if (!histFile || !histFile->is_open()) {
					spdlog::error("SaveCommandHandler: file stream not available");
					return;
				}

				(*histFile) << thisCommand << '\n';
				histFile->flush();
			}

			ExecuteCommand(console, thisCommand);
		}
	};

	inline bool RegisterHistoryHandlers () {
		// Validate UI and console
		auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			spdlog::error("UI singleton is null");
			return false;
		}

		auto console = ui->GetMenu(RE::Console::MENU_NAME);
		if (!console) {
			spdlog::error("Console menu is null");
			return false;
		}

		auto uiMovie = console->uiMovie;

		if (!uiMovie) {
			spdlog::error("uiMovie is null");
			return false;
		}

		auto menuObj = console->menuObj;

		if (!menuObj.IsObject()) {
			spdlog::error("menuObj is not an object");
			return false;
		}

		auto root = uiMovie->asMovieRoot;

		if (!root) {
			spdlog::error("asMovieRoot is null");
			return false;
		}

		RE::Scaleform::GFx::Value consoleHistoryObj;

		root->CreateObject(&consoleHistoryObj);

		Util::LoadIni(Project::ID);

		std::string histFileDefaultName = "console_history.txt";

		std::filesystem::path histFilePath = Util::GetIniString(Project::ID, "Main", "HistoryFile", histFileDefaultName);
		if (std::filesystem::is_directory(histFilePath)) {
			histFilePath /= histFileDefaultName;
		}

		histFilePath = Util::GetMyDocumentsDir() / histFilePath;

		spdlog::info("History file is {}", histFilePath.string());

		auto historySize = Util::GetIniInt(Project::ID, "Main", "HistorySize", 128, 32, 1024);

		MenuUtil::InvokeMethod(console, "SetHistorySize", historySize);

		std::deque<std::string> lastLines;

		histFile = std::make_shared<std::fstream>();

		histFile->open(histFilePath, std::ios::in | std::ios::out | std::ios::app);

		if (!histFile->is_open()) {
			spdlog::error("RegisterHistoryHandlers: failed to open {}", histFilePath.string());
			return false;
		}

		// Read last lines
		histFile->seekg(0);
		std::string fileLine;
		while (std::getline(*histFile, fileLine)) {
			lastLines.push_back(fileLine);
			if (lastLines.size() > static_cast<std::size_t>(historySize)) lastLines.pop_front();
		}

		// Truncate and rewrite
		histFile->close();
		histFile->open(histFilePath, std::ios::out | std::ios::trunc);
		for (const auto& line : lastLines) {
			*histFile << line << '\n';
			AddCommandToHistory(console, line);
		}

		// Keep file open for future append
		histFile->close();
		histFile->open(histFilePath, std::ios::out | std::ios::app);

		// Register as function to save command to our history
		RE::Scaleform::GFx::Value saveCommandFunc;
		root->CreateFunction(&saveCommandFunc, new SaveCommandHandler());
		consoleHistoryObj.SetMember("saveCommand", saveCommandFunc);

		if (!menuObj.SetMember("ConsoleHistoryObj", consoleHistoryObj)) {
			spdlog::error("Failed to set ConsoleHistoryObj as member of menuObj");
			return false;
		}

		auto welcomeMessage = Util::GetIniString(Project::ID, "Main", "WelcomeMessage", "");

		if (!welcomeMessage.empty()) {
			auto msgStr = fmt::format(fmt::runtime(welcomeMessage), histFilePath.string());
			auto msgStrNl = msgStr + "\n";

			MenuUtil::InvokeMethod(console, "AddHistory", msgStrNl);

			spdlog::info(msgStr);
		}

		spdlog::info("ConsoleHistory initialized succesfully");

		Events::MenuOpenClose.on(RE::Console::MENU_NAME, [console] (bool opening) {
			if (opening) {
				SetCommandPrompt(console);
			}
		});

		SetCommandPrompt(console);

		return true;
	}

}
