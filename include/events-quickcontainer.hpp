#pragma once

#include "events.hpp"

namespace Events {

	enum class QuickContainerType { Update };

	struct QuickContainerData {
		RE::TESObjectREFR* containerRef = nullptr;
		RE::TESObjectREFR* inventoryRef = nullptr;
		RE::QuickContainerMode mode = RE::QuickContainerMode::kLoot;
		std::string selectedName;
		std::string containerName;

		bool operator== (const QuickContainerData& a_rhs) const noexcept = default;

		bool operator!= (const QuickContainerData& a_rhs) const noexcept = default;

		explicit operator bool () const noexcept {
			return containerRef != nullptr || inventoryRef != nullptr;
		}
	};

	inline Dispatcher<QuickContainerType, void(const QuickContainerData&)> QuickContainer {};

	namespace {
		QuickContainerData g_quickContainerData;

		using Event_t = RE::QuickContainerStateEvent;
		using QuickContainerProc_t =
			RE::BSEventNotifyControl (*)(RE::BSTEventSink<Event_t>*, const Event_t&, RE::BSTEventSource<Event_t>*);

		inline QuickContainerProc_t g_OriginalQuickContainerSink = nullptr;

		inline RE::BSEventNotifyControl Hook_QuickContainerSink (
			RE::BSTEventSink<Event_t>* self,
			const Event_t& event,
			RE::BSTEventSource<Event_t>* src
		) {
			if (event.optionalValue) {
				const auto& data = event.optionalValue.value();

				// spdlog::info(
				// 	"[QuickContainer] containerName='{}' selectedIndex={} mode={} flags: A={} X={} locked={}",
				// 	data.containerName.c_str(),
				// 	data.selectedClipIndex,
				// 	data.mode.underlying(),
				// 	data.buttonAEnabled,
				// 	data.buttonXEnabled,
				// 	data.isLocked
				// );

				QuickContainerData tmpData;
				tmpData.containerName = data.containerName;

				if (data.containerRef) tmpData.containerRef = data.containerRef.get().get();
				if (data.inventoryRef) tmpData.inventoryRef = data.inventoryRef.get().get();

				tmpData.mode = static_cast<RE::QuickContainerMode>(data.mode.underlying());

				if (0 <= data.selectedClipIndex && static_cast<std::size_t>(data.selectedClipIndex) < data.itemData.size()) {
					const auto& sel = data.itemData[static_cast<std::size_t>(data.selectedClipIndex)];
					tmpData.selectedName = sel.itemName;
				}

				if (tmpData != g_quickContainerData) {
					QuickContainer.dispatch(QuickContainerType::Update, tmpData);
					g_quickContainerData = tmpData;
				}

				// size_t idx = 0;
				// for (const auto& entry : data.itemData) {
				// 	spdlog::info(
				// 		"[QuickContainer] item[{}] name='{}' count={} equipState={} legendary={} favorite={} tagged={} better={}",
				// 		idx,
				// 		entry.itemName.c_str(),
				// 		entry.itemCount,
				// 		entry.equipState,
				// 		entry.isLegendary,
				// 		entry.isFavorite,
				// 		entry.isTaggedForSearch,
				// 		entry.isBetterThanEquippedItem
				// 	);
				// 	++idx;
				// }
			} else {
				if (g_quickContainerData) {
					g_quickContainerData = {};
					QuickContainer.dispatch(QuickContainerType::Update, g_quickContainerData);
				}
			}

			if (g_OriginalQuickContainerSink) {
				return g_OriginalQuickContainerSink(self, event, src);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	}

	inline const QuickContainerData& GetQuickContainerData () {
		return g_quickContainerData;
	}

	inline void InstallQuickContainerHook () {
		if (g_OriginalQuickContainerSink) return;

		REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::BSTValueEventSink_QuickContainerStateEvent_[0] };
		auto old = vtbl.write_vfunc(1, &Hook_QuickContainerSink);
		g_OriginalQuickContainerSink = reinterpret_cast<QuickContainerProc_t>(old);
		spdlog::debug("[Events::QuickContainer] Installed vtable hook");
	}

	inline void UninstallQuickContainerHook () {
		if (!g_OriginalQuickContainerSink) return;

		REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::BSTValueEventSink_QuickContainerStateEvent_[0] };
		vtbl.write_vfunc(1, g_OriginalQuickContainerSink);
		g_OriginalQuickContainerSink = nullptr;
		spdlog::debug("[Events::QuickContainer] Uninstalled vtable hook");
	}

}
