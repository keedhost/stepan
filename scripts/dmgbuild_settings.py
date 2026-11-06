# Stepan — твій друг в системі координат
# Copyright (C) 2026 Andrii Kondratiev
#
# Слава Україні! Слава ЗСУ!
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

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
