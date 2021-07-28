set_project("LCUI")
set_version("2.2.1")
add_rules("mode.debug", "mode.release", "c++.openmp")
add_includedirs("include")
add_defines("LCUI_EXPORTS")
includes("lib/**/xmake.lua")
includes("test/xmake.lua")
set_warnings("all")

if is_plat("windows") then
    add_defines("_CRT_SECURE_NO_WARNINGS")
end

target("LCUI")
    set_kind("shared")
    add_files("src/*.c")
    add_configfiles("src/config.h.in")
    set_configdir("src")
    add_headerfiles("include/LCUI.h")
    add_headerfiles("include/LCUI_Build.h")
    add_headerfiles("include/(LCUI/**.h)")
    add_deps(
        "builder",
        "css",
        "font",
        "gui",
        "image",
        "paint",
        "platform",
        "text",
        "thread",
        "timer",
        "util",
        "worker"
    )
