/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _MSM_8X60_CELOX_H
#define _MSM_8X60_CELOX_H

#include <sound/apr_audio.h>
#include <sound/q6asm.h>

enum {
    SESSION_DSP_AUDIO_0 = 0,
    SESSION_DSP_AUDIO_1,
    SESSION_DSP_AUDIO_2,
    SESSION_DSP_COUNT,
    SESSION_VOICE = SESSION_DSP_COUNT,
    SESSION_COUNT
};

extern int msm_session_open(int session_id, int stream,
        struct audio_client *audio_client);
extern int msm_session_close(int session_id, int stream);


#endif /*_MSM_8X60_CELOX_H*/
