/*
 * SoundRenderer.cpp
 */

#include "SoundRenderer.h"
#include <cmath>
#if defined _OPENMP
#include <omp.h>
#endif

#include <iostream>
#include <iomanip>
#include <assert.h>
#include <stdio.h>
using namespace std;

// equal loudness data for 60 phons
// source : https://plot.ly/~mrlyule/16/equal-loudness-contours-iso-226-2003/#plot
// frequency [Hz], sound pressure level [db SPL]
const unsigned int equal_loudness_N = 31;
const float equal_loudness_nominal = 60.01; // this value corresponds to nominal amplitude
const float equal_loudness_data[equal_loudness_N][2] = {
	{ 20, 109.51 },
	{ 25, 104.23 },
	{ 31.5, 99.08 },
	{ 40, 94.18 },
	{ 50, 89.96 },
	{ 63, 85.94 },
	{ 80, 82.05 },
	{ 100, 78.65 },
	{ 125, 75.56 },
	{ 160, 72.47 },
	{ 200, 69.86 },
	{ 250, 67.53 },
	{ 315, 65.39 },
	{ 400, 63.45 },
	{ 500, 62.05 },
	{ 630, 60.81 },
	{ 800, 59.89 },
	{ 1000, 60.01 },
	{ 1250, 62.15 },
	{ 1600, 63.19 },
	{ 2000, 59.96 },
	{ 2500, 57.26 },
	{ 3150, 56.42 },
	{ 4000, 57.57 },
	{ 5000, 60.89 },
	{ 6300, 66.36 },
	{ 8000, 71.66 },
	{ 10000, 73.16 },
	{ 12500, 68.63 },
	{ 16000, 68.43 },
	{ 20000, 104.92 }
};


namespace SoundRenderer
{

void RenderAmplitudesToFrequency(
		const float amplitude_times[],
		const float amplitude_values[],
		const unsigned int amplitude_n_steps,
		audio_t sound_values[],
		const unsigned int sound_n_samples,
		const unsigned int sound_channel ,
		const float base_frequency,
		const audio_t base_amplitude,
		bool set_not_add
	)
{
	const float dt = 1.0/SAMPLE_RATE;
#pragma omp parallel for default(shared)
	for( int i=0; i<amplitude_n_steps-1; ++i )
	{
		const float & t0 = amplitude_times[i];
		const float & t1 = amplitude_times[i+1];
		const float diff_t = t1 - t0;

		const float & a0 = amplitude_values[i];
		const float & a1 = amplitude_values[i+1];
		const float diff_a = a1 - a0;

		const float da = diff_a / diff_t * dt;
		float a = a0;

		const int sound_n0 = t0*SAMPLE_RATE;
		const int sound_n1 = t1*SAMPLE_RATE;
		const int sound_i0 = sound_n0 * 2 + sound_channel;
		const int sound_i1 = sound_n1 > sound_n_samples ?
				2*sound_n_samples : sound_n1 * 2 + sound_channel;

		// TODO: cache the sin function values in a lookup table, if this sppeds things up
		// on some devices ?
		// or even better: use a saw wave or some other sine wave approximations that is simple to compute
		if(set_not_add)
		{
			for( int j=sound_i0; j<sound_i1; j+=2, a+=da )
				sound_values[j] = base_amplitude * a * sin( 2*M_PI*base_frequency*(j-sound_channel)/SAMPLE_RATE/2. );
		}
		else
		{
			for( int j=sound_i0; j<sound_i1; j+=2, a+=da )
				sound_values[j] += base_amplitude * a * sin( 2*M_PI*base_frequency*(j-sound_channel)/SAMPLE_RATE/2. );
		}
	}
}

void RenderAmplitudesToFrequencyWithConstantTimeSteps(
		const unsigned int loudness_values[],
		const unsigned int loudness_n_steps, //!< length of amplitude_values
		const unsigned int loudness_max_expected_value, //!< amplitudes are divided by this number in order to normalize the output
		const double loudness_max_time, //!< end time
		audio_t sound_values[], //!< 2*sound_n_samples in length
		const unsigned int sound_n_samples,
		const unsigned int sound_channel, //!< channel number 0-1
		const float base_frequency, //!< base frequency for the amplitude envelope
		const audio_t max_amplitude, //!< amplitude conversion factor
		bool set_not_add, //!< if set to true, the amplitudes will be set and not added to existing values
		const float frequency_doubling_time, //!< how much frequency increases per unit of time
		const float background_amplitude,
		float * amplitude_values
		)
{
	const unsigned int background_amp = audio_A * background_amplitude;
	std::cout << "Freq. doubling time is : " << frequency_doubling_time << std::endl;
	const audio_t k_add = set_not_add ? 0 : 1;

	// TODO: test how much latency the function overhead creates on RPi and act accordingly (remove this TODO if ok, reimplement this function if not OK)
	std::function< float(float) > f_phase;
	if( frequency_doubling_time > 0. )
		f_phase = [&base_frequency,&frequency_doubling_time](float t) -> float {
		return base_frequency * frequency_doubling_time / log(2.) *
			( exp( log(2.) * t / frequency_doubling_time) - 1. ); };
	else
		f_phase = [&base_frequency,&frequency_doubling_time](float t) -> float { return
			base_frequency * t; };

#pragma omp parallel default(shared)
	{
	unsigned int i_freq = 0;
	// This factor corrects amplitudes to equalize loudness for changing frequencies
	float correction_of_amplitudes_for_changing_frequency = 1.0;
	float correction_convert_loudness_to_amplitude = 1.0;
#pragma omp for
	for( unsigned int i=0; i<loudness_n_steps; i++ )
	{
		// find the time interval that we will set in the sample
		const unsigned int sound_j0 = i * SAMPLE_RATE * loudness_max_time / loudness_n_steps;
		const unsigned int sound_j1_from_amplitudes = (i+1) * SAMPLE_RATE * loudness_max_time / loudness_n_steps;
		const unsigned int sound_j1 = sound_j1_from_amplitudes < sound_n_samples ? sound_j1_from_amplitudes : sound_n_samples;

		// correct for changing frequencies
		if( frequency_doubling_time > 0. )
		{
			const float avg_t = (i+0.5) * loudness_max_time / loudness_n_steps;
			const float avg_freq = base_frequency * exp( log(2.) * avg_t / frequency_doubling_time );

			while( (i_freq < equal_loudness_N-1) && (equal_loudness_data[i_freq+1][0] < avg_freq) )
				i_freq++;

			const float d_freq = equal_loudness_data[i_freq+1][0]-equal_loudness_data[i_freq][0];
			const float k_i = (avg_freq - equal_loudness_data[i_freq][0]) / d_freq;
			const float k_ip1 = 1.0 - k_i;
			const float avg_loudness = equal_loudness_data[i_freq][1]*k_i + equal_loudness_data[i_freq+1][1]*k_ip1;

			correction_of_amplitudes_for_changing_frequency = exp(
					(avg_loudness - equal_loudness_nominal) / 20. );
		}

		// calculate this loudness (as a fraction in expected range 0 <---> L0=1.0)
		float this_loudness = ((float)loudness_values[i]) / loudness_max_expected_value + background_amplitude;

		// Correction for exponential perception of loudness
		// "A widely used "rule of thumb" for the loudness of a particular sound is
		// that the sound must be increased in intensity by a factor of ten
		// for the sound to be perceived as twice as loud."
		// (http://hyperphysics.phy-astr.gsu.edu/hbase/Sound/loud.html#c2)

		const float kL = log(10.) / (2.*log(2.));
		const float kA0L0 = max_amplitude / pow(1.0,kL);
		const float this_amplitude_f = kA0L0 * pow(this_loudness,kL);
		const audio_t this_amplitude = this_amplitude_f < max_amplitude ? this_amplitude_f : max_amplitude;

		if( amplitude_values != NULL )
			amplitude_values[2*i+sound_channel] = ((float)this_amplitude) / max_amplitude;

		for( int j=2*sound_j0+sound_channel; j<2*sound_j1+sound_channel; j+=2 )
		{
			const float t = (j-sound_channel)/2/((float)SAMPLE_RATE);
			const float phase = f_phase(t);
			sound_values[j] = this_amplitude * sin( 2*M_PI*phase ) \
				+ k_add * sound_values[j];
		}
	} // end of for loop
	} // end of parallel region
}

void GenerateSmootingKernel(
		float sigma_in_steps, //!< sigma in steps, mean is always zero
		float kernel_values[],
		const unsigned int kernel_n
		)
{
	assert( kernel_n % 2 == 1 );
	const int kernel_n_2 = kernel_n / 2;
	const float k1 = 1.0/sqrt(2*M_PI*sigma_in_steps*sigma_in_steps);
	const float k2 = -1.0 / (2*sigma_in_steps*sigma_in_steps);
	double kernel_sum = 0.;
	for( int j=0; j<kernel_n; ++j )
	{
		const int i=j-kernel_n_2;
		const float dx = i;//-sigma_in_steps;
		kernel_values[j] = k1*exp( k2 * i*i );//dx*dx );
		kernel_sum += kernel_values[j];
	}
	// Now normalize the kernel
	const double k_sum = 1.0/kernel_sum;
	for( int i=0; i<kernel_n; ++i )
		kernel_values[i] *= k_sum;
}


void ApplyKernelSmoothingToAmplitudes(
		const unsigned int amplitude_values_in[],
		unsigned int amplitude_values_out[],
		const unsigned int amplitude_n_steps,
		const float kernel_values[],
		const unsigned int kernel_n
		)
{
	assert( kernel_n % 2 == 1 ); // odd-sized kernels only
	const unsigned int kernel_n_2 = kernel_n / 2;

#pragma omp parallel for
	for( unsigned int i=0; i<amplitude_n_steps; ++i )
	{
		amplitude_values_out[i] = 0;


		const unsigned int j_start = i > kernel_n_2 ? i-kernel_n_2 : 0;
		const unsigned int j_end = amplitude_n_steps-1 > i + kernel_n_2 ? i + kernel_n_2 : amplitude_n_steps-1;
		for(
				int j=j_start;
				j<=j_end;
				++j )
		{
			const unsigned int k = j - (i-kernel_n_2);
			amplitude_values_out[i] += (kernel_values[k] * amplitude_values_in[j]) +0.5;
		}
	}
}

} // end of namespace SoundRenderer

SimpleDepthRenderer::SimpleDepthRenderer(
	float max_distance,
	float step_distance,
	float speed_of_sound,
	float base_frequency,
	float base_frequency_doubling_length,
	float background_amplitude,
	float stereo_distance,
	float lower_distance,
	float lower_frequency,
	float lower_frequency_doubling_length,
	float lower_amplitude,
	bool save_loudness
	)
	:max_distance(max_distance),
	 step_distance(step_distance),
	 speed_of_sound(speed_of_sound),
	 base_frequency(base_frequency),
	 freq_doubling_length(base_frequency_doubling_length),
	 background_amplitude(background_amplitude),
	 stereo_distance(stereo_distance),
	 lower_distance(lower_distance),
	 lower_frequency(lower_frequency),
	 lower_freq_doubling_length(lower_frequency_doubling_length),
	 lower_amplitude(lower_amplitude),
	 save_loudness(save_loudness),
	 max_counter(max_distance/step_distance),
#if defined _OPENMP
	 num_counters(omp_get_max_threads()),
#else
	 num_counters(1),
#endif
	 loudness_n_per_channel(this->max_counter)
{
	counters = new unsigned int * [num_counters];
	counters[0] = new unsigned int [num_counters*max_counter*2];
	for( int i=1; i<num_counters*2; ++i )
		counters[i] = &counters[0][max_counter*i];
	loudness_data = new float [2*loudness_n_per_channel];
	amplitudes_data = new float [2*loudness_n_per_channel];
}

SimpleDepthRenderer::~SimpleDepthRenderer()
{
	delete [] counters[0];
	delete [] counters;
	delete [] loudness_data;
	delete [] amplitudes_data;
}

void SimpleDepthRenderer::RenderPointcloudToSound(
	const rs2::vertex * vertices, const unsigned int n_vertices,
	audio_t sound_out[],
	unsigned int sound_n )
{
	this->RenderDistanceToSound(
		-stereo_distance/2, 0, 0,
		vertices, n_vertices, 0,
		sound_out, sound_n,
		true,
		base_frequency,
		freq_doubling_length,
		background_amplitude
		);
	this->RenderDistanceToSound(
		+stereo_distance/2, 0, 0,
		vertices, n_vertices, 1,
		sound_out, sound_n,
		true,
		base_frequency,
		freq_doubling_length,
		background_amplitude
		);

//	return;
	if( lower_distance > 0. )
	{
		std::cout << "Rendering lower distance" << std::endl;
	this->RenderDistanceToSound(
		-stereo_distance/2, -lower_distance, 0,
		vertices, n_vertices, 0,
		sound_out, sound_n,
		false,
		lower_frequency,
		lower_freq_doubling_length,
		lower_amplitude
		);
	this->RenderDistanceToSound(
		+stereo_distance/2, -lower_distance, 0,
		vertices, n_vertices, 1,
		sound_out, sound_n,
		false,
		lower_frequency,
		lower_freq_doubling_length,
		lower_amplitude
		);
	}
}

void SimpleDepthRenderer::RenderPointcloudToSoundDelayIsAngle(
	const rs2::vertex * vertices, const unsigned int n_vertices,
	audio_t sound_out[],
	unsigned int sound_n,
	const unsigned int camera_w,
	const float delay_distance_at_max_angle
	)
{
	int num_used_counters = 1;
#pragma omp parallel default(shared)
	{

#if defined _OPENMP
		unsigned int * my_counter_left = counters[omp_get_thread_num()];
		unsigned int * my_counter_right = counters[omp_get_thread_num()+num_counters];
#pragma omp single
		{
			num_used_counters = omp_get_num_threads();
		}
#else
		unsigned int * my_counter_left = counters[0];
		unsigned int * my_counter_right = counters[num_counters];
#endif

		// DO NOT OMP PARALLELIZE
		for( int i=0; i<max_counter; ++i )
		{
			my_counter_left[i] = 0;
			my_counter_right[i] = 0;
		}

#pragma omp for
		for( int i=0; i < n_vertices; ++i )
		{
			const int i_width = i % camera_w;
			const float delay_distance_fraction = 2.0*i_width/camera_w - 1.; // -1 <-> +1
			const float this_delay_distance = delay_distance_at_max_angle * \
					delay_distance_fraction;

			rs2::vertex const & v = vertices[i];
			if( v.z < 0.0001 )
				continue;
			const float dd = sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
			const unsigned int i_bin_left = ((unsigned int)((dd+this_delay_distance)/step_distance));
			if(i_bin_left<max_counter)
				my_counter_left[i_bin_left]++;
			const unsigned int i_bin_right = ((unsigned int)((dd-this_delay_distance)/step_distance));
			if(i_bin_right<max_counter)
				my_counter_right[i_bin_right]++;
		}
	} // end openMP parallel region

	// move all counts to counter 0
	for( int i=1; i<num_used_counters; ++i )
	{
#pragma omp parallel for default(shared)
		for( int j=0; j<max_counter; ++j )
		{
			counters[0][j] += counters[i][j];
			counters[i][j] = 0;
			counters[num_counters][j] += counters[num_counters+i][j];
			counters[num_counters+i][j] = 0;
		}
	}

	// Re-normalize signals so that a sample in which 25% of the points are within 10cm distance
	// reaches (max amplitude)/10. amp_div is the avg. num of samples per interval in the described configuration
	const unsigned int amp_div = (n_vertices / 25.0) / (0.1/step_distance) * 10;
	// Effective max counts = audio_A * amplitude_divider / base_amplitude
	// audio_A = amplitude_values[i] / amplitude_divider * base_amplitude + 0.5
	SoundRenderer::RenderAmplitudesToFrequencyWithConstantTimeSteps(
			counters[0], max_counter, amp_div, max_distance / speed_of_sound,
			sound_out, sound_n, 0, base_frequency,
			audio_A, // max amplitude
			true,
			freq_doubling_length / speed_of_sound,
			background_amplitude,
			save_loudness ? amplitudes_data : NULL
			);
	SoundRenderer::RenderAmplitudesToFrequencyWithConstantTimeSteps(
			counters[0+num_counters], max_counter, amp_div, max_distance / speed_of_sound,
			sound_out, sound_n, 1, base_frequency,
			audio_A, // max amplitude
			true,
			freq_doubling_length / speed_of_sound,
			background_amplitude,
			save_loudness ? amplitudes_data : NULL
			);

	if(save_loudness )
	{
#pragma omp parallel for
		for( int i=0; i<loudness_n_per_channel; ++i )
		{
			loudness_data[2*i+0] = ((float)counters[0][i]/amp_div) ;
			loudness_data[2*i+1] = ((float)counters[num_counters][i]/amp_div) ;
		}
	}
}

void SimpleDepthRenderer::RenderDistanceToSound(
	float x, float y, float z,
	const rs2::vertex * vertices, const unsigned int n_vertices,
	int channel,
	audio_t sound_out[],
	unsigned int sound_n,
	bool set_not_add,
	float frequency,
	float frequency_doubling_length,
	float background_amplitude
	)
{
	int num_used_counters = 1;
#pragma omp parallel default(shared)
	{

#if defined _OPENMP
		unsigned int * my_counter = counters[omp_get_thread_num()];
#pragma omp single
		{
			num_used_counters = omp_get_num_threads();
		}
#else
		unsigned int * my_counter = counters[0];
#endif

		// DO NOT OMP PARALLELIZE
		for( int i=0; i<max_counter; ++i )
			my_counter[i] = 0;

		float dx, dy, dz, dd;
#pragma omp for
		for( int i=0; i < n_vertices; ++i )
		{
			rs2::vertex const & v = vertices[i];
			if( v.z < 0.0001 )
				continue;
			dx = v.x - x;
			dy = v.y - y;
			dz = v.z - z;
			dd = sqrt(dx*dx+dy*dy+dz*dz);
			const unsigned int i_bin = ((unsigned int)(dd/step_distance));
			if(i_bin<max_counter)
				my_counter[i_bin]++;
		}
	} // end openMP parallel region

	// move all counts to counter 0
	for( int i=1; i<num_used_counters; ++i )
	{
#pragma omp parallel for default(shared)
		for( int j=0; j<max_counter; ++j )
		{
			counters[0][j] += counters[i][j];
			counters[i][j] = 0;
		}
	}

	// Re-normalize signals so that a sample in which 25% of the points are within 10cm distance
	// reaches (max amplitude)/10. amp_div is the avg. num of samples per interval in the described configuration
	const unsigned int amp_div = (n_vertices / 25.0) / (0.1/step_distance) * 10;
	// Effective max counts = audio_A * amplitude_divider / base_amplitude
	// audio_A = amplitude_values[i] / amplitude_divider * base_amplitude + 0.5
	SoundRenderer::RenderAmplitudesToFrequencyWithConstantTimeSteps(
			counters[0], max_counter, amp_div, max_distance / speed_of_sound,
			sound_out, sound_n, channel, frequency,
			lower_distance > 0. ? audio_A/2 : audio_A, // max amplitude
			set_not_add,
			frequency_doubling_length / speed_of_sound,
			background_amplitude,
			(set_not_add && save_loudness) ? amplitudes_data : NULL
			);

	if(set_not_add && save_loudness )
	{
#pragma omp parallel for
		for( int i=0; i<loudness_n_per_channel; ++i )
			loudness_data[2*i+channel] = ((float)counters[0][i]/amp_div) ;
	}
}

