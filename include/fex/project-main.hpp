#pragma once

#include "events.hpp"
#include "events-input.hpp"
#include "events-viewcaster.hpp"
#include "events-quickcontainer.hpp"
#include "internal.hpp"
#include "papyrus.hpp"
#include "scrap-scrap.hpp"

#include "fex/load-handler.hpp"
#include "fex/papyrus-settings.hpp"
#include "fex/papyrus-logging.hpp"

void DumpControlMappings () {
	auto* cm = RE::ControlMap::GetSingleton();
	if (!cm) return;

	for (auto ctx = 0; ctx < std::to_underlying(RE::UserEvents::INPUT_CONTEXT_ID::kTotal); ++ctx) {
		auto* ic = cm->controlMaps[ctx];
		if (!ic) continue;
		for (auto dev = 0; dev < std::to_underlying(RE::INPUT_DEVICE::kSupported); ++dev) {
			for (auto& m : ic->deviceMappings[dev]) {
				spdlog::debug("[Controls] ctx={} dev={} id={} key={}", ctx, dev, m.eventID.c_str(), m.inputKey);
			}
		}
	}
}

namespace Project {

	inline void InitializeLoadEvent () {
		Events::SaveLoad.on(Events::SaveLoadType::Load, [] () {
			LoadHandler::RunLoadFunctions();
		});
	}

	inline bool RegisterPapyrusFunctions (RE::BSScript::IVirtualMachine* vm) {
		vm->BindNativeMethod("fex", "getForm", Papyrus::GetFormByEditorId, true);

		vm->BindNativeMethod("fex", "updatePowerArmor3d", Papyrus::UpdatePowerArmor3d, true);

		vm->BindNativeMethod("fex", "notification", Papyrus::ShowMessage, true);

		vm->BindNativeMethod("fex:game", "getCrosshairRef", Papyrus::GetCrosshairRef, true);

		vm->BindNativeMethod("fex:debug", "scrapRef", Papyrus::Scrap::TestScrapRef, true);
		vm->BindNativeMethod("fex:debug", "searchRefs", Papyrus::TestSearchRefs, true);
		vm->BindNativeMethod("fex:debug", "printInventory", Papyrus::TestPrintInventory, true);
		vm->BindNativeMethod("fex:debug", "transfer", Papyrus::TestTransferItems, true);
		vm->BindNativeMethod("fex:debug", "getCallerName", Papyrus::TestGetCallerName, true);

		vm->BindNativeMethod("fex:settings", "load", Papyrus::Settings::LoadSettings, true);
		vm->BindNativeMethod("fex:settings", "save", Papyrus::Settings::SaveSettings, true);

		vm->BindNativeMethod("fex:settings", "getBool", Papyrus::Settings::GetSettingBool, true);
		vm->BindNativeMethod("fex:settings", "setBool", Papyrus::Settings::SetSettingBool, true);

		vm->BindNativeMethod("fex:settings", "getString", Papyrus::Settings::GetSettingString, true);
		vm->BindNativeMethod("fex:settings", "setString", Papyrus::Settings::SetSettingString, true);

		vm->BindNativeMethod("fex:settings", "getInt", Papyrus::Settings::GetSettingInt, true);
		vm->BindNativeMethod("fex:settings", "setInt", Papyrus::Settings::SetSettingInt, true);
		vm->BindNativeMethod("fex:settings", "getFloat", Papyrus::Settings::GetSettingFloat, true);
		vm->BindNativeMethod("fex:settings", "setFloat", Papyrus::Settings::SetSettingFloat, true);

		vm->BindNativeMethod("fex:settings", "getStruct", Papyrus::Settings::GetSettingsStruct, true);
		vm->BindNativeMethod("fex:settings", "setStruct", Papyrus::Settings::SetSettingsStruct, true);

		vm->BindNativeMethod("fex", "format", Papyrus::Logging::Format, true);
		vm->BindNativeMethod("fex", "consoleLog", Papyrus::Logging::ConsoleLog, true);

		vm->BindNativeMethod("fex:log", "debug", Papyrus::Logging::debug, true);
		vm->BindNativeMethod("fex:log", "info", Papyrus::Logging::info, true);
		vm->BindNativeMethod("fex:log", "warn", Papyrus::Logging::warn, true);
		vm->BindNativeMethod("fex:log", "error", Papyrus::Logging::error, true);

		return true;
	}

	inline void main () {
		F4SE::GetPapyrusInterface()->Register(RegisterPapyrusFunctions);

		Events::Messaging.once(F4SE::MessagingInterface::kGameDataReady, [] () {
			Internal::CreateLookupMaps();

			LoadHandler::LoadPluginIniFiles();

			Events::InstallViewcasterHook();

			Events::Viewcaster.on(Events::ViewcasterType::Update, [] (RE::TESObjectREFR* objectRef) {
				spdlog::debug("[Event:ViewcasterUpdate] {}", Internal::ToString(objectRef));
			});

			Events::InstallQuickContainerHook();

			Events::QuickContainer.on(Events::QuickContainerType::Update, [] (const Events::QuickContainerData& data) {
				if (data) {
					spdlog::debug(
						"[Event:QuickContainerUpdate] Container={}, Inventory={}, Selected={}",
						Internal::ToString(data.containerRef),
						Internal::ToString(data.inventoryRef),
						data.selectedName
					);
				} else {
					spdlog::debug("[Event:QuickContainerUpdate] <Empty>");
				}
			});

			Events::InstallInputHook();

			Events::Control.on("Sprint", [] (bool pressed) {
				spdlog::debug("[Event:ControlSprint] {}", pressed);
			});

			InitializeLoadEvent();
			return true;
		});
	}
}
