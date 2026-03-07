#pragma once

#include "internal.hpp"

namespace Internal {
	using FormIdSet = std::unordered_set<RE::TESFormID>;
	using ComponentMap = std::unordered_map<RE::TESFormID, double>;
	using ComponentLookup = std::unordered_map<RE::TESFormID, ComponentMap>;

	inline ComponentLookup ComponentLookup_OMOD;
	inline ComponentLookup ComponentLookup_ARMO_WEAP;
	inline ComponentLookup ComponentLookup_MISC;

	inline constexpr double BaseScrapScalarMul = 0.5;
	inline constexpr double DefaultScrapScalar = 1.0;

	inline constexpr RE::TESFormID ScrapFilterKeywordFormId = 0x001CF58B;

	inline double ScaleComponentCount (RE::TESForm* baseForm, double count) {
		if (!baseForm || count == 0) return 0;

		double scrapScalar = DefaultScrapScalar;

		auto formType = baseForm->GetFormType();
		if (formType == RE::ENUM_FORMTYPE::kCMPO) {
			auto* cmpoForm = baseForm->As<RE::BGSComponent>();
			if (!cmpoForm) return 0;

			auto* scrapScalarForm = cmpoForm->modScrapScalar;
			if (scrapScalarForm) scrapScalar = scrapScalarForm->GetValue();
		}

		return BaseScrapScalarMul * scrapScalar * count;
	}

	inline void AddScaledComponentToMap (RE::TESFormID formId, double count, ComponentMap& outMap) {
		auto* baseForm = Internal::GetForm(formId);
		if (!baseForm) return;

		auto scaledCount = ScaleComponentCount(baseForm, count);
		if (scaledCount > 0) {
			outMap[baseForm->formID] += scaledCount;
		}
	}

	inline void CreateLookupMaps () {
		ComponentLookup_OMOD.clear();
		ComponentLookup_ARMO_WEAP.clear();

		auto* data = RE::TESDataHandler::GetSingleton();
		if (!data) return;

		auto* scrapFilterKeyword = Internal::GetForm<RE::BGSKeyword>(ScrapFilterKeywordFormId);

		auto& allCOBJs = data->GetFormArray<RE::BGSConstructibleObject>();

		for (auto* cobjForm : allCOBJs) {
			if (!cobjForm) continue;
			if (!cobjForm->requiredItems) continue;

			ComponentMap cmpoMap;

			auto& required = *cobjForm->requiredItems;
			for (auto& req : required) {
				auto* compForm = req.first;
				if (!compForm) continue;

				auto count = req.second.i;
				if (count == 0) continue;

				cmpoMap[compForm->formID] += count;
			}
			if (cmpoMap.empty()) continue;

			auto* created = cobjForm->GetCreatedItem();
			if (!created) continue;

			if (scrapFilterKeyword && cobjForm->filterKeywords.HasKeyword(scrapFilterKeyword)) {
				auto createdType = created->GetFormType();

				if (createdType == RE::ENUM_FORMTYPE::kFLST) {
					auto* createdList = created->As<RE::BGSListForm>();
					if (!createdList) continue;

					for (auto* listedForm : createdList->arrayOfForms) {
						if (!listedForm) continue;

						auto listedType = listedForm->GetFormType();
						if (listedType != RE::ENUM_FORMTYPE::kWEAP && listedType != RE::ENUM_FORMTYPE::kARMO) continue;

						ComponentLookup_ARMO_WEAP[listedForm->formID] = cmpoMap;
					}
				} else if (createdType == RE::ENUM_FORMTYPE::kWEAP || createdType == RE::ENUM_FORMTYPE::kARMO) {
					ComponentLookup_ARMO_WEAP[created->formID] = cmpoMap;
				} else {
					continue;
				}
			}

			auto* omodForm = created->As<RE::BGSMod::Attachment::Mod>();
			if (!omodForm) continue;

			auto* miscForm = omodForm->GetLooseMod();
			if (!miscForm) continue;

			ComponentLookup_OMOD[omodForm->formID] = cmpoMap;
		}

		auto& allMISCs = data->GetFormArray<RE::TESObjectMISC>();
		for (auto* miscForm : allMISCs) {
			if (!miscForm || !miscForm->componentData) continue;
			ComponentMap cmpoMap;

			auto& components = *miscForm->componentData;
			for (auto& entry : components) {
				auto* componentForm = entry.first;
				if (!componentForm) continue;

				auto cmpoCount = entry.second.i;
				if (cmpoCount == 0) continue;

				cmpoMap[componentForm->formID] += cmpoCount;
			}

			if (!cmpoMap.empty()) ComponentLookup_MISC[miscForm->formID] = cmpoMap;
		}
	}

	inline void GetMiscRefComponents (RE::TESObjectREFR* objectRef, ComponentMap& cmpoMap) {
		if (!objectRef) return;

		auto* base = objectRef->GetBaseObject();
		if (!base) return;

		auto formType = base->GetFormType();
		if (formType != RE::ENUM_FORMTYPE::kMISC) return;

		auto baseCompIt = ComponentLookup_MISC.find(base->formID);
		if (baseCompIt != ComponentLookup_MISC.end()) {
			for (const auto& [compFormId, count] : baseCompIt->second) {
				cmpoMap[compFormId] += count;
			}
		}
	}

	inline void GetEquipmentRefComponents (RE::TESObjectREFR* objectRef, ComponentMap& cmpoMap) {
		if (!objectRef) return;

		auto* base = objectRef->GetBaseObject();
		if (!base) return;

		auto formType = base->GetFormType();
		if (formType != RE::ENUM_FORMTYPE::kWEAP && formType != RE::ENUM_FORMTYPE::kARMO) return;

		auto baseCompIt = ComponentLookup_ARMO_WEAP.find(base->formID);
		if (baseCompIt != ComponentLookup_ARMO_WEAP.end()) {
			for (const auto& [cmpoFormId, count] : baseCompIt->second) {
				AddScaledComponentToMap(cmpoFormId, count, cmpoMap);
			}
		}

		if (!objectRef->extraList) {
			return;
		}

		auto* objInst = objectRef->extraList->GetByType<RE::BGSObjectInstanceExtra>();
		if (!objInst) {
			return;
		}

		auto idxData = objInst->GetIndexData();
		if (idxData.empty()) {
			return;
		}

		for (auto& idx : idxData) {
			if (idx.disabled) continue;

			const RE::TESFormID omodId = idx.objectID;

			auto omodCompIt = ComponentLookup_OMOD.find(omodId);
			if (omodCompIt == ComponentLookup_OMOD.end()) {
				continue;
			}

			for (const auto& [cmpoFormId, count] : omodCompIt->second) {
				AddScaledComponentToMap(cmpoFormId, count, cmpoMap);
			}
		}
	}
}

namespace Papyrus::Scrap {

	inline void TestScrapRef (std::monostate mono, RE::TESObjectREFR* objectRef) {
		spdlog::debug("[Papyrus:TestScrapRef] called target={}", Internal::ToString(objectRef));

		if (!objectRef) return;

		auto* base = objectRef->GetBaseObject();
		if (!base) return;

		auto formType = base->GetFormType();

		Internal::ComponentMap components;

		if (formType == RE::ENUM_FORMTYPE::kWEAP || formType == RE::ENUM_FORMTYPE::kARMO) {
			Internal::GetEquipmentRefComponents(objectRef, components);
		} else if (formType == RE::ENUM_FORMTYPE::kMISC) {
			Internal::GetMiscRefComponents(objectRef, components);
		}

		spdlog::debug("[Papyrus:TestScrapRef] component count={}", components.size());

		size_t index = 0;
		for (auto& [formId, count] : components) {
			auto* cmpoForm = Internal::GetForm(formId);

			spdlog::debug(
				"[Papyrus:TestScrapRef] component[{}] form={} count={}",
				index,
				Internal::ToString(cmpoForm),
				count
			);
			++index;
		}
	}
}
