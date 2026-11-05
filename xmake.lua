set_xmakever("2.9.8")
set_project("WinMMWRP")

set_allowedplats("windows")
set_allowedmodes("debug", "release")
set_allowedarchs("x86", "x86_64", "x64", "arm64")
	
add_rules("mode.release", "mode.debug")
set_languages("clatest", "cxxlatest")
set_runtimes("stdc++_static")

option("useclang")
    set_default("false")
    set_showmenu(true)
option_end()

target("WinMMWRP")
	set_kind("shared")
	set_basename("winmm")
	set_options("useclang")

	if is_mode("debug") then
		add_defines("DEBUG")
		add_defines("_DEBUG")
		set_symbols("debug")
		set_optimize("none")
	else	
		add_defines("NDEBUG")
		set_symbols("hidden")
		set_optimize("fastest")
		set_strip("all")
	end

	if has_config("useclang") then
		set_toolchains("clang-cl")
	else
		set_toolchains("mingw")
	end
	
	add_ldflags("-j")
	add_cxflags("-Wall")

	add_includedirs("inc")
	add_files("src/*.cpp")
    add_files("*.def")

	if is_plat("windows") then	
		set_toolchains("mingw")

		-- Remove lib prefix
		set_prefixname("")

		add_shflags("-static", { force = true })
		add_syslinks("uuid", "shlwapi", "ole32")
		add_defines("_WIN32", "_WINXP", "_WIN32_WINNT=0x6000")

		if is_mode("debug") then 
			add_syslinks("-l:libwinpthread.a")
		end

		if is_arch("x86") then 
			add_shflags("legacyExports.def", { force = true })
		else 
			add_shflags("newExports.def", { force = true })
		end
	else
		-- This is Win32 only
		remove_files("src/*.cpp")
	end
target_end()

-- Build every architecture in one go: `xmake buildall [-m debug|release]`
task("buildall")
	set_menu {
		usage = "xmake buildall [options]",
		description = "Build both x86 and x64",
		options = {
			{'m', "mode", "kv", "release", "Build mode (debug or release)"}
		}
	}
	on_run(function ()
		import("core.base.option")
		local mode = option.get("mode") or "release"
		for _, arch in ipairs({"x86", "x64"}) do
			-- Prefer the MSYS2 mingw SDKs; xmake may otherwise pick Git's bundled mingw64
			local sdk = (arch == "x86") and "C:/msys64/mingw32" or "C:/msys64/mingw64"
			if os.isdir(sdk) then
				os.exec("xmake f -p windows -a %s -m %s --mingw=%s -y", arch, mode, sdk)
			else
				os.exec("xmake f -p windows -a %s -m %s -y", arch, mode)
			end
			os.exec("xmake build -y")
		end
	end)
task_end()
