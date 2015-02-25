/*
 * Copyright (C) 2012 Red Hat
 * based in parts on udlfb.c:
 * Copyright (C) 2009 Roberto De Ioris <roberto@unbit.it>
 * Copyright (C) 2009 Jaya Kumar <jayakumar.lkml@gmail.com>
 * Copyright (C) 2009 Bernie Thompson <bernie@plugable.com>
 *
 * Copyright (C) 2014 Tactical Research, Leigh Cameron.  Created based on udl_encoder.c. 
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include "dummy_encoder.h"

/* dummy encoder */
void dummy_enc_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
	kfree(encoder);
}

static void dummy_encoder_disable(struct drm_encoder *encoder)
{
	/*TODO: we could actually tri-state the outputs here */
}

static bool dummy_mode_fixup(struct drm_encoder *encoder,
			   const struct drm_display_mode *mode,
			   struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void dummy_encoder_prepare(struct drm_encoder *encoder)
{
}

static void dummy_encoder_commit(struct drm_encoder *encoder)
{
}

static void dummy_encoder_mode_set(struct drm_encoder *encoder,
				 struct drm_display_mode *mode,
				 struct drm_display_mode *adjusted_mode)
{
}

static void
dummy_encoder_dpms(struct drm_encoder *encoder, int mode)
{
}

static enum drm_connector_status * dummy_encoder_detect(struct drm_encoder *encoder,
					    struct drm_connector *connector)
{
	return 	connector_status_connected;

}

static const struct drm_encoder_helper_funcs dummy_helper_funcs = {
	.dpms = dummy_encoder_dpms,
	.mode_fixup = dummy_mode_fixup,
	.prepare = dummy_encoder_prepare,
	.mode_set = dummy_encoder_mode_set,
	.commit = dummy_encoder_commit,
	.disable = dummy_encoder_disable,
	.detect = dummy_encoder_detect,
};

static const struct drm_encoder_funcs dummy_enc_funcs = {
	.destroy = dummy_enc_destroy,
};

struct drm_encoder *dummy_encoder_init(struct drm_device *dev)
{
	struct drm_encoder *encoder;

	encoder = kzalloc(sizeof(struct drm_encoder), GFP_KERNEL);
	if (!encoder)
		return NULL;

	drm_encoder_init(dev, encoder, &dummy_enc_funcs, DRM_MODE_ENCODER_TMDS);
	drm_encoder_helper_add(encoder, &dummy_helper_funcs);
	encoder->possible_crtcs = 1;
	return encoder;
}
