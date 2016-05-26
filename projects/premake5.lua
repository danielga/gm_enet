newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
if gmcommon == nil then
	error("you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
end

include(gmcommon)

local ENET_DIRECTORY = "../enet"

CreateWorkspace({name = "enet"})
	CreateProject({serverside = true})
		includedirs(ENET_DIRECTORY .. "/include")
		links("enet")
		IncludeLuaShared()

		filter("system:windows")
			links({"ws2_32", "winmm"})

		filter("system:linux or macosx")
			defines({
				"HAS_GETADDRINFO",
				"HAS_GETNAMEINFO",
				"HAS_GETHOSTBYADDR_R",
				"HAS_GETHOSTBYNAME_R",
				"HAS_POLL",
				"HAS_FCNTL",
				"HAS_INET_PTON",
				"HAS_INET_NTOP",
				"HAS_MSGHDR_FLAGS",
				"HAS_SOCKLEN_T"
			})

	CreateProject({serverside = false})
		includedirs(ENET_DIRECTORY .. "/include")
		links("enet")
		IncludeLuaShared()

		filter("system:windows")
			links({"ws2_32", "winmm"})

		filter("system:linux or macosx")
			defines({
				"HAS_GETADDRINFO",
				"HAS_GETNAMEINFO",
				"HAS_GETHOSTBYADDR_R",
				"HAS_GETHOSTBYNAME_R",
				"HAS_POLL",
				"HAS_FCNTL",
				"HAS_INET_PTON",
				"HAS_INET_NTOP",
				"HAS_MSGHDR_FLAGS",
				"HAS_SOCKLEN_T"
			})

	project("enet")
		kind("StaticLib")
		includedirs(ENET_DIRECTORY .. "/include")
		vpaths({
			["Header files"] = ENET_DIRECTORY .. "/**.h",
			["Source files"] = ENET_DIRECTORY .. "/**.c"
		})
		files({
			ENET_DIRECTORY .. "/callbacks.c",
			ENET_DIRECTORY .. "/compress.c",
			ENET_DIRECTORY .. "/host.c",
			ENET_DIRECTORY .. "/list.c",
			ENET_DIRECTORY .. "/packet.c",
			ENET_DIRECTORY .. "/peer.c",
			ENET_DIRECTORY .. "/protocol.c"
		})

		filter("system:windows")
			files(ENET_DIRECTORY .. "/win32.c")
			links({"ws2_32", "winmm"})

		filter("system:linux or macosx")
			defines({
				"HAS_GETADDRINFO",
				"HAS_GETNAMEINFO",
				"HAS_GETHOSTBYADDR_R",
				"HAS_GETHOSTBYNAME_R",
				"HAS_POLL",
				"HAS_FCNTL",
				"HAS_INET_PTON",
				"HAS_INET_NTOP",
				"HAS_MSGHDR_FLAGS",
				"HAS_SOCKLEN_T"
			})
			files(ENET_DIRECTORY .. "/unix.c")
