#ifndef SRC_SOUNDCONTROLLER_H_
#define SRC_SOUNDCONTROLLER_H_

#include <string>
#include <chrono>

#include "Defaults.h"
#include "audio.h"

class SoundController
{
public:
	SoundController(
		unsigned int max_start_sound_samples = SAMPLE_RATE * 10,
		unsigned int max_render_sound_samples = SAMPLE_RATE * 20
			);
	~SoundController();
	void PlaySound(
		std::chrono::time_point<std::chrono::high_resolution_clock> frame_ts,
		unsigned int n_samples,
		audio_t signal[],
		bool compensate_delay_from_start
			);
	void PlayStartNow(
		unsigned int n_samples,
		audio_t signal[]
			);
	const unsigned int max_start_sound_samples;
	const unsigned int max_render_sound_samples;
private:
	Audio * AllocateMemoryForAudio( unsigned int max_size );
	std::chrono::time_point<std::chrono::high_resolution_clock> sound_start;
	Audio * start_audio;
	Audio * render_audio;
};





#endif /* SRC_SOUNDCONTROLLER_H_ */
