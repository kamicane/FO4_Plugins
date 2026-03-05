#include "project.h"
#include "util.hpp"
#include "project-main.hpp"
#include "events.hpp"

#define DLLEXPORT __declspec(dllexport)

extern "C" DLLEXPORT constinit auto F4SEPlugin_Version = [] () noexcept {
	F4SE::PluginVersionData data {};

	data.PluginVersion(Project::Version);
	data.PluginName(Project::ID);
	data.AuthorName("kamicane");
	data.UsesAddressLibrary(true);
	data.UsesSigScanning(false);
	data.IsLayoutDependent(true);
	data.HasNoStructUse(false);

	data.CompatibleVersions({ F4SE::RUNTIME_LATEST_AE });

	return data;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query (const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
	info->infoVersion = F4SE::PluginInfo::kVersion;
	info->name = Project::ID.data();
	info->version = Project::Version.pack();

	const auto ver = f4se->RuntimeVersion();
	if (ver != F4SE::RUNTIME_1_10_163) {
		F4SE::log::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

void MessageHandler (F4SE::MessagingInterface::Message* message) {
	Events::Messaging.dispatch(message->type);
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load (const F4SE::LoadInterface* f4se) {
	F4SE::Init(f4se);

	Util::LoadIni(Project::ID);

	auto logger = Util::GetLogger(Project::ID);
	spdlog::set_default_logger(logger);

	spdlog::info("Plugin {} v{} loaded", Project::ID, Project::Version);

	const auto* messaging = F4SE::GetMessagingInterface();
	if (!messaging) {
		spdlog::error("Fatal error: could not get F4SE messaging interface");
		return false;
	}

	messaging->RegisterListener(MessageHandler);

	Project::main();

	return true;
}
