#pragma once

#include "events.hpp"

namespace Events {

	class ControlHeldDispatcher : public Dispatcher<std::pair<std::string_view, float>, void()> {
	private:
		std::vector<std::pair<std::string, float>> regs;
		using Base = Dispatcher<std::pair<std::string_view, float>, void()>;

	public:
		template<typename Listener>
		auto on (std::pair<std::string_view, float> event, Listener&& listener) {
			regs.emplace_back(std::string(event.first), event.second);
			return Base::on(event, std::forward<Listener>(listener));
		}

		template<typename Handle>
		void off (std::pair<std::string_view, float> event, Handle handle) {
			regs.erase(
				std::remove_if(
					regs.begin(),
					regs.end(),
					[&] (auto const& p) {
						return p.first == event.first && p.second == event.second;
					}
				),
				regs.end()
			);
			Base::off(event, handle);
		}

		template<typename Listener>
		void once (std::pair<std::string_view, float> event, Listener&& listener) {
			regs.emplace_back(std::string(event.first), event.second);
			Base::once(event, std::forward<Listener>(listener));
		}

		const std::vector<std::pair<std::string, float>>& registered () const noexcept {
			return regs;
		}
	};

	inline Dispatcher<std::string_view, void(bool)> Control {};
	inline ControlHeldDispatcher ControlHeld {};

	namespace {
		using PerformInputProc = void (*)(RE::PlayerControls*, const RE::InputEvent*);
		inline PerformInputProc original_PerformInputProcessing = nullptr;

		// inline std::unordered_map<std::string, std::chrono::steady_clock::time_point> s_pressTimes;

		inline void Hook_PerformInputProcessing (RE::PlayerControls* self, const RE::InputEvent* queueHead) {
			for (const auto* event = queueHead; event; event = event->next) {
				if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
					const auto* button = reinterpret_cast<const RE::ButtonEvent*>(event);
					// const auto devInt = *reinterpret_cast<const std::int32_t*>(&b->device);
					// spdlog::debug(
					// 	"[RawInput] Button: {} dev={} id={} just={} pressed={} released={}",
					// 	b->QRawUserEvent().c_str(),
					// 	devInt,
					// 	b->idCode,
					// 	b->QJustPressed(),
					// 	b->QPressed(),
					// 	b->QReleased()
					// );

					// const std::string controlName = button->QRawUserEvent().c_str();
					const auto controlName = std::string(button->strUserEvent);
					// spdlog::debug("[InputHook] controlName = {}, held = {}", controlName, button->heldDownSecs);

					if (button->QJustPressed()) {
						// s_pressTimes.erase(controlName);
						// s_pressTimes.emplace(controlName, std::chrono::steady_clock::now());

						Control.dispatch(controlName, true);
					}

					if (button->QReleased()) {
						// s_pressTimes.erase(controlName);
						Control.dispatch(controlName, false);
					}

					// auto it = s_pressTimes.find(controlName);
					// if (it != s_pressTimes.end()) {
					// auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - it->second)
					// 						.count();
					auto ms = button->heldDownSecs * 1000.0F;

					for (auto const& reg : ControlHeld.registered()) {
						if (reg.first != controlName) continue;
						const float threshold = reg.second;
						if (ms < threshold) continue;

						ControlHeld.dispatch(reg);
					}
					// }
				}
			}

			if (original_PerformInputProcessing) {
				original_PerformInputProcessing(self, queueHead);
			}
		}

		inline void InstallInputHook () {
			if (original_PerformInputProcessing) return;
			REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::PlayerControls[0] };
			const auto old = vtbl.write_vfunc(0, &Hook_PerformInputProcessing);
			original_PerformInputProcessing = reinterpret_cast<PerformInputProc>(old);
			spdlog::debug("[Events::Input] Installed");
		}

		inline void UninstallInputHook () {
			if (!original_PerformInputProcessing) return;
			REL::Relocation<std::uintptr_t> vtbl { RE::VTABLE::PlayerControls[0] };
			vtbl.write_vfunc(0, original_PerformInputProcessing);
			original_PerformInputProcessing = nullptr;
			spdlog::debug("[Events::Input] Uninstalled");
		}
	}

}
