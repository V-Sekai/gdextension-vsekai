import os, sys, platform, json, subprocess
import SCons
from SCons.Variables import BoolVariable

def add_sources(sources, dirpath, extension):
    for f in os.listdir(dirpath):
        if f.endswith("." + extension):
            sources.append(dirpath + "/" + f)

def replace_flags(flags, replaces):
    for k, v in replaces.items():
        if k not in flags:
            continue
        if v is None:
            flags.remove(k)
        else:
            flags[flags.index(k)] = v

def validate_godotcpp_dir(key, val, env):
    normalized = val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)
    if not os.path.isdir(normalized):
        raise UserError("GDExtension directory ('%s') does not exist: %s" % (key, val))

env = Environment()
opts = Variables(["customs.py"], ARGUMENTS)
opts.Add(EnumVariable("godot_version", "The Godot target version", "4.1", ["3", "4.0", "4.1"]))
opts.Add(
    PathVariable(
        "godot_cpp",
        "Path to the directory containing Godot CPP folder",
        None,
        validate_godotcpp_dir,
    )
)
opts.Update(env)

# Minimum target platform versions.
if "ios_min_version" not in ARGUMENTS:
    ARGUMENTS["ios_min_version"] = "11.0"
if "macos_deployment_target" not in ARGUMENTS:
    ARGUMENTS["macos_deployment_target"] = "11.0"
if "android_api_level" not in ARGUMENTS:
    ARGUMENTS["android_api_level"] = "28"

# Recent godot-cpp versions disables exceptions by default, but libdatachannel requires them.
ARGUMENTS["disable_exceptions"] = "no"

# Load godot-cpp SConstruct
sconstruct_cpp = "thirdparty/godot-cpp/SConstruct"
cpp_env = SConscript(sconstruct_cpp)
env = cpp_env.Clone()

# Load godot-steam-audio SConstruct
sconstruct_steam_audio = "thirdparty/godot-steam-audio/SCSub"

env.Append(CPPPATH=["thirdparty/godot-steam-audio/src"])

if env.get("CC", "").lower() == "cl":
    # Building with MSVC
    env.AppendUnique(CCFLAGS=("/I",  "src/lib/steamaudio/unity/include/phonon/"))
else:
    env.AppendUnique(CCFLAGS=("-isystem",  "src/lib/steamaudio/unity/include/phonon/"))

sources = Glob("src/*.cpp")

steam_audio_lib_path = env.get("STEAM_AUDIO_LIB_PATH", "thirdparty/godot-steam-audio/src/lib")

if env["platform"] == "linux":
    env.Append(LIBPATH=[f'{steam_audio_lib_path}/linux-x64'])
    env.Append(LIBS=["libphonon.so"])
elif env["platform"] == "windows":
    env.Append(LIBPATH=[f'{steam_audio_lib_path}/windows-x64'])
    env.Append(LIBS=["phonon"])
elif env["platform"] == "macos":
    env.Append(LIBPATH=[f'{steam_audio_lib_path}/osx'])
    env.Append(LIBS=["libphonon.dylib"])

library = env.SharedLibrary(
    "project/addons/godot-steam-audio/bin/godot-steam-audio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)

if cpp_env.get("is_msvc", False):
    # Make sure we don't build with static cpp on MSVC (default in recent godot-cpp versions).
    replace_flags(env["CCFLAGS"], {"/MT": "/MD"})
    replace_flags(cpp_env["CCFLAGS"], {"/MT": "/MD"})

# Should probably go to upstream godot-cpp.
# We let SCons build its default ENV as it includes OS-specific things which we don't
# want to have to pull in manually.
# Then we prepend PATH to make it take precedence, while preserving SCons' own entries.
env.PrependENVPath("PATH", os.getenv("PATH"))
env.PrependENVPath("PKG_CONFIG_PATH", os.getenv("PKG_CONFIG_PATH"))
if "TERM" in os.environ:  # Used for colored output.
    env["ENV"]["TERM"] = os.environ["TERM"]

# Patch mingw SHLIBSUFFIX.
if env["platform"] == "windows" and env["use_mingw"]:
    env["SHLIBSUFFIX"] = ".dll"

# Patch OSXCross config.
if env["platform"] == "macos" and os.environ.get("OSXCROSS_ROOT", ""):
    env["SHLIBSUFFIX"] = ".dylib"
    if env["macos_deployment_target"] != "default":
        env["ENV"]["MACOSX_DEPLOYMENT_TARGET"] = env["macos_deployment_target"]

opts.Update(env)

target = env["target"]
if env["godot_version"] == "3":
    result_path = os.path.join("bin", "gdnative", "webrtc" if env["target"] == "release" else "webrtc_debug")
elif env["godot_version"] == "4.0":
    result_path = os.path.join("bin", "extension-4.0", "webrtc")
else:
    result_path = os.path.join("bin", "extension-4.1", "webrtc")

# Our includes and sources
env.Append(CPPPATH=["thirdparty/godot-cpp/include/", "thirdparty/godot-cpp/include/godot_cpp", "thirdparty/gdextension_webrtc", "thirdparty/gdextension_webrtc/net"])
env.Append(CPPDEFINES=["RTC_STATIC"])
sources = []
sources.append(
    [
        "thirdparty/gdextension_webrtc/WebRTCLibDataChannel.cpp",
        "thirdparty/gdextension_webrtc/WebRTCLibPeerConnection.cpp",
    ]
)
if env["godot_version"] == "3":
    env.Append(CPPDEFINES=["GDNATIVE_WEBRTC"])
    sources.append("src/init_gdnative.cpp")
    add_sources(sources, "thirdparty/gdextension_webrtc/src/net/", "cpp")
else:
    sources.append("src/init_gdextension.cpp")
    if env["godot_version"] == "4.0":
        env.Append(CPPDEFINES=["GDEXTENSION_WEBRTC_40"])

# Add our build tools
for tool in ["openssl", "cmake", "rtc"]:
    env.Tool(tool, toolpath=["tools"])

ssl = env.OpenSSL()

rtc = env.BuildLibDataChannel(ssl)

# Forces building our sources after OpenSSL and libdatachannel.
# This is because OpenSSL headers are generated by their build system and SCons doesn't know about them.
# Note: This might not be necessary in this specific case since our sources doesn't include OpenSSL headers directly,
# but it's better to be safe in case of indirect inclusions by one of our other dependencies.
env.Depends(sources, ssl + rtc)

# We want to statically link against libstdc++ on Linux to maximize compatibility, but we must restrict the exported
# symbols using a GCC version script, or we might end up overriding symbols from other libraries.
# Using "-fvisibility=hidden" will not work, since libstdc++ explicitly exports its symbols.
symbols_file = None
if env["platform"] == "linux" or (
    env["platform"] == "windows" and env.get("use_mingw", False) and not env.get("use_llvm", False)
):
    if env["godot_version"] == "3":
        symbols_file = env.File("misc/gcc/symbols-gdnative.map")
    else:
        symbols_file = env.File("misc/gcc/symbols-extension.map")
    env.Append(
        LINKFLAGS=[
            "-Wl,--no-undefined,--version-script=" + symbols_file.abspath,
            "-static-libgcc",
            "-static-libstdc++",
        ]
    )
    env.Depends(sources, symbols_file)

# Make the shared library
result_name = "libwebrtc_native{}{}".format(env["suffix"], env["SHLIBSUFFIX"])
if env["godot_version"] != "3" and env["platform"] == "macos":
    framework_path = os.path.join(
        result_path, "lib", "libwebrtc_native.macos.{}.{}.framework".format(env["target"], env["arch"])
    )
    library_file = env.SharedLibrary(target=os.path.join(framework_path, result_name), source=sources)
    plist_file = env.Substfile(
        os.path.join(framework_path, "Resources", "Info.plist"),
        "misc/dist/macos/Info.plist",
        SUBST_DICT={"{LIBRARY_NAME}": result_name, "{DISPLAY_NAME}": "libwebrtc_native" + env["suffix"]},
    )
    library = [library_file, plist_file]
else:
    library = env.SharedLibrary(target=os.path.join(result_path, "lib", result_name), source=sources)

Default(library)

# GDNativeLibrary
if env["godot_version"] == "3":
    gdnlib = "webrtc" if target != "debug" else "webrtc_debug"
    ext = ".tres"
    extfile = env.Substfile(
        os.path.join(result_path, gdnlib + ext),
        "misc/webrtc" + ext,
        SUBST_DICT={
            "{GDNATIVE_PATH}": gdnlib,
            "{TARGET}": "template_" + env["target"],
        },
    )
else:
    extfile = env.Substfile(
        os.path.join(result_path, "webrtc.gdextension"),
        "misc/webrtc.gdextension",
        SUBST_DICT={"{GODOT_VERSION}": env["godot_version"]},
    )

Default(extfile)
