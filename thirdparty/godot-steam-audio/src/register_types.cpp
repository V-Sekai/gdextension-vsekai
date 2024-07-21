#include "register_types.hpp"

#include "config.hpp"
#include "geometry.hpp"
#include "geometry_dynamic.hpp"
#include "listener.hpp"
#include "material.hpp"
#include "player.hpp"
#include "server.hpp"
#include "stream.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void init_ext(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<SteamAudioStreamPlayback>();
		ClassDB::register_class<SteamAudioStream>();
		ClassDB::register_class<SteamAudioListener>();
		ClassDB::register_class<SteamAudioGeometry>();
		ClassDB::register_class<SteamAudioDynamicGeometry>();
		ClassDB::register_class<SteamAudioMaterial>();
		ClassDB::register_class<SteamAudioConfig>();
		ClassDB::register_class<SteamAudioPlayer>();
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(SteamAudioServer);
		auto sa = memnew(SteamAudioServer);
	}
}

void uninit_ext(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE && p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
}
