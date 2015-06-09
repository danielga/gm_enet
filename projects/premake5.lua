newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://bitbucket.org/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common dir"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
if gmcommon == nil then
	error("you didn't provide a path to your garrysmod_common (https://bitbucket.org/danielga/garrysmod_common) directory")
end

include(gmcommon)

CreateSolution("enet")
	CreateProject(SERVERSIDE)
		includedirs({"../enet/include"})
		vpaths({["enet"] = "../enet/*.c"})
		files({
			"../enet/callbacks.c",
			"../enet/compress.c",
			"../enet/host.c",
			"../enet/list.c",
			"../enet/packet.c",
			"../enet/peer.c",
			"../enet/protocol.c"
		})
		IncludeLuaShared()

		SetFilter(FILTER_WINDOWS)
			files({"../enet/win32.c"})
			links({"ws2_32", "winmm"})

		SetFilter(FILTER_LINUX, FILTER_MACOSX)
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
			files({"../enet/unix.c"})

	CreateProject(CLIENTSIDE)
		includedirs({"../enet/include"})
		vpaths({["enet"] = "../enet/*.c"})
		files({
			"../enet/callbacks.c",
			"../enet/compress.c",
			"../enet/host.c",
			"../enet/list.c",
			"../enet/packet.c",
			"../enet/peer.c",
			"../enet/protocol.c"
		})
		IncludeLuaShared()

		SetFilter(FILTER_WINDOWS)
			files({"../enet/win32.c"})
			links({"ws2_32", "winmm"})

		SetFilter(FILTER_LINUX, FILTER_MACOSX)
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
			files({"../enet/unix.c"})