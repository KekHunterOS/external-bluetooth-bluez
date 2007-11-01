/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2007  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "gstsbcenc.h"
#include "gstsbcutil.h"

#define SBC_ENC_DEFAULT_MODE CFG_MODE_AUTO
#define SBC_ENC_DEFAULT_BLOCKS 16
#define SBC_ENC_DEFAULT_SUB_BANDS 8
#define SBC_ENC_DEFAULT_BITPOOL 53
#define SBC_ENC_DEFAULT_ALLOCATION CFG_ALLOCATION_AUTO

GST_DEBUG_CATEGORY_STATIC(sbc_enc_debug);
#define GST_CAT_DEFAULT sbc_enc_debug

#define GST_TYPE_SBC_MODE (gst_sbc_mode_get_type())

static GType gst_sbc_mode_get_type(void)
{
	static GType sbc_mode_type = 0;
	static GEnumValue sbc_modes[] = {
		{  0, "Auto",		"auto"		},
		{  1, "Mono",		"mono"		},
		{  2, "Dual Channel",	"dual"		},
		{  3, "Stereo",		"stereo"	},
		{  4, "Joint Stereo",	"joint"		},
		{ -1, NULL, NULL}
	};

	if (!sbc_mode_type)
		sbc_mode_type = g_enum_register_static("GstSbcMode", sbc_modes);

	return sbc_mode_type;
}

#define GST_TYPE_SBC_ALLOCATION (gst_sbc_allocation_get_type())

static GType gst_sbc_allocation_get_type(void)
{
	static GType sbc_allocation_type = 0;
	static GEnumValue sbc_allocations[] = {
		{ CFG_ALLOCATION_AUTO,		"Auto",		"auto" },
		{ CFG_ALLOCATION_LOUDNESS,	"Loudness",	"loudness" },
		{ CFG_ALLOCATION_SNR,		"SNR",		"snr" },
		{ -1, NULL, NULL}
	};

	if (!sbc_allocation_type)
		sbc_allocation_type = g_enum_register_static(
				"GstSbcAllocation", sbc_allocations);

	return sbc_allocation_type;
}

enum {
	PROP_0,
	PROP_MODE,
	PROP_ALLOCATION,
	PROP_BLOCKS,
	PROP_SUBBANDS
};

GST_BOILERPLATE(GstSbcEnc, gst_sbc_enc, GstElement, GST_TYPE_ELEMENT);

static const GstElementDetails sbc_enc_details =
	GST_ELEMENT_DETAILS("Bluetooth SBC encoder",
				"Codec/Encoder/Audio",
				"Encode a SBC audio stream",
				"Marcel Holtmann <marcel@holtmann.org>");

static GstStaticPadTemplate sbc_enc_sink_factory =
	GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
		GST_STATIC_CAPS("audio/x-raw-int, "
				"rate = (int) { 16000, 32000, 44100, 48000 }, "
				"channels = (int) [ 1, 2 ], "
				"endianness = (int) LITTLE_ENDIAN, "
				"signed = (boolean) true, "
				"width = (int) 16, "
				"depth = (int) 16"));

static GstStaticPadTemplate sbc_enc_src_factory =
	GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
		GST_STATIC_CAPS("audio/x-sbc, "
				"rate = (int) { 16000, 32000, 44100, 48000 }, "
				"channels = (int) [ 1, 2 ], "
				"mode = (string) { mono, dual, stereo, joint }, "
				"blocks = (int) { 4, 8, 12, 16 }, "
				"subbands = (int) { 4, 8 }, "
				"allocation = (string) { snr, loudness },"
				"bitpool = (int) [ 2, 64 ]"));

static GstCaps* sbc_enc_generate_srcpad_caps(GstSbcEnc *enc, GstCaps *caps)
{
	gint rate;
	gint channels;
	GstCaps* src_caps;
	GstStructure *structure;
	const gchar *mode;
	const gchar *allocation;

	structure = gst_caps_get_structure(caps, 0);

	if (!gst_structure_get_int (structure, "rate", &rate))
		return NULL;
	if (!gst_structure_get_int (structure, "channels", &channels))
		return NULL;

	enc->sbc.rate = rate;
	enc->sbc.channels = channels;

	if (enc->mode == 0)
		enc->sbc.joint = CFG_MODE_JOINT_STEREO;
	else
		enc->sbc.joint = enc->mode;

	enc->sbc.blocks = enc->blocks;
	enc->sbc.subbands = enc->subbands;
	if (enc->allocation == 0)
		enc->sbc.allocation = CFG_ALLOCATION_LOUDNESS;
	else
		enc->sbc.allocation = enc->allocation;

	enc->sbc.bitpool = SBC_ENC_DEFAULT_BITPOOL;

	mode = gst_sbc_get_mode_string(enc->sbc.joint);
	allocation = gst_sbc_get_allocation_string(enc->sbc.allocation);

	src_caps = gst_caps_new_simple("audio/x-sbc",
					"rate", G_TYPE_INT, enc->sbc.rate,
					"channels", G_TYPE_INT, enc->sbc.channels,
					"mode", G_TYPE_STRING, mode,
					"subbands", G_TYPE_INT, enc->sbc.subbands,
					"blocks", G_TYPE_INT, enc->sbc.blocks,
					"allocation", G_TYPE_STRING, allocation,
					"bitpool", G_TYPE_INT, enc->sbc.bitpool,
					NULL);

	return src_caps;
}

static gboolean sbc_enc_sink_setcaps (GstPad * pad, GstCaps * caps)
{
	GstSbcEnc *enc;
	GstStructure *structure;
	GstCaps *othercaps;

	enc = GST_SBC_ENC(GST_PAD_PARENT (pad));
	structure = gst_caps_get_structure(caps, 0);

	othercaps = sbc_enc_generate_srcpad_caps(enc, caps);
	if (othercaps == NULL)
		goto error;

	gst_pad_set_caps (enc->srcpad, othercaps);
	gst_caps_unref(othercaps);

	return TRUE;

error:
	GST_ERROR_OBJECT (enc, "invalid input caps");
	return FALSE;
}

gboolean gst_sbc_enc_fill_sbc_params(GstSbcEnc *enc, GstCaps *caps)
{
	GstStructure *structure;
	gint rate, channels, subbands, blocks, bitpool;
	const gchar* mode;
	const gchar* allocation;

	g_assert(gst_caps_is_fixed(caps));

	structure = gst_caps_get_structure(caps, 0);

	if (!gst_structure_get_int(structure, "rate", &rate))
		return FALSE;
	if (!gst_structure_get_int(structure, "channels", &channels))
		return FALSE;
	if (!gst_structure_get_int(structure, "subbands", &subbands))
		return FALSE;
	if (!gst_structure_get_int(structure, "blocks", &blocks))
		return FALSE;
	if (!gst_structure_get_int(structure, "bitpool", &bitpool))
		return FALSE;

	if (!(mode = gst_structure_get_string(structure, "mode")))
		return FALSE;
	if (!(allocation = gst_structure_get_string(structure, "allocation")))
		return FALSE;

	enc->sbc.rate = rate;
	enc->sbc.channels = channels;
	enc->blocks = blocks;
	enc->sbc.subbands = subbands;
	enc->sbc.bitpool = bitpool;
	enc->mode = gst_sbc_get_mode_int(mode);
	enc->allocation = gst_sbc_get_allocation_mode_int(allocation);

	return TRUE;
}

static gboolean gst_sbc_enc_change_caps(GstSbcEnc *enc, GstCaps *caps)
{
	GST_INFO_OBJECT(enc, "Changing srcpad caps (renegotiation)");

	if (!gst_pad_accept_caps(enc->srcpad, caps)) {
		GST_WARNING_OBJECT(enc, "Src pad refused caps");
		return FALSE;
	}

	if (!gst_sbc_enc_fill_sbc_params(enc, caps)) {
		GST_ERROR_OBJECT(enc, "couldn't get sbc parameters from caps");
		return FALSE;
	}

	return TRUE;
}

static GstFlowReturn sbc_enc_chain(GstPad *pad, GstBuffer *buffer)
{
	GstSbcEnc *enc = GST_SBC_ENC(gst_pad_get_parent(pad));
	GstAdapter *adapter = enc->adapter;
	GstFlowReturn res = GST_FLOW_OK;
	gint codesize = enc->sbc.subbands * enc->sbc.blocks * enc->sbc.channels * 2;

	gst_adapter_push(adapter, buffer);

	while (gst_adapter_available(adapter) >= codesize && res == GST_FLOW_OK) {
		GstBuffer *output;
		GstCaps *caps;
		const guint8 *data;
		int consumed;

		data = gst_adapter_peek(adapter, codesize);
		consumed = sbc_encode(&enc->sbc, (gpointer) data, codesize);
		if (consumed <= 0) {
			GST_ERROR ("comsumed < 0, codesize: %d", codesize);
			break;
		}
		gst_adapter_flush(adapter, consumed);

		caps = GST_PAD_CAPS(enc->srcpad);

		res = gst_pad_alloc_buffer_and_set_caps(enc->srcpad,
						GST_BUFFER_OFFSET_NONE,
						enc->sbc.len, caps, &output);

		if (res != GST_FLOW_OK)
			goto done;

		if (!gst_caps_is_equal(caps, GST_BUFFER_CAPS(output)))
			if (!gst_sbc_enc_change_caps(enc,
					GST_BUFFER_CAPS(output))) {
				res = GST_FLOW_ERROR;
				GST_ERROR_OBJECT(enc, "couldn't renegotiate caps");
				goto done;
		}

		memcpy(GST_BUFFER_DATA(output), enc->sbc.data, enc->sbc.len);
		GST_BUFFER_TIMESTAMP(output) = GST_BUFFER_TIMESTAMP(buffer);

		res = gst_pad_push(enc->srcpad, output);
		if (res != GST_FLOW_OK) {
			GST_ERROR_OBJECT(enc, "pad pushing failed");
			goto done;
		}

	}

done:
	gst_object_unref(enc);

	return res;
}

static GstStateChangeReturn sbc_enc_change_state(GstElement *element,
						GstStateChange transition)
{
	GstSbcEnc *enc = GST_SBC_ENC(element);

	switch (transition) {
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		GST_DEBUG("Setup subband codec");
		sbc_init(&enc->sbc, 0);
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		GST_DEBUG("Finish subband codec");
		sbc_finish(&enc->sbc);
		break;

	default:
		break;
	}

	return parent_class->change_state(element, transition);
}

static void gst_sbc_enc_dispose(GObject *object)
{
	GstSbcEnc *enc = GST_SBC_ENC(object);

	if (enc->adapter != NULL)
		g_object_unref (G_OBJECT (enc->adapter));

	enc->adapter = NULL;
}

static void gst_sbc_enc_base_init(gpointer g_class)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);

	gst_element_class_add_pad_template(element_class,
		gst_static_pad_template_get(&sbc_enc_sink_factory));

	gst_element_class_add_pad_template(element_class,
		gst_static_pad_template_get(&sbc_enc_src_factory));

	gst_element_class_set_details(element_class, &sbc_enc_details);
}

static void gst_sbc_enc_set_property(GObject *object, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	GstSbcEnc *enc = GST_SBC_ENC(object);

	/* TODO - CAN ONLY BE CHANGED ON READY AND BELOW */

	switch (prop_id) {
	case PROP_MODE:
		enc->mode = g_value_get_enum(value);
		break;
	case PROP_ALLOCATION:
		enc->allocation = g_value_get_enum(value);
		break;
	case PROP_BLOCKS:
		/* TODO - verify consistency */
		enc->blocks = g_value_get_int(value);
		break;
	case PROP_SUBBANDS:
		/* TODO - verify consistency */
		enc->subbands = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gst_sbc_enc_get_property(GObject *object, guint prop_id,
					GValue *value, GParamSpec *pspec)
{
	GstSbcEnc *enc = GST_SBC_ENC(object);

	switch (prop_id) {
	case PROP_MODE:
		g_value_set_enum(value, enc->mode);
		break;
	case PROP_ALLOCATION:
		g_value_set_enum(value, enc->allocation);
		break;
	case PROP_BLOCKS:
		g_value_set_int(value, enc->blocks);
		break;
	case PROP_SUBBANDS:
		g_value_set_int(value, enc->subbands);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gst_sbc_enc_class_init(GstSbcEncClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->set_property = GST_DEBUG_FUNCPTR(gst_sbc_enc_set_property);
	object_class->get_property = GST_DEBUG_FUNCPTR(gst_sbc_enc_get_property);
	object_class->dispose = GST_DEBUG_FUNCPTR(gst_sbc_enc_dispose);

	element_class->change_state = GST_DEBUG_FUNCPTR(sbc_enc_change_state);

	g_object_class_install_property(object_class, PROP_MODE,
			g_param_spec_enum("mode", "Mode",
				"Encoding mode", GST_TYPE_SBC_MODE,
				SBC_ENC_DEFAULT_MODE, G_PARAM_READWRITE));

	g_object_class_install_property(object_class, PROP_ALLOCATION,
			g_param_spec_enum("allocation", "Allocation",
				"Allocation mode", GST_TYPE_SBC_ALLOCATION,
				SBC_ENC_DEFAULT_ALLOCATION, G_PARAM_READWRITE));

	g_object_class_install_property(object_class, PROP_BLOCKS,
			g_param_spec_int("blocks", "Blocks",
				"Blocks", 0, G_MAXINT,
				SBC_ENC_DEFAULT_BLOCKS,	G_PARAM_READWRITE));

	g_object_class_install_property(object_class, PROP_SUBBANDS,
			g_param_spec_int("subbands", "Sub Bands",
				"Sub Bands", 0, G_MAXINT,
				SBC_ENC_DEFAULT_SUB_BANDS, G_PARAM_READWRITE));

	GST_DEBUG_CATEGORY_INIT(sbc_enc_debug, "sbcenc", 0,
						"SBC encoding element");
}

static void gst_sbc_enc_init(GstSbcEnc *self, GstSbcEncClass *klass)
{
	self->sinkpad = gst_pad_new_from_static_template(&sbc_enc_sink_factory, "sink");
	gst_pad_set_setcaps_function (self->sinkpad,
			GST_DEBUG_FUNCPTR (sbc_enc_sink_setcaps));
	gst_element_add_pad(GST_ELEMENT(self), self->sinkpad);

	self->srcpad = gst_pad_new_from_static_template(&sbc_enc_src_factory, "src");
	gst_pad_set_chain_function(self->sinkpad, GST_DEBUG_FUNCPTR(sbc_enc_chain));
	gst_element_add_pad(GST_ELEMENT(self), self->srcpad);

	self->subbands = SBC_ENC_DEFAULT_SUB_BANDS;
	self->blocks = SBC_ENC_DEFAULT_BLOCKS;
	self->mode = SBC_ENC_DEFAULT_MODE;
	self->allocation = SBC_ENC_DEFAULT_ALLOCATION;

	self->adapter = gst_adapter_new();
}
