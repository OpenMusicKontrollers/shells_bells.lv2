/*
 * Copyright (c) 2019-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _SHELLS_BELLS_LV2_H
#define _SHELLS_BELLS_LV2_H

#include <stdint.h>
#if !defined(_WIN32)
#	include <sys/mman.h>
#else
#	define mlock(...)
#	define munlock(...)
#endif

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#include <props.h>

#define SHELLS_BELLS_URI    "http://open-music-kontrollers.ch/lv2/shells_bells"
#define SHELLS_BELLS_PREFIX SHELLS_BELLS_URI "#"

// plugin uris
#define SHELLS_BELLS__bells         SHELLS_BELLS_PREFIX "bells"

// plugin UI uris
#define SHELLS_BELLS__ui            SHELLS_BELLS_PREFIX "ui"

// plugin properties
#define SHELLS_BELLS__channel       SHELLS_BELLS_PREFIX "channel"
#define SHELLS_BELLS__note          SHELLS_BELLS_PREFIX "note"
#define SHELLS_BELLS__velocity      SHELLS_BELLS_PREFIX "velocity"
#define SHELLS_BELLS__duration      SHELLS_BELLS_PREFIX "duration"
#define SHELLS_BELLS__trigger       SHELLS_BELLS_PREFIX "trigger"
#define SHELLS_BELLS__fontHeight    SHELLS_BELLS_PREFIX "fontHeight"

#define MAX_NPROPS 6

typedef struct _plugstate_t plugstate_t;

struct _plugstate_t {
	int32_t channel;
	int32_t note;
	int32_t velocity;
	int32_t duration;
	int32_t trigger;
	int32_t font_height;
};

#endif // _SHELLS_BELLS_LV2_H
