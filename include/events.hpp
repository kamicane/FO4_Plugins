#pragma once

namespace Events {

	enum class SaveLoadType { Load, Save };

	template<typename Event, typename Callback>
	class Dispatcher {
	private:
		eventpp::EventDispatcher<Event, Callback> dispatcher;

	protected:

		virtual void RegisterNative () { }

		virtual void UnregisterNative () { }

	public:

		virtual ~Dispatcher () = default;

		template<typename Listener>
		auto on (Event event, Listener&& listener) {
			auto hasListeners = dispatcher.hasAnyListener(event);
			auto handle = dispatcher.appendListener(event, std::forward<Listener>(listener));
			if (!hasListeners) { // first listener
				RegisterNative();
			}
			return handle;
		}

		template<typename Listener>
		void once (Event event, Listener&& listener) {
			using Handle = decltype(dispatcher.appendListener(event, std::declval<Callback>()));
			auto handlePtr = std::make_shared<Handle>();
			*handlePtr = this->on(event, [this, event, handlePtr, listener = std::forward<Listener>(listener)] (auto&&... args) mutable {
				const bool shouldUnregister = static_cast<bool>(listener(std::forward<decltype(args)>(args)...));
				if (shouldUnregister) {
					this->off(event, *handlePtr);
				}
			});
		}

		template<typename Handle>
		requires requires(eventpp::EventDispatcher<Event, Callback> d, Event e, Handle h) { d.removeListener(e, h); }
		void off (Event event, Handle handle) {
			dispatcher.removeListener(event, handle);
			if (!dispatcher.hasAnyListener(event)) {
				UnregisterNative();
			}
		}

		template<typename... Args>
		requires requires(eventpp::EventDispatcher<Event, Callback> d, Event e, Args... a) { d.dispatch(e, a...); }
		void dispatch (Event event, Args&&... args) {
			dispatcher.dispatch(event, std::forward<Args>(args)...);
		}
	};

	inline Dispatcher<uint32_t, void()> Messaging {};

	class MenuOpenCloseDispatcher : public Dispatcher<std::string_view, void(bool)> {
	private:
		struct Sink : RE::BSTEventSink<RE::MenuOpenCloseEvent> {
			MenuOpenCloseDispatcher* parent;

			explicit Sink (MenuOpenCloseDispatcher* a_parent)
				: parent(a_parent) { }

			RE::BSEventNotifyControl ProcessEvent (
				const RE::MenuOpenCloseEvent& event,
				RE::BSTEventSource<RE::MenuOpenCloseEvent>* source
			) override {
				parent->dispatch(event.menuName, event.opening);

				return RE::BSEventNotifyControl::kContinue;
			}
		};

		std::unique_ptr<Sink> sink = nullptr;

	protected:
		void RegisterNative () override {
			auto* ui = RE::UI::GetSingleton();
			if (ui) {
				sink = std::make_unique<Sink>(this);
				ui->RegisterSink<RE::MenuOpenCloseEvent>(sink.get());
			} else {
				spdlog::error("[Events] Fatal error: could not register MenuOpenCloseEvent");
			}
		}

		void UnregisterNative () override {
			if (!sink) return;
			auto* ui = RE::UI::GetSingleton();
			if (ui) {
				ui->UnregisterSink<RE::MenuOpenCloseEvent>(sink.get());
			}
			sink.reset();
		}
	};

	inline MenuOpenCloseDispatcher MenuOpenClose {};

	class SaveLoadDispatcher : public Dispatcher<SaveLoadType, void()> {
	private:
		struct Sink : RE::BSTEventSink<RE::TESLoadGameEvent> {
			SaveLoadDispatcher* parent;

			explicit Sink (SaveLoadDispatcher* a_parent)
				: parent(a_parent) { }

			RE::BSEventNotifyControl ProcessEvent (
				const RE::TESLoadGameEvent& event,
				RE::BSTEventSource<RE::TESLoadGameEvent>* source
			) override {
				parent->dispatch(Events::SaveLoadType::Load);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		std::unique_ptr<Sink> sink = nullptr;

	protected:
		void RegisterNative () override {
			auto* loadGameEventSource = RE::TESLoadGameEvent::GetEventSource();
			if (loadGameEventSource) {
				sink = std::make_unique<Sink>(this);
				loadGameEventSource->RegisterSink(sink.get());
			} else {
				spdlog::error("[Events] Fatal error: could not register TESLoadGameEvent");
			}
		}

		void UnregisterNative () override {
			if (!sink) return;
			auto* loadGameEventSource = RE::TESLoadGameEvent::GetEventSource();
			if (loadGameEventSource) {
				loadGameEventSource->UnregisterSink(sink.get());
			}
			sink.reset();
		}
	};

	inline SaveLoadDispatcher SaveLoad {};
}
