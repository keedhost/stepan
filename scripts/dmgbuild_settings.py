# dmgbuild settings for the Stepan installer image.
# Invoked by scripts/make-dmg.sh via:
#   dmgbuild -Dapp=... -Dicon=... -Dbackground=... -s dmgbuild_settings.py "Stepan" out.dmg
import os.path

application = defines.get("app")
appname = os.path.basename(application)

format = "UDZO"
filesystem = "HFS+"

files = [application]
symlinks = {"Applications": "/Applications"}

icon = defines.get("icon")
background = defines.get("background")

show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False

arrange_by = None
icon_size = 128
text_size = 12
label_pos = "bottom"

window_rect = ((400, 150), (660, 500))
default_view = "icon-view"

icon_locations = {
    appname: (165, 226),
    "Applications": (495, 226),
}
