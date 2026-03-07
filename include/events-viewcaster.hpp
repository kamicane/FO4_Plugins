#pragma once

#include "events.hpp"

namespace Events {

	enum class ViewcasterType { Update };

	inline Dispatcher<ViewcasterType, void(RE::TESObjectREFR*)> Viewcaster {};

	namespace {
		using Event = RE::ViewCasterUpdateEvent;
		using Proc_t = RE::BSEventNotifyControl (*)(RE::BSTEventSink<Event>*, const Event&, RE::BSTEventSource<Event>*);

		inline Proc_t g_OriginalViewcasterSink = nullptr;
		inline std::uint32_t g_ViewcasterFormId = 0;

		inline RE::BSEventNotifyControl Hook_ViewcasterSink (
			RE::BSTEventSink<Event>* self,
			const Event& ev,
			RE::BSTEventSource<Event>* src
		) {
			// spdlog::debug("VC UPDATE");

			std::uint32_t newFormId = 0;
			RE::TESObjectREFR* ref = nullptr;

			if (ev.optionalValue) {
				auto handle = ev.optionalValue->currentVCData.activatePickRef;
				if (handle) {
					ref = handle.get().get();
					if (ref) {
						newFormId = ref->GetFormID();
						// std::string name = ref->GetDisplayFullName();
						// spdlog::debug("[ViewCaster] activatePickRef formID {:08X}, name={}", newFormId, name);
					} else {
						// spdlog::debug("[ViewCaster] activatePickRef handle existed but objectRef null");
						newFormId = 0;
					}
				} else {
					// spdlog::debug("[ViewCaster] no activatePickRef");
					newFormId = 0;
				}
			}

			if (newFormId != g_ViewcasterFormId) {
				Viewcaster.dispatch(ViewcasterType::Update, ref);
				g_ViewcasterFormId = newFormId;
			}

			if (g_OriginalViewcasterSink) {
				return g_OriginalViewcasterSink(self, ev, src);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	}

	inline std::uint32_t GetViewcasterFormId () {
		return g_ViewcasterFormId;
	}

	inline void InstallViewcasterHook () {
		if (g_OriginalViewcasterSink) return;
		REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::BSTValueEventSink_ViewCasterUpdateEvent_[0] };
		auto old = vtbl.write_vfunc(1, &Hook_ViewcasterSink);
		g_OriginalViewcasterSink = reinterpret_cast<Proc_t>(old);
		spdlog::debug("[Events::ViewCaster] Installed");
	}

	inline void UninstallViewcasterHook () {
		if (!g_OriginalViewcasterSink) return;
		REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::BSTValueEventSink_ViewCasterUpdateEvent_[0] };
		vtbl.write_vfunc(1, g_OriginalViewcasterSink);
		g_OriginalViewcasterSink = nullptr;
		g_ViewcasterFormId = 0;
		spdlog::debug("[Events::ViewCaster] Uninstalled");
	}

}
