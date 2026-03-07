#pragma once

#include "events.hpp"
#include "internal.hpp"
#include "project.h"
#include "papyrus.hpp"
#include "menu-text-input.hpp"
#include "util.hpp"

namespace Papyrus::SaveLoad {
	inline RE::BGSSaveLoadFileEntry g_load_entry {};
	inline std::string g_load_name;

	inline void SaveNextSlot (std::monostate mono) {
		spdlog::debug("SaveNextSlot called");

		Util::LoadIni(Project::ID);

		auto savePrefixRaw = Util::GetIniString(Project::ID, "Strings", "sSavePrefix", "Rotating Save");
		auto savePrefix = Util::SanitizeSaveName(savePrefixRaw);

		auto useSaveNotifications = Util::GetIniBool(Project::ID, "Main", "bSaveNotifications");

		auto numSlots = Util::GetIniInt(Project::ID, "Main", "iNumSaves", 10, 1, 99);

		auto characterSection = fmt::format("Character:{:08X}", Internal::EnsurePlayerId());

		auto lastSavedSlot = Util::GetIniInt(Project::ID, characterSection, "iLastSavedSlot", 0, 0, numSlots);

		auto nextSlot = lastSavedSlot + 1;

		if (nextSlot > numSlots) nextSlot = 1;

		auto saveName = fmt::format("{} {:02}", savePrefix, nextSlot);

		std::string successMessage;
		std::string failureMessage;

		if (useSaveNotifications) {
			successMessage = Util::GetIniString(Project::ID, "Strings", "sSaveNotification");
			failureMessage = Util::GetIniString(Project::ID, "Strings", "sCantSaveNow");
		}

		Internal::SaveGame(
			saveName,
			successMessage,
			failureMessage,
			[characterSection = std::move(characterSection), nextSlot] (bool wasSaved) {
				if (wasSaved) {
					Util::SetIniInt(Project::ID, characterSection, "iLastSavedSlot", nextSlot);
					Util::SaveIni(Project::ID);
				}
			}
		);
	}

	inline void ShowSaveMenu (std::monostate mono) {
		spdlog::debug("[ShowSaveMenu] called");

		Util::LoadIni(Project::ID);
		auto useSaveNotifications = Util::GetIniBool(Project::ID, "Main", "bSaveNotifications");

		std::string successMessage;
		std::string failureMessage;

		if (useSaveNotifications) {
			successMessage = Util::GetIniString(Project::ID, "Strings", "sSaveNotification");
			failureMessage = Util::GetIniString(Project::ID, "Strings", "sCantSaveNow");
		}

		Events::TextInput.once("Close", [successMessage, failureMessage] (std::string_view text) {
			spdlog::debug("ShowSaveMenu -> kHide");

			if (!text.empty()) {
				Internal::SaveGame(text, successMessage, failureMessage);
			}
			return true;
		});

		auto locationName = Internal::GetCurrentLocationName();
		if (locationName.empty()) locationName = "New Save";

		Menu::TextInput::Open("Save Game", locationName);
	}

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

	// inline void QueueSaveAndQuitToDesktop (std::monostate mono) {
	// 	auto* bslm = RE::BGSSaveLoadManager::GetSingleton();
	// 	bslm->BufferSceneScreenShot();
	// 	bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kSaveAndQuitToDesktop);
	// }

	inline void QueueLoadLastSave (std::monostate mono) {
		auto* bslm = RE::BGSSaveLoadManager::GetSingleton();
		bslm->QueueSaveLoadTask(RE::BGSSaveLoadManager::QUEUED_TASK::kLoadMostRecentSave);
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
}

namespace Project {

	inline bool RegisterPapyrusFunctions (RE::BSScript::IVirtualMachine* vm) {
		vm->BindNativeMethod(Project::ID, "SaveGame", Papyrus::SaveLoad::SaveGame, std::nullopt, true);

		vm->BindNativeMethod(Project::ID, "LoadGame", Papyrus::SaveLoad::QueueLoadGame, true);

		vm->BindNativeMethod(Project::ID, "ForceSave", Papyrus::SaveLoad::QueueForceSave, std::nullopt, true);
		vm->BindNativeMethod(Project::ID, "AutoSave", Papyrus::SaveLoad::QueueAutoSave, std::nullopt, true);

		vm->BindNativeMethod(Project::ID, "SaveNextSlot", Papyrus::SaveLoad::SaveNextSlot, true);
		vm->BindNativeMethod(Project::ID, "ShowSaveMenu", Papyrus::SaveLoad::ShowSaveMenu, true);
		vm->BindNativeMethod(Project::ID, "LoadLastSave", Papyrus::SaveLoad::QueueLoadLastSave, true);

		vm->BindNativeMethod(Project::ID, "GetPlayerId", Papyrus::GetPlayerId, std::nullopt, true);
		vm->BindNativeMethod(Project::ID, "SetPlayerId", Papyrus::SetPlayerId, std::nullopt, true);

		return true;
	}

	inline void main () {
		F4SE::GetPapyrusInterface()->Register(RegisterPapyrusFunctions);

		Events::Messaging.once(F4SE::MessagingInterface::kGameDataReady, [] () {
			spdlog::debug("kGameDataReady");
			Menu::TextInput::Register();
			return true;
		});
	}

}
