/*
 * render-to-sound.cpp
 *
 */

#include <iostream>
#include <iomanip>
#include "argh.h"
#include <fstream>
#include <chrono>

#include "audio.h"
#include "SoundController.h"
#include "SoundRenderer.h"

// suitable audio library (simple interface, apache license for this lib, zlib license for SDL2 (base library)
// https://github.com/jakebesworth/Simple-SDL2-Audio

using namespace std;

void print_audio_spec( SDL_AudioSpec const & a )
{
	cout << "Audio spec (" << ((void *)&a) << ")" << endl;
	const int w = 15;
	const int w2 = 20;
	cout << setw(w) << "format" << " : " << (SDL_AUDIO_ISFLOAT(a.format) ?
			"float" : "int" ) << ", ";
	cout << (SDL_AUDIO_ISSIGNED(a.format) ?
			"signed" : "uinsigned" ) << ", ";
	cout << (SDL_AUDIO_ISBIGENDIAN(a.format) ?
			"big" : "small" ) << " endian" << endl;
	cout << setw(w) << "format" << " : " << "width is " << (SDL_AUDIO_BITSIZE(a.format)) << " bits" << endl;
	cout << setw(w) << "freq" << " : " << setw(w2) << a.freq << endl;
	cout << setw(w) << "channels" << " : " << setw(w2) << ((int)a.channels) << endl;
	cout << setw(w) << "silence" << " : " << setw(w2) << ((int)a.silence) << endl;
	cout << setw(w) << "samples" << " : " << setw(w2) << a.samples << endl;
	cout << setw(w) << "size" << " : " << setw(w2) << a.size << endl;
	cout << setw(w) << "callback" << " : " << setw(w2) << a.callback << endl;
	cout << setw(w) << "userdata" << " : " << setw(w2) << a.userdata << endl;
}

void print_audio( Audio const & a )
{
	cout << "Audio (" << ((void *)&a) << endl;
	const int w = 15;
	const int w2 = 20;
    cout << setw(w) << "length" << " : " << setw(w2) << (a.length) << endl;
    cout << setw(w) << "lengthTrue" << " : " << setw(w2) << (a.lengthTrue) << endl;
    // lengthTrue seems to be length of usable samples (loaded data) in bytes (n_chan * n_samples * bitwidth)
    cout << setw(w) << "* bufferTrue" << " : " << setw(w2) << ((void *)a.bufferTrue) << endl;
    cout << setw(w) << "* buffer" << " : " << setw(w2) << ((void *)a.buffer) << endl;
    cout << setw(w) << "loop" << " : " << setw(w2) << ((int)a.loop) << endl;
    cout << setw(w) << "fade" << " : " << setw(w2) << ((int)a.fade) << endl;
    cout << setw(w) << "free" << " : " << setw(w2) << ((int)a.free) << endl;
    cout << setw(w) << "volume" << " : " << setw(w2) << ((int)a.volume) << endl;
    // SDL_AudioSpec audio;
	cout << setw(w) << "* next" << " : " << setw(w2) << (a.next) << endl;
	print_audio_spec( a.audio );
}

void test_one_rendering()
{
	SoundController sc;

//	playMusic( "./test.wav", SDL_MIX_MAXVOLUME );

		/*
~ $ pactl list short sinks
0	alsa_output.pci-0000_00_03.0.hdmi-stereo	module-alsa-card.c	s16le 2ch 44100Hz	SUSPENDED
1	alsa_output.pci-0000_00_1b.0.analog-stereo	module-alsa-card.c	s16le 2ch 44100Hz	RUNNING

float32 format error was probably due to an older version of libsdl, but I couldn't get 2.0.9 to work in 30sec,
so I am sticking with int16, 48ksps rate

Later migrating to direct SDL2 use anyway (probabyl?)
Example of an beep generator in pure SDL2:
https://stackoverflow.com/questions/10110905/simple-sound-wave-generator-with-sdl-in-c

*/

		/* TODO: make this a full test:
		 * first construct a start sound,
		 * then construct a render sound (that has no starting beep)
		 * and play them both like one would in a real case,
		 * rendered sound has two channels, amplitudes are
		 * (each square 0.25s):
		 */
//		char form_L[] = "______/###|__--_#";
//		char form_R[] = "___#d___----d___-";
		char form_L[] = "_#######____";
		char form_R[] = "_#######____";
//		char form_L[] = "_________________";
//		char form_R[] = "___##############";
		//               01234567890123456
		//               total lenght: 4s
		// d means \, just \ is an escape char so it can't be used
		char * form_arr[] = { form_L, form_R };
		char form_n = (sizeof(form_L)-1)/sizeof(form_L[0]);
		const unsigned int render_n = form_n * SAMPLE_RATE / 4;
		const unsigned int samples_per_square = SAMPLE_RATE/4;
		const float time_per_square = ((float)samples_per_square) / SAMPLE_RATE;

		const float freq = 1000;
		const unsigned int start_sound_samples = SAMPLE_RATE/4;//10;
		audio_t start_sound[2*start_sound_samples];

		for( int i=0; i<start_sound_samples*2; ++i )
			start_sound[i] = 0;

		{
			float amplitude_times[2] = { 0.0, ((float)start_sound_samples)/SAMPLE_RATE };
			float amplitude_values[2] = { 1.0, 1.0 };
			SoundRenderer::RenderAmplitudesToFrequency(
					amplitude_times, amplitude_values, 2,
					start_sound, start_sound_samples, 0,
					2*freq, audio_A
					);
			SoundRenderer::RenderAmplitudesToFrequency(
					amplitude_times, amplitude_values, 2,
					start_sound, start_sound_samples, 1,
					2*freq, audio_A
					);
		}

		auto frame_ts = std::chrono::high_resolution_clock::now();
		sc.PlayStartNow(
			start_sound_samples, start_sound );

		audio_t render_data[2*render_n];
		for( int i=0; i<2*render_n; ++i )
			render_data[i] = 0;

		float amplitude_times[2*form_n];
		float amplitude_values[2*form_n];
		for( int i=0; i<form_n; ++i )
		{
			amplitude_times[2*i] = i*time_per_square;
			amplitude_times[2*i+1] = (i+1)*time_per_square-0.5/SAMPLE_RATE;
		}
		for( int j=0; j<2; ++j )
		{
			for( int i=0; i<form_n; ++i )
			{
				float a0, a1;
				switch(form_arr[j][i])
				{
				case '_': a0=0.; a1=0.; break;
				case '-': a0=0.5; a1=0.5; break;
				case '#': a0=1.0; a1=1.0; break;
				case '/': a0=0.0; a1=1.0; break;
				case 'd': a0=1.0; a1=0.0; break;
				};
				amplitude_values[2*i] = a0;
				amplitude_values[2*i+1] = a1;
			}
			for( int i=0; i<2*form_n; ++i )
			{
				cout << setw(2) << i << " : " << setw(20) << amplitude_values[i];
				cout << " | " << setw(20) << amplitude_times[i];
				cout << endl;
			}
			SoundRenderer::RenderAmplitudesToFrequency(
					amplitude_times, amplitude_values, 2*form_n,
					render_data, render_n, j,
					freq, audio_A
					);
		}

		{
			ofstream f("./test.dat", std::ios_base::binary );
			f.write( (const char *) render_data, 2*render_n*sizeof(audio_t) );
			f.close();
		}

		cout << "Delaying " << endl;
		// Calculation takes 30us it seems
		SDL_Delay(230);
		sc.PlaySound(
			frame_ts,
			render_n, render_data,
			true
				);

	cout << "Waiting now ... " << endl;
	SDL_Delay(4000);
	cout << "Delay : 4000" << endl;
	SDL_Delay(1000);
	cout << "Delay : 5000" << endl;
	cout << "Finishing up." << endl;
	// TODO: if sound is too short, there is some noise when it ends. So make sounds last longer than the play time

}

void test_histogram_based_sound_generation()
{
	SoundController sc;
	const unsigned int sound_n = SAMPLE_RATE * 6;
	audio_t sound_data[sound_n*2];

	{
		// Test if playing music in loop makes the noise go away ?

	}

	const unsigned int amplitude_values[] = { 0, 10, 5, 10, 0, 10, 0, 0, 0, 0, 0, 0 };
	SoundRenderer::RenderAmplitudesToFrequencyWithConstantTimeSteps(
//			amplitude_values, 6, 1, 3.0,
			amplitude_values, 12, 10, 6.0,
			sound_data, sound_n, 0, 1000, audio_A, true );

	auto frame_ts = std::chrono::high_resolution_clock::now();
	sc.PlayStartNow(0,NULL);
	sc.PlaySound(frame_ts, sound_n, sound_data, true );
	SDL_Delay(4000);

	{
		ofstream f("./test.dat", std::ios_base::binary );
		f.write( (const char *) sound_data, 2*sound_n*sizeof(audio_t) );
		f.close();
	}
}

void test_smoothing_kernel()
{
	float kernel[100];
	float sigma;
	int n;

	sigma = 2;
	n = 13;
	cout << "Kernel with sigma = " << sigma << ", n = " << n << endl;
	SoundRenderer::GenerateSmootingKernel(sigma, kernel, n );
	for( int i=0; i<n; ++i )
		printf("% 3d | % 9.4f\n", i, kernel[i]);

//	sigma = 5;
//	n = 31;
//	cout << "Kernel with sigma = " << sigma << ", n = " << n << endl;
//	SoundRenderer::GenerateSmootingKernel(sigma, kernel, n );
//	for( int i=0; i<n; ++i )
//		printf("% 3d | % 9.4f\n", i, kernel[i]);

	const unsigned int amplitude_n = 20;
	unsigned int amplitude_data_in[amplitude_n], amplitude_data_out[amplitude_n];
	for( int i=0; i<amplitude_n; ++i )
	{
		amplitude_data_in[i] = 0;
	}

	amplitude_data_in[8] = 1000;
	amplitude_data_in[12] = 1000;

	SoundRenderer::ApplyKernelSmoothingToAmplitudes(
		amplitude_data_in, amplitude_data_out, amplitude_n,
		kernel, n
		);

	printf("Amplitudes pre and post application of the kernel smoothing:\n");
	long int sum = 0;
	for( int i=0; i<amplitude_n; ++i )
	{
		printf("% 3d | % 6d | % 6d\n", i, amplitude_data_in[i], amplitude_data_out[i] );
		sum += amplitude_data_out[i];
	}
	printf("Sum : %ld\n", sum );
}

int main(int argc, char * argv[])
{
//	test_one_rendering();
//	test_histogram_based_sound_generation();
	test_smoothing_kernel();
    return EXIT_SUCCESS;
}
