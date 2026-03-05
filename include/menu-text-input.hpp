#pragma once

#include "events.hpp"
#include "menu-util.hpp"
#include "menu-base.hpp"

namespace Events {
	inline Dispatcher<std::string_view, void(std::string_view)> TextInput {};
}

namespace Menu {
	class TextInput : public Menu::Base<TextInput> {
	public:
		static constexpr const char* MENU_NAME = "TextInputMenu";

		static RE::IMenu* Create (const RE::UIMessage& /* msg */) {
			spdlog::debug("[TextInputMenu] calling create");
			return new TextInput();
		}

		~TextInput () override = default;

		static void Close () {
			if (auto* UIMessageQueue = RE::UIMessageQueue::GetSingleton()) {
				UIMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kHide);
			}
		}

		static void Open (std::string_view title, std::string_view text) {
			TitleValue = title;
			TextValue = text;

			if (auto* UIMessageQueue = RE::UIMessageQueue::GetSingleton()) {
				UIMessageQueue->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kShow);
			}
		}

	private:
		struct InitData {
			std::string title, text;
		};

		std::unique_ptr<RE::BSGFxShaderFXTarget> Background_mc { nullptr };

		inline static std::string TextValue;
		inline static std::string TitleValue;

		std::string textResult;

		TextInput () {
			this->codeObjectNames = { "Initialize", "Confirm", "Cancel", "PlaySound" };

			Dispatcher.once("Initialize", [] (TextInput* self, ASParams) {
				MenuUtil::InvokeMethod(self, "SetText", TextValue);
				MenuUtil::InvokeMethod(self, "SetTitle", TitleValue);

				return true;
			});

			Dispatcher.once("Cancel", [] (TextInput* /* self */, ASParams) {
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
				TextInput::Close();
				return true;
			});

			Dispatcher.once("Confirm", [] (TextInput* self, ASParams params) {
				if (params.size() == 1 && params[0].IsString()) {
					self->textResult = params[0].GetString();
					spdlog::info("[TextInputMenu] Input Confirm called: `{}`", self->textResult);
				}

				TextInput::Close();
				return true;
			});

			menuFlags.set(
				RE::UI_MENU_FLAGS::kCustomRendering,
				RE::UI_MENU_FLAGS::kDisablePauseMenu,
				// RE::UI_MENU_FLAGS::kHasButtonBar, // IDK what this does, but it works with or without
				RE::UI_MENU_FLAGS::kModal,
				RE::UI_MENU_FLAGS::kPausesGame,
				RE::UI_MENU_FLAGS::kSkipRenderDuringFreezeFrameScreenshot,
				RE::UI_MENU_FLAGS::kUpdateUsesCursor,
				RE::UI_MENU_FLAGS::kUsesBlurredBackground,
				RE::UI_MENU_FLAGS::kUsesCursor,
				RE::UI_MENU_FLAGS::kUsesMenuContext
			);

			menuHUDMode.emplace("SpecialMode");
			depthPriority = RE::UI_DEPTH_PRIORITY::kTerminal;

			auto* const ScaleformManager = RE::BSScaleformManager::GetSingleton();
			if (!ScaleformManager) {
				spdlog::error("BSScaleformManager singleton is null - skipping GFx init");
				return;
			}

			const auto LoadMovieSuccess = ScaleformManager->LoadMovieEx(*this, "Interface/TextInputMenu.swf", "root.Menu_mc");
			if (!LoadMovieSuccess) {
				spdlog::error("LoadMovieEx failed - skipping GFx initialization to avoid crash");
				return;
			}

			if (!uiMovie) {
				spdlog::error("uiMovie is null after LoadMovieEx - skipping GFx init");
				return;
			}

			filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.Menu_mc");
			if (filterHolder) {
				filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kPlayerSetColor);
				shaderFXObjects.push_back(filterHolder.get());
			} else {
				spdlog::error("filterHolder is null after construction");
				return;
			}

			Background_mc = std::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.Menu_mc.Background_mc");
			if (Background_mc) {
				Background_mc->EnableShadedBackground(RE::HUDColorTypes::kMenuNoColorBackground);
				shaderFXObjects.push_back(Background_mc.get());
			} else {
				spdlog::warn("Background_mc not found - continuing without background shader");
			}

			SetUpButtonBar(*filterHolder, "ButtonHintBar_mc", RE::HUDColorTypes::kPlayerSetColor);

			MessageDispatcher.once(RE::UI_MESSAGE_TYPE::kShow, [] (TextInput* self) {
				spdlog::debug("[TextInput:MessageDispatcher] kShow");

				if (auto* ControlMap = RE::ControlMap::GetSingleton()) {
					ControlMap->SetTextEntryMode(true);
				}

				self->textResult = "";

				self->SetUIMessageHandled();

				return true;
			});

			MessageDispatcher.once(RE::UI_MESSAGE_TYPE::kHide, [] (TextInput* self) {
				spdlog::debug("[TextInput:MessageDispatcher] kHide");

				if (auto* ControlMap = RE::ControlMap::GetSingleton()) {
					ControlMap->SetTextEntryMode(false);
				}

				Events::TextInput.dispatch("Close", self->textResult);

				self->SetUIMessageHandled();

				return true;
			});

			spdlog::debug("GFx initialization complete");
		}
	};
}
