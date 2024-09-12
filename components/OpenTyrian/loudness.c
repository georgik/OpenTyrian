/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "file.h"
#include "lds_play.h"
#include "loudness.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"

float music_volume = 0, sample_volume = 0;
float volume = 0;

bool music_stopped = true;
unsigned int song_playing = 0;

bool audio_disabled = false, music_disabled = false, samples_disabled = false;

/* SYN: These shouldn't be used outside this file. Hands off! */
FILE *music_file = NULL;
Uint32 *song_offset;
Uint16 song_count = 0;


SAMPLE_TYPE *channel_buffer[SFX_CHANNELS] = { NULL };
SAMPLE_TYPE *channel_pos[SFX_CHANNELS] = { NULL };
Uint32 channel_len[SFX_CHANNELS] = { 0 };
Uint8 channel_vol[SFX_CHANNELS];


int sound_init_state = false;
int freq = 11025 * OUTPUT_QUALITY;

SDL_AudioStream *audio_stream = NULL;

void audio_cb( void *userdata, unsigned char *feedme, int howmuch );

void load_song( unsigned int song_num );
SDL_AudioDeviceID dev_id;

bool init_audio(void) {
    if (audio_disabled)
        return false;

 /*   SDL_AudioSpec ask, got;

    ask.freq = freq;
    ask.format = (BYTES_PER_SAMPLE == 2) ? SDL_AUDIO_S16 : SDL_AUDIO_S8;
    ask.channels = 1;

    printf("\trequested %d Hz, %d channels\n", ask.freq, ask.channels);

    // Update to SDL3 OpenAudioDevice
    dev_id = SDL_OpenAudioDevice(0, &ask);
    if (dev_id == 0) {
        fprintf(stderr, "error: failed to initialize SDL audio: %s\n", SDL_GetError());
        audio_disabled = true;
        return false;
    }

    printf("\tobtained  %d Hz, %d channels\n", got.freq, got.channels);

    // Create Audio Stream
    audio_stream = SDL_CreateAudioStream(&ask, &got);
    if (!audio_stream) {
        fprintf(stderr, "error: failed to create SDL_AudioStream: %s\n", SDL_GetError());
        audio_disabled = true;
        return false;
    }

    opl_init();

    SDL_PauseAudioDevice(dev_id);  // Start playing audio
*/
    return true;
}

IRAM_ATTR void audio_cb(void *user_data, unsigned char *sdl_buffer, int howmuch)
{
    (void)user_data;

    static long ct = 0;

    SAMPLE_TYPE *feedme = (SAMPLE_TYPE *)sdl_buffer;
    music_disabled = true;
    if (!music_disabled && !music_stopped)
    {
        SAMPLE_TYPE *music_pos = feedme;
        long remaining = howmuch / BYTES_PER_SAMPLE;
        while (remaining > 0)
        {
            while (ct < 0)
            {
                ct += freq;
                lds_update();
            }

            long i = (long)((ct / REFRESH) + 4) & ~3;
            i = (i > remaining) ? remaining : i;
            opl_update((SAMPLE_TYPE *)music_pos, i);
            music_pos += i;
            remaining -= i;
            ct -= (long)(REFRESH * i);
        }

        int qu = howmuch / BYTES_PER_SAMPLE;
        for (int smp = 0; smp < qu; smp++)
        {
            feedme[smp] *= music_volume;
        }
    }
    samples_disabled = false;
    if (!samples_disabled)
    {
        for (int ch = 0; ch < SFX_CHANNELS; ch++)
        {
            volume = sample_volume * (channel_vol[ch] / (float)SFX_CHANNELS);

            unsigned int qu = ((unsigned)howmuch > channel_len[ch] ? channel_len[ch] : (unsigned)howmuch) / BYTES_PER_SAMPLE;
            for (unsigned int smp = 0; smp < qu; smp++)
            {
#if (BYTES_PER_SAMPLE == 2)
                Sint64 clip = (Sint32)feedme[smp] + (Sint32)(channel_pos[ch][smp] * volume);
                feedme[smp] = (clip > 0x7fff) ? 0x7fff : (clip <= -0x8000) ? -0x8000 : (Sint16)clip;
#else
                Sint16 clip = (Sint16)feedme[smp] + (Sint16)(channel_pos[ch][smp] * volume);
                feedme[smp] = (clip > 0x7f) ? 0x7f : (clip <= -0x80) ? -0x80 : (Sint8)clip;
#endif
            }

            channel_pos[ch] += qu;
            channel_len[ch] -= qu * BYTES_PER_SAMPLE;

            if (channel_len[ch] == 0)
            {
                free(channel_buffer[ch]);
                channel_buffer[ch] = channel_pos[ch] = NULL;
            }
        }
    }

    // Use SDL_AudioStream for conversion
    if (audio_stream) {
        SDL_PutAudioStreamData(audio_stream, sdl_buffer, howmuch);
        int converted_size = SDL_GetAudioStreamData(audio_stream, sdl_buffer, howmuch);
        if (converted_size < 0) {
            fprintf(stderr, "error: SDL_GetAudioStreamData failed: %s\n", SDL_GetError());
        }
    }
}

void deinit_audio(void) {
    if (audio_disabled)
        return;
/*
    SDL_PauseAudioDevice(dev_id);  // Pause audio

    SDL_CloseAudioDevice(dev_id);  // Close audio device

    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);  // Destroy the audio stream
        audio_stream = NULL;
    }

    for (unsigned int i = 0; i < SFX_CHANNELS; i++) {
        free(channel_buffer[i]);
        channel_buffer[i] = channel_pos[i] = NULL;
        channel_len[i] = 0;
    }

    lds_free();
	*/
}


void load_music( void )
{
	// if (music_file == NULL)
	// {
	// 	music_file = dir_fopen_die(data_dir(), "music.mus", "rb");
		
	// 	efread(&song_count, sizeof(song_count), 1, music_file);
		
	// 	song_offset = malloc((song_count + 1) * sizeof(*song_offset));
		
	// 	efread(song_offset, 4, song_count, music_file);
	// 	song_offset[song_count] = ftell_eof(music_file);

	// }
}

void load_song( unsigned int song_num )
{
	if (audio_disabled)
		return;
	
	/*SDL_LockAudio();
	
	if (song_num < song_count)
	{
		unsigned int song_size = song_offset[song_num + 1] - song_offset[song_num];
		lds_load(music_file, song_offset[song_num], song_size);
	}
	else
	{
		fprintf(stderr, "warning: failed to load song %d\n", song_num + 1);
	}
	
	SDL_UnlockAudio();
	*/
}

void play_song( unsigned int song_num )
{
	if (song_num != song_playing)
	{
		load_song(song_num);
		song_playing = song_num;
	}
	
	music_stopped = false;
}

void restart_song( void )
{
	unsigned int temp = song_playing;
	song_playing = -1;
	play_song(temp);
}

void stop_song( void )
{
	music_stopped = true;
}

void fade_song( void )
{
	/* STUB: we have no implementation of this to port */
}

void set_volume( unsigned int music, unsigned int sample )
{
	music_volume = music * (1.5f / 255.0f);
	sample_volume = sample * (1.0f / 255.0f);
}

void JE_multiSamplePlay(JE_byte *buffer, JE_word size, JE_byte chan, JE_byte vol)
{
	if (audio_disabled || samples_disabled)
		return;
	
// 	SDL_LockAudio();
	
// 	free(channel_buffer[chan]);
	
// 	channel_len[chan] = size * BYTES_PER_SAMPLE * SAMPLE_SCALING;
// 	channel_buffer[chan] = malloc(channel_len[chan]);
// 	channel_pos[chan] = channel_buffer[chan];
// 	channel_vol[chan] = vol + 1;

// 	for (int i = 0; i < size; i++)
// 	{
// 		for (int ex = 0; ex < SAMPLE_SCALING; ex++)
// 		{
// #if (BYTES_PER_SAMPLE == 2)
// 			channel_buffer[chan][(i * SAMPLE_SCALING) + ex] = (Sint8)buffer[i] << 8;
// #else  /* BYTES_PER_SAMPLE */
// 			channel_buffer[chan][(i * SAMPLE_SCALING) + ex] = (Sint8)buffer[i];
// #endif  /* BYTES_PER_SAMPLE */
// 		}
// 	}

// 	SDL_UnlockAudio();
}

