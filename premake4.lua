solution "OpenClam"
project "OpenClam"
	language "C"
	kind "ConsoleApp"
	location "premake_files"
	targetdir "bin"
	objdir "obj"
	files {
		"src/main.c",
		"src/builtin_cmd.c",
		"src/builtin_cmd.h",
		"src/cmd.c",
		"src/cmd.h",
		"src/cmd_parser.c",
		"src/cmd_parser.h",
		"src/job.c",
		"src/job.h",
		"src/process.c",
		"src/process.h",
		"src/shell.c",
		"src/shell.h",
		"src/token.c",
		"src/token.h"
	}

	configurations { "Debug", "Release" }
	configuration "Debug"
		buildoptions { "-O0", "-g", "-Wall", "-D_GNU_SOURCE=1" }
		targetname "debug"

	configuration "Release"
		buildoptions { "-O3", "-Wall", "-D_GNU_SOURCE=1" }
		targetname "release"

