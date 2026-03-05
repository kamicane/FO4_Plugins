#pragma once

#include "events.hpp"

namespace Menu {
	template<typename Derived>
	class Base : public RE::GameMenuBase {
	protected:
		using ASParams = std::span<const RE::Scaleform::GFx::Value>;

		std::vector<std::string> codeObjectNames;

		bool uiMessageHandled = false;

		Events::Dispatcher<std::string_view, void(Derived*, ASParams)> Dispatcher;
		Events::Dispatcher<RE::UI_MESSAGE_TYPE, void(Derived*)> MessageDispatcher;

	public:
		void SetUIMessageHandled () {
			this->uiMessageHandled = true;
		}

		static void Register () {
			if (auto* ui = RE::UI::GetSingleton()) {
				ui->RegisterMenu(Derived::MENU_NAME, &Derived::Create, nullptr);
				spdlog::debug("[Menu::Base] Registered menu {}", Derived::MENU_NAME);
			}
		}

		template<class T>
		static constexpr bool always_false = false;

		template<typename T>
		RE::Scaleform::GFx::Value MakeValue (T&& val) {
			RE::Scaleform::GFx::Value out;
			using U = std::decay_t<T>;
			if constexpr (std::is_same_v<U, bool>) {
				out = val;
			} else if constexpr (std::is_integral_v<U> || std::is_floating_point_v<U>) {
				out = static_cast<double>(val);
			} else if constexpr (std::is_same_v<U, std::string>) {
				if (uiMovie && uiMovie->asMovieRoot) {
					uiMovie->asMovieRoot->CreateString(&out, val.c_str());
				}
			} else if constexpr (std::is_convertible_v<U, const char*>) {
				if (uiMovie && uiMovie->asMovieRoot) {
					uiMovie->asMovieRoot->CreateString(&out, static_cast<const char*>(val));
				}
			} else if constexpr (std::is_same_v<U, std::string_view>) {
				const std::string tmp { std::forward<T>(val) };
				if (uiMovie && uiMovie->asMovieRoot) {
					uiMovie->asMovieRoot->CreateString(&out, tmp.c_str());
				}
			} else {
				static_assert(always_false<T>, "Unsupported arg type for InvokeMethod");
			}
			return out;
		}

		RE::UI_MESSAGE_RESULTS ProcessMessage (RE::UIMessage& message) override {
			this->uiMessageHandled = false;
			this->MessageDispatcher.dispatch(*message.type, static_cast<Derived*>(this));

			if (!this->uiMessageHandled) {
				return RE::IMenu::ProcessMessage(message);
			}

			return RE::UI_MESSAGE_RESULTS::kHandled;
		}

		void MapCodeObjectFunctions () override {
			for (std::uint32_t i = 0; i < this->codeObjectNames.size(); ++i) {
				MapCodeMethodToASFunction(this->codeObjectNames[i].c_str(), i);
			}
		}

		// Default Call implementation: dispatch through internal dispatcher with method name
		void Call (const RE::Scaleform::GFx::FunctionHandler::Params& params) override {
			const auto rawId = *reinterpret_cast<const std::uint32_t*>(&params.userData);

			if (rawId < this->codeObjectNames.size()) {
				const RE::Scaleform::GFx::Value* ptr = params.args;
				const auto cnt = static_cast<std::size_t>(params.argCount);
				auto span = ASParams(ptr, cnt);
				this->Dispatcher.dispatch(this->codeObjectNames[rawId], static_cast<Derived*>(this), span);
			}
		}
	};
}
