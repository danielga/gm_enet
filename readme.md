gm_enet
=======

A module for Garry's Mod that adds bindings for ENet.

Info
-------

This project is composed by the main module (lanes.lua) and the internal module (gm_lanes_core).
You can also refer to this project as gm_lanes or Lanes for Garry's Mod.
lanes.lua goes into lua/includes/modules and gm[cl/sv]\_lanes_core\_[win32/linux/macos].dll goes into lua/bin.

Mac was not tested at all (sorry but I'm cheap and lazy).

If stuff starts erroring or fails to work, be sure to check the correct line endings (\n and such) are present in the files for each OS.

This project requires garrysmod_common (https://bitbucket.org/danielga/garrysmod_common), a framework to facilitate the creation of compilations files (Visual Studio, make, XCode, etc). Simply set the environment variable (GARRYSMOD_COMMON) or the premake option (gmcommon) to the path of your local copy of garrysmod_common.

Credits
-------
ENet
http://enet.bespin.org

lua-enet
http://leafo.net/lua-enet