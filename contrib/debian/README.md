
Debian
====================
This directory contains files used to package doged/doge-qt
for Debian-based Linux systems. If you compile doged/doge-qt yourself, there are some useful files here.

## bitcoincash: URI support ##


doge-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install doge-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your doge-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin-abc128.png` to `/usr/share/pixmaps`

doge-qt.protocol (KDE)

