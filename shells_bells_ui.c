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

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

#include <shells_bells.h>

#define SER_ATOM_IMPLEMENTATION
#include <ser_atom.lv2/ser_atom.h>

#include <d2tk/hash.h>
#include <d2tk/frontend_pugl.h>

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	d2tk_pugl_config_t config;
	d2tk_frontend_t *dpugl;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	PROPS_T(props, MAX_NPROPS);

	plugstate_t state;
	plugstate_t stash;

	LV2_URID atom_eventTransfer;
	LV2_URID midi_MidiEvent;
	LV2_URID shells_bells_channel;
	LV2_URID shells_bells_note;
	LV2_URID shells_bells_velocity;
	LV2_URID shells_bells_duration;
	LV2_URID shells_bells_trigger;

	d2tk_coord_t header_height;
	d2tk_coord_t footer_height;
	d2tk_coord_t font_height;

	int done;
};

static void
_intercept_trigger(void *data __attribute__((unused)),
	int64_t frames __attribute__((unused)),
	props_impl_t *impl __attribute__((unused)))
{
	// nothing to do
}

static void
_message_get(plughandle_t *handle, LV2_URID key)
{
	ser_atom_t ser;
	props_impl_t *impl = _props_impl_get(&handle->props, key);
	if(!impl)
	{
		return;
	}

	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 1;

	props_get(&handle->props, &handle->forge, 0, key, &ref);

	const LV2_Atom_Event *ev = (const LV2_Atom_Event *)ser_atom_get(&ser);
	const LV2_Atom *atom = &ev->body;
	handle->writer(handle->controller, 0, lv2_atom_total_size(atom),
		handle->atom_eventTransfer, atom);

	ser_atom_deinit(&ser);
}

static void
_message_set(plughandle_t *handle, LV2_URID key)
{
	ser_atom_t ser;
	props_impl_t *impl = _props_impl_get(&handle->props, key);
	if(!impl)
	{
		return;
	}

	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 1;

	props_set(&handle->props, &handle->forge, 0, key, &ref);

	const LV2_Atom_Event *ev = (const LV2_Atom_Event *)ser_atom_get(&ser);
	const LV2_Atom *atom = &ev->body;
	handle->writer(handle->controller, 0, lv2_atom_total_size(atom),
		handle->atom_eventTransfer, atom);

	ser_atom_deinit(&ser);
}

static inline void
_expose_header(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	const d2tk_coord_t frac [3] = { 1, 1, 1 };
	D2TK_BASE_LAYOUT(rect, 3, frac, D2TK_FLAG_LAYOUT_X_REL, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				d2tk_base_label(base, -1, "Open•Music•Kontrollers", 0.5f, lrect,
					D2TK_ALIGN_LEFT | D2TK_ALIGN_TOP);
			} break;
			case 1:
			{
				d2tk_base_label(base, -1, "SHells•Bells", 1.f, lrect,
					D2TK_ALIGN_CENTER | D2TK_ALIGN_TOP);
			} break;
			case 2:
			{
				d2tk_base_label(base, -1, "Version "SHELLS_BELLS_VERSION, 0.5f, lrect,
					D2TK_ALIGN_RIGHT | D2TK_ALIGN_TOP);
			} break;
		}
	}
}

static inline void
_expose_term(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	char *shell = getenv("SHELL");

	char *args [] = {
		shell ? shell : "/bin/sh",
		NULL
	};

	D2TK_BASE_PTY(base, D2TK_ID, args, handle->font_height, rect, false, pty)
	{
		const d2tk_state_t state =  d2tk_pty_get_state(pty);

		if(d2tk_state_is_close(state))
		{
			handle->done = 1;
		}

		if(d2tk_state_is_bell(state))
		{
			handle->state.trigger = true;

			_message_set(handle, handle->shells_bells_trigger);
		}
	}
}

static inline void
_expose_footer(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	D2TK_BASE_TABLE(rect, 4, 2, D2TK_FLAG_TABLE_REL, tab)
	{
		const unsigned x = d2tk_table_get_index_x(tab);
		const unsigned y = d2tk_table_get_index_y(tab);
		const d2tk_rect_t *trect = d2tk_table_get_rect(tab);

		switch(y)
		{
			case 0:
			{
				static const char *lbls [4] = {
					"Channel",
					"Note",
					"Velocity",
					"Duration (ms)"
				};

				d2tk_base_label(base, -1, lbls[x], 0.5f, trect,
					D2TK_ALIGN_MIDDLE | D2TK_ALIGN_RIGHT);
			} break;
			case 1:
			{
				switch(x)
				{
					case 0:
					{
						if(d2tk_base_prop_int32_is_changed(base, D2TK_ID, trect,
							0x0, &handle->state.channel, 0xf))
						{
							_message_set(handle, handle->shells_bells_channel);
						}
					} break;
					case 1:
					{
						if(d2tk_base_prop_int32_is_changed(base, D2TK_ID, trect,
							0x0, &handle->state.note, 0x7f))
						{
							_message_set(handle, handle->shells_bells_note);
						}
					} break;
					case 2:
					{
						if(d2tk_base_prop_int32_is_changed(base, D2TK_ID, trect,
							0x0, &handle->state.velocity, 0x7f))
						{
							_message_set(handle, handle->shells_bells_velocity);
						}
					} break;
					case 3:
					{
						if(d2tk_base_prop_int32_is_changed(base, D2TK_ID, trect,
							0, &handle->state.duration, 10000))
						{
							_message_set(handle, handle->shells_bells_duration);
						}
					} break;
				}
			} break;
		}
	}
}

static int
_expose(void *data, d2tk_coord_t w, d2tk_coord_t h)
{
	plughandle_t *handle = data;
	const d2tk_rect_t rect = D2TK_RECT(0, 0, w, h);

	const d2tk_coord_t frac [3] = { handle->header_height, 0, handle->footer_height};
	D2TK_BASE_LAYOUT(&rect, 3, frac, D2TK_FLAG_LAYOUT_Y_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				_expose_header(handle, lrect);
			} break;
			case 1:
			{
				_expose_term(handle, lrect);
			} break;
			case 2:
			{
				_expose_footer(handle, lrect);
			} break;
		}
	}

	return 0;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor,
	const char *plugin_uri,
	const char *bundle_path __attribute__((unused)),
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
	}

	if(!parent)
	{
		fprintf(stderr,
			"%s: Host does not support ui:parent\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
	{
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);
	}

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->atom_eventTransfer = handle->map->map(handle->map->handle,
		LV2_ATOM__eventTransfer);
	handle->midi_MidiEvent = handle->map->map(handle->map->handle,
		LV2_MIDI__MidiEvent);
	handle->shells_bells_channel = handle->map->map(handle->map->handle,
		SHELLS_BELLS__channel);
	handle->shells_bells_note = handle->map->map(handle->map->handle,
		SHELLS_BELLS__note);
	handle->shells_bells_velocity = handle->map->map(handle->map->handle,
		SHELLS_BELLS__velocity);
	handle->shells_bells_duration = handle->map->map(handle->map->handle,
		SHELLS_BELLS__duration);
	handle->shells_bells_trigger = handle->map->map(handle->map->handle,
		SHELLS_BELLS__trigger);

	if(!props_init(&handle->props, plugin_uri,
		defs, MAX_NPROPS, &handle->state, &handle->stash,
		handle->map, handle))
	{
		fprintf(stderr, "failed to initialize property structure\n");
		free(handle);
		return NULL;
	}

	handle->controller = controller;
	handle->writer = write_function;

	const d2tk_coord_t w = 800;
	const d2tk_coord_t h = 800;

	d2tk_pugl_config_t *config = &handle->config;
	config->parent = (uintptr_t)parent;
	config->bundle_path = bundle_path;
	config->min_w = w/2;
	config->min_h = h/2;
	config->w = w;
	config->h = h;
	config->fixed_size = false;
	config->fixed_aspect = false;
	config->expose = _expose;
	config->data = handle;

	handle->dpugl = d2tk_pugl_new(config, (uintptr_t *)widget);
	if(!handle->dpugl)
	{
		free(handle);
		return NULL;
	}

	const float scale = d2tk_frontend_get_scale(handle->dpugl);
	handle->header_height = 32 * scale;
	handle->footer_height = 64 * scale;
	handle->font_height = 16 * scale;

	if(host_resize)
	{
		host_resize->ui_resize(host_resize->handle, w, h);
	}

	_message_get(handle, handle->shells_bells_channel);
	_message_get(handle, handle->shells_bells_note);
	_message_get(handle, handle->shells_bells_velocity);
	_message_get(handle, handle->shells_bells_duration);
	_message_get(handle, handle->shells_bells_trigger);

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	d2tk_frontend_free(handle->dpugl);

	free(handle);
}

static void
port_event(LV2UI_Handle instance, uint32_t index __attribute__((unused)),
	uint32_t size __attribute__((unused)), uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if(protocol != handle->atom_eventTransfer)
	{
		return;
	}

	const LV2_Atom_Object *obj = buf;

	ser_atom_t ser;
	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 0;
	props_advance(&handle->props, &handle->forge, 0, obj, &ref);

	ser_atom_deinit(&ser);

	d2tk_frontend_redisplay(handle->dpugl);
}

static int
_idle(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);
	d2tk_style_t style = *d2tk_base_get_default_style(base);
	/*
	style.fill_color[D2TK_TRIPLE_ACTIVE] = handle.dark_reddest;
	style.fill_color[D2TK_TRIPLE_ACTIVE_HOT] = handle.light_reddest;
	style.fill_color[D2TK_TRIPLE_ACTIVE_FOCUS] = handle.dark_reddest;
	style.fill_color[D2TK_TRIPLE_ACTIVE_HOT_FOCUS] = handle.light_reddest;
	*/
	style.font_face = "FiraCode-Regular.ttf";
	d2tk_base_set_style(base, &style);

	if(d2tk_frontend_step(handle->dpugl))
	{
		handle->done = 1;
	}

	return handle->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle
};

static int
_resize(LV2UI_Handle instance, int width, int height)
{
	plughandle_t *handle = instance;

	return d2tk_frontend_set_size(handle->dpugl, width, height);
}

static const LV2UI_Resize resize_ext = {
	.ui_resize = _resize
};

static const void *
extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__resize))
		return &resize_ext;
		
	return NULL;
}

static const LV2UI_Descriptor shells_bells_ui= {
	.URI            = SHELLS_BELLS__ui,
	.instantiate    = instantiate,
	.cleanup        = cleanup,
	.port_event     = port_event,
	.extension_data = extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &shells_bells_ui;
		default:
			return NULL;
	}
}
