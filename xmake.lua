set_project("lcui")
set_version("2.2.1")
add_rules("mode.debug", "mode.release", "c++.openmp", "mode.coverage")
add_includedirs("include")
add_defines("LCUI_EXPORTS", "_UNICODE")
includes("lib/**/xmake.lua")
set_warnings("all")

if is_plat("windows") then
    add_defines("_CRT_SECURE_NO_WARNINGS")
else
    add_cxflags("-fPIC")
end

if is_mode("release") then
    set_symbols("none")
end

target("lcui")
    set_kind("shared")
    add_files("src/*.c")
    add_configfiles("src/config.h.in")
    set_configdir("src")
    add_headerfiles("include/LCUI.h")
    add_headerfiles("include/LCUI_Build.h")
    add_headerfiles("include/(LCUI/**.h)")
    add_deps(
        "lcui-util",
        "lcui-thread",
        "lcui-css",
        "lcui-font",
        "lcui-paint",
        "lcui-image",
        "lcui-gui",
        "lcui-platform",
        "lcui-text",
        "lcui-builder",
        "lcui-timer",
        "lcui-worker"
    )
