workspace "megu-memleakfix"
   configurations { "Release", "Debug" }
   architecture "x86"
   location "build"

project "megu-memleakfix"
  kind "SharedLib"
  language "C++"
  targetdir "bin/%{cfg.buildcfg}"
  targetname "GettingUp.MemLeakFix"
  targetextension ".asi"

  includedirs { "source" }

  files { "source/dllmain.h", "source/dllmain.cpp" }

  characterset ("Unicode")

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
    staticruntime "On"
