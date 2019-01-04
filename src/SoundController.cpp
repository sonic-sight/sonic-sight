#include "SoundController.h"
#include "Defaults.h"

#include "audio.h"

#include <cstdlib>
#include <iostream>
#include <assert.h>

using namespace std;


// TODO: make sound controller behave as a singleton

SoundController::SoundController(
		unsigned int max_start_sound_samples,
		unsigned int max_render_sound_samples
	)
	:max_start_sound_samples(max_start_sound_samples),
	 max_render_sound_samples(max_render_sound_samples)
{
	SDL_Init(SDL_INIT_AUDIO);
	initAudio();
	start_audio = AllocateMemoryForAudio(max_start_sound_samples);
	render_audio = AllocateMemoryForAudio(max_render_sound_samples);
}

SoundController::~SoundController() {
	delete [] start_audio->bufferTrue;
	start_audio->bufferTrue = NULL;
	delete [] render_audio->bufferTrue;
	render_audio->bufferTrue = NULL;
	endAudio();
	SDL_Quit();
}

void SoundController::PlaySound(
		std::chrono::time_point<std::chrono::high_resolution_clock> frame_ts,
		unsigned int n_samples,
		audio_t signal[],
		bool compensate_delay_from_start
		) {
	cout << "Playing base sound" << endl;
	assert( n_samples < max_render_sound_samples );
	// Determine how many samples to skip, and copy only the remainder
	auto render_start = std::chrono::high_resolution_clock::now();
	// better micro seconds
	const std::chrono::microseconds skip_microseconds = \
		std::chrono::duration_cast<std::chrono::microseconds>(
			render_start - sound_start );
	const unsigned int skip_samples = compensate_delay_from_start \
		? SAMPLE_RATE * skip_microseconds.count() / 1000000ul \
		: 0 ;

	if( skip_samples < n_samples )
	{
		memmove( (void*) render_audio->bufferTrue,
				(void*) &signal[2*skip_samples],
				(n_samples-skip_samples) * sizeof(audio_t) * 2);
		playSoundFromMemory( render_audio, SDL_MIX_MAXVOLUME );
	}
	cout << "Sound started playing " << \
		std::chrono::duration_cast<std::chrono::milliseconds>( \
		render_start - frame_ts ).count() << " milisec late" << endl;
	cout << "Skipped " << skip_samples << " samples." << endl;
}

void SoundController::PlayStartNow(
		unsigned int n_samples,
		audio_t signal[]) {
	cout << "Playing start sound" << endl;
	assert( n_samples < max_start_sound_samples );
	memmove( (void*) start_audio->bufferTrue, (void*) signal, n_samples * sizeof(audio_t) * 2 );
	sound_start = std::chrono::high_resolution_clock::now();
	playSoundFromMemory( start_audio, SDL_MIX_MAXVOLUME );
}

Audio * SoundController::AllocateMemoryForAudio( unsigned int max_samples ){
	/*
Audio (0x21c9ba0
         length :               576000
     lengthTrue :               576000
   * bufferTrue :       0x7f414fa64010
       * buffer :       0x7f414fa64010
           loop :                    0
           fade :                    0
           free :                    1
         volume :                   64 // this is SDL_MIX_MAXVOLUME/2
         * next :                    0
Audio spec (0x21c9bc0)
         format : int, signed, small endian
         format : width is 16 bits
           freq :                48000
       channels :                    2
        silence :                    0
        samples :                 4096
           size :                    0
       callback :                    0
       userdata :                    0
	   */
	Audio * a = new Audio;
	SDL_AudioSpec & as = a->audio;
	as.format = AUDIO_S16SYS;
	as.freq = SAMPLE_RATE;
	as.channels = 2;
	as.silence = 0;
	as.samples = 4096;
	as.size = 0;
	as.callback = NULL;
	as.userdata = NULL;

    a->next = NULL;
    a->loop = 0;
    a->fade = 0;
    a->free = 0;// Set to 0, we manage the buffers in SoundController;
    a->volume = SDL_MIX_MAXVOLUME;

    // lengthTrue seems to be length of usable samples (loaded data) in bytes (n_chan * n_samples * bitwidth)
    // Allocate buffer and set length
    a->lengthTrue = as.channels * max_samples;
    a->bufferTrue = new uint8_t[a->lengthTrue];
    if( a->bufferTrue == NULL )
    {
    	cerr << "ERROR: we couldn't allocate buffer, segfaulting for now" << endl;
    	char * PPP = NULL; *PPP = 1; // TODO: implement some exception handling ?
    }

    a->buffer = a->bufferTrue;
    a->length = a->lengthTrue;
    (a->audio).callback = NULL;
    (a->audio).userdata = NULL;

	return a;
}
