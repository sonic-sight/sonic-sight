/*
 * SoundRenderer.h
 */

#ifndef SRC_SOUNDRENDERER_H_
#define SRC_SOUNDRENDERER_H_

#include "Defaults.h"
#include <librealsense2/rs.hpp>

namespace SoundRenderer
{
// Adds the signal from amplitude arrays to the sound array,
// using possibly variable sized time steps in amplitudes
// and interpolating between values of amplitudes
void RenderAmplitudesToFrequency(
		const float amplitude_times[],
		const float amplitude_values[], //!< 0 <-> 1 floating values
		const unsigned int amplitude_n_steps, //!< amplitude_...[] is size n_steps
		audio_t sound_values[], //!< 2*sound_n_samples in length
		const unsigned int sound_n_samples,
		const unsigned int sound_channel, //!< channel number 0-1
		const float base_frequency, //!< base frequency for the amplitude envelope
		const audio_t base_amplitude = audio_A, //!< amplitude conversion factor
		bool set_not_add = false //!< if set to true, the amplitudes will be set and not added to existing values
	);

// Adds the signal from the amplitude arrays to the sound array,
// presuming equal time steps and keeping the amplitude in one interval constant
void RenderAmplitudesToFrequencyWithConstantTimeSteps(
		const unsigned int loudness_values[],
		const unsigned int loudness_n_steps, //!< length of amplitude_values
		const unsigned int loudness_max_expected_value, //!< amplitudes are divided by this number in order to normalize the output
		const double loudness_max_time, //!< end time
		audio_t sound_values[], //!< 2*sound_n_samples in length
		const unsigned int sound_n_samples,
		const unsigned int sound_channel, //!< channel number 0-1
		const float base_frequency, //!< base frequency for the amplitude envelope
		const audio_t max_amplitude = audio_A, //!< change amplitude that this renderer never exceeds
		bool set_not_add = false, //!< if set to true, the amplitudes will be set and not added to existing values
		const float frequency_increase_with_time = 0.0, //!< how much frequency increases per unit of time
		const float background_amplitude = 0.0, //!< background signal amplitude (fraction)
		float * amplitude_values = NULL // if not null should be the same length as amplitude_values,
		// stores amplitudes corresponding to loudness values
		);

void GenerateSmootingKernel(
		float sigma_in_steps, //!< sigma in steps, mean is always zero
		float kernel_values[],
		const unsigned int kernel_n //!< must be an odd number
		);

void ApplyKernelSmoothingToAmplitudes(
		const unsigned int amplitude_values_in[],
		unsigned int amplitude_values_out[],
		const unsigned int amplitude_n_steps,
		const float kernel_values[],
		const unsigned int kernel_n //!< must be an odd number
		);
}

/* This class converts depth to a sound sample
 * by taking into account the distances only.
 */
class SimpleDepthRenderer
{
public:
	SimpleDepthRenderer(
		float max_distance,
		float step_distance,
		float speed_of_sound,
		float base_frequency,
		float freq_doubling_length,
		float background_amplitude,
		float stereo_distance,
		float lower_distance,
		float lower_frequency,
		float lower_frequency_increase,
		float lower_amplitude,
		bool save_loudness
			);
	~SimpleDepthRenderer();
	void RenderPointcloudToSound(
			const rs2::vertex * vertices, const unsigned int n_vertices,
			audio_t sound_out[],
			unsigned int sound_n );
private:
	void RenderDistanceToSound(
			float x, float y, float z,
			const rs2::vertex * vertices, const unsigned int n_vertices,
			int channel,
			audio_t sound_out[],
			unsigned int sound_n,
			bool set_not_add,
			float frequency,
			float frequency_increase,
			float background_amplitude
			);
public:
	const float max_distance;
	const float step_distance;
	const float speed_of_sound;
	const float base_frequency;
	const float freq_doubling_length;
	const float background_amplitude;
	const float stereo_distance;
	const float lower_distance;
	const float lower_frequency;
	const float lower_freq_doubling_length;
	const float lower_amplitude;
	const bool save_loudness;
	const unsigned int max_counter; //!< counters go [0] to [max_counter-1]
	const int num_counters;
private:
	unsigned int **counters; //!< an array of counts for each omp thread
public:
	const unsigned int loudness_n_per_channel;
	const float * get_loudness_data()const
	{ return loudness_data; }
	const float * get_amplitudes_data()const
	{ return amplitudes_data; }
private:
	float * loudness_data; //! size 2*loudness_n_per_channel, interleaved data
	float * amplitudes_data; //! size 2*loudness_n_per_channel, interleaved data
};




#endif /* SRC_SOUNDRENDERER_H_ */
