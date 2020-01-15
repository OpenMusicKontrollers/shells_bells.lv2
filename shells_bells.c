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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <inttypes.h>

#include <shells_bells.h>

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Ref ref;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	plugstate_t state;
	plugstate_t stash;

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	PROPS_T(props, MAX_NPROPS);

	LV2_URID midi_MidiEvent;

	int64_t rate;
	int64_t last_remaining;
	uint8_t last_channel;
	uint8_t last_note;
};

static void
_send_midi(plughandle_t *handle, int64_t frames, uint32_t len, const uint8_t *msg)
{
	if(handle->ref)
	{
		handle->ref = lv2_atom_forge_frame_time(&handle->forge, frames);
	}

	if(handle->ref)
	{
		handle->ref = lv2_atom_forge_atom(&handle->forge, len, handle->midi_MidiEvent);
	}

	if(handle->ref)
	{
		handle->ref = lv2_atom_forge_write(&handle->forge, msg, len);
	}
}

static void
_disable_bell(plughandle_t *handle, int64_t frames)
{
	if(handle->last_remaining <= 0)
	{
		return;
	}

	const uint8_t msg [3] = {
		LV2_MIDI_MSG_NOTE_OFF | handle->last_channel,
		handle->last_note,
		0x0
	};

	_send_midi(handle, frames, sizeof(msg), msg);

	handle->last_channel = 0;
	handle->last_note = 0;
	handle->last_remaining = 0;
}


static void
_enable_bell(plughandle_t *handle, int64_t frames)
{
	_disable_bell(handle, frames);

	const uint8_t msg [3] = {
		LV2_MIDI_MSG_NOTE_ON | handle->state.channel,
		handle->state.note,
		handle->state.velocity
	};

	_send_midi(handle, frames, sizeof(msg), msg);

	handle->last_channel = handle->state.channel;
	handle->last_note = handle->state.note;
	handle->last_remaining = handle->rate / 1000 * handle->state.duration;
}

static void
_intercept_trigger(void *data, int64_t frames,
	props_impl_t *impl __attribute__((unused)))
{
	plughandle_t *handle = data;

	if(handle->state.trigger)
	{
		_enable_bell(handle, frames);
	}
	else
	{
		_disable_bell(handle, frames);
	}

	handle->state.trigger = false;
}

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	const char *bundle_path __attribute__((unused)),
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
	{
		return NULL;
	}
	mlock(handle, sizeof(plughandle_t));

	handle->rate = rate;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
		{
			handle->map = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
		{
			handle->log = features[i]->data;
		}
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

	handle->midi_MidiEvent = handle->map->map(handle->map->handle,
		LV2_MIDI__MidiEvent);

	if(!props_init(&handle->props, descriptor->URI,
		defs, MAX_NPROPS, &handle->state, &handle->stash,
		handle->map, handle))
	{
		fprintf(stderr, "failed to initialize property structure\n");
		free(handle);
		return NULL;
	}

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	plughandle_t *handle = (plughandle_t *)instance;

	switch(port)
	{
		case 0:
			handle->control = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->notify = (LV2_Atom_Sequence *)data;
			break;

		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	plughandle_t *handle = instance;

	const uint32_t capacity = handle->notify->atom.size;
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->notify, capacity);
	handle->ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	props_idle(&handle->props, &handle->forge, 0, &handle->ref);

	LV2_ATOM_SEQUENCE_FOREACH(handle->control, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		props_advance(&handle->props, &handle->forge, 0, obj, &handle->ref);
	}

	if(handle->last_remaining > 0)
	{
		if(handle->last_remaining <= nsamples)
		{
			_disable_bell(handle, nsamples - 1);
		}
		else
		{
			handle->last_remaining -= nsamples;
		}
	}

	if(handle->ref)
	{
		lv2_atom_forge_pop(&handle->forge, &frame);
	}
	else
	{
		lv2_atom_sequence_clear(handle->notify);

		if(handle->log)
		{
			lv2_log_trace(&handle->logger, "forge buffer overflow\n");
		}
	}
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	free(handle);
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = instance;

	return props_save(&handle->props, store, state, flags, features);
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve,
	LV2_State_Handle state, uint32_t flags,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = instance;

	return props_restore(&handle->props, retrieve, state, flags, features);
}

static const LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};


static const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
	{
		return &state_iface;
	}

	return NULL;
}

static const LV2_Descriptor shells_bells_bells= {
	.URI						= SHELLS_BELLS__bells,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &shells_bells_bells;

		default:
			return NULL;
	}
}
