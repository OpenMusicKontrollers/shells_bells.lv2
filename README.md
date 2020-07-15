## Shells Bells

### A just-for-fun LV2 plugin bundle

The most limited, useless, hackable and fun plugin bundle ever. Sound the bell in the shell
(or from any program forked from the latter) to send a MIDI note.

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/shells_bells.lv2/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/shells_bells.lv2/commits/master)

### Binaries

For GNU/Linux (64-bit, 32-bit).

To install the plugin bundle on your system, simply copy the __shells_bells.lv2__
folder out of the platform folder of the downloaded package into your
[LV2 path](http://lv2plug.in/pages/filesystem-hierarchy-standard.html).

#### Stable release

* [shells_bells.lv2-0.4.0.zip](https://dl.open-music-kontrollers.ch/shells_bells.lv2/stable/shells_bells.lv2-0.4.0.zip) ([sig](https://dl.open-music-kontrollers.ch/shells_bells.lv2/stable/shells_bells.lv2-0.4.0.zip.sig))

#### Unstable (nightly) release

* [shells_bells.lv2-latest-unstable.zip](https://dl.open-music-kontrollers.ch/shells_bells.lv2/unstable/shells_bells.lv2-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/shells_bells.lv2/unstable/shells_bells.lv2-latest-unstable.zip.sig))

### Sources

#### Stable release

* [shells_bells.lv2-0.4.0.tar.xz](https://git.open-music-kontrollers.ch/lv2/shells_bells.lv2/snapshot/shells_bells.lv2-0.4.0.tar.xz)
([sig](https://git.open-music-kontrollers.ch/lv2/shells_bells.lv2/snapshot/shells_bells.lv2-0.4.0.tar.xz.asc))

#### Git repository

* <https://git.open-music-kontrollers.ch/lv2/shells_bells.lv2>

<!--
### Packages

* [ArchLinux](https://www.archlinux.org/packages/community/x86_64/shells_bells.lv2/)
-->

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/shells_bells.lv2)
* [Github](https://github.com/OpenMusicKontrollers/shells_bells.lv2)

#### Plugins

![Screenshot](/screenshots/screenshot_1.png)

##### Bells

Its UI drops you into a shell and whenever you sound the bell, a MIDI note
is played back on its DSP side.

#### Dependencies

* [LV2](http://lv2plug.in) (LV2 Plugin Standard)
* [OpenGl](https://www.opengl.org) (OpenGl)
* [GLEW](http://glew.sourceforge.net) (GLEW)
* [VTERM](http://www.leonerd.org.uk/code/libvterm) (Virtual terminal emulator)

#### Build / install

	git clone https://git.open-music-kontrollers.ch/lv2/shells_bells.lv2
	cd shells_bells.lv2
	meson build
	cd build
	ninja -j4
	ninja test
	sudo ninja install

#### UI

On hi-DPI displays, the UI scales automatically if you have set the correct DPI
in your ~/.Xresources.

    Xft.dpi: 200

If not, you can manually set your DPI via environmental variable *D2TK_SCALE*:

    export D2TK_SCALE=200

#### License

Copyright (c) 2019-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)

This is free software: you can redistribute it and/or modify
it under the terms of the Artistic License 2.0 as published by
The Perl Foundation.

This source is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Artistic License 2.0 for more details.

You should have received a copy of the Artistic License 2.0
along the source as a COPYING file. If not, obtain it from
<http://www.perlfoundation.org/artistic_license_2_0>.
