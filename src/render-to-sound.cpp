/*
 * render-to-sound.cpp
 *
 */

#include <iostream>
#include "argh.h"
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>

#include <librealsense2/rs.hpp>

// For sending signals to other processes:
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#include "SoundController.h"
#include "SoundRenderer.h"

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

template<typename T>
T get_value( argh::parser & cmdl, std::string parameter, T default_value )
{
	T t_tmp;
	cmdl( parameter, default_value ) >> t_tmp;
	return t_tmp;
}


int main(int argc, char * argv[]) try
{
	int i_tmp;
	float f_tmp;
	std::stringstream ss_tmp;
	argh::parser cmdl;
	cmdl.add_params(
			{
				"--signal-pid",
				"--signal-depth-filename",
				"--camera-width",
				"--camera-height",
				"--camera-fps",
				"--save-depth-to",
				"--renderer-max-distance",
				"--renderer-step-distance",
				"--renderer-speed-of-sound",
				"--renderer-stereo-distance",
				"--renderer-base-frequency",
				"--renderer-freq-doubling-length",
				"--renderer-base-amplitude",
				"--renderer-start-frequency",
				"--renderer-start-duration",
				"--renderer-start-amplitude",
				"--renderer-interval-extra-time",
				"--renderer-lower-distance",
				"--renderer-lower-frequency",
				"--renderer-lower-frequency-doubling-length",
				"--renderer-lower-amplitude"
			});
	cmdl.parse(argc,argv);

	if( cmdl[{"-h","--help"}] )
	{
		using namespace std;
		cout << "Possible parameters:" << endl;

		cout << "[external communication]" << endl;
		cout << "--signal-pid=<pid> : " << endl;
		cout << "\t pid of the process that is sent SIGUSR1," << endl;
		cout << "\t after pointcloud is sved in the specified file" << endl;
		cout << "--signal-depth-filename=<filename> : " << endl;
		cout << "\t filename to save depth to before signaling to an external process" << endl;

		cout << "[camera]" << endl;
		cout << "Caution: some parameter combinations are not supported by the camera," << endl;
		cout << "and over an USB2 connection the selection of available parameters is even smaller" << endl;
		cout << "--camera-width=<width=1280> : " << endl;
		cout << "\t depth camera width in pixels" << endl;
		cout << "--camera-height=<height=720> : " << endl;
		cout << "\t depth camera height in pixels" << endl;
		cout << "--camera-fps=<fps=15> : " << endl;
		cout << "\t depth camera frame rate" << endl;

		cout << "[data archiving]" << endl;
		cout << "--save-depth-to=<filename-template> : " << endl;
		cout << "\t pointclouds and generated waveforms are saved to " << endl;
		cout << "\t <filename-template>__<timestamp>.{dat|waveform}" << endl;

		cout << "[depth rendering]" << endl;
		cout << "--renderer-max-distance=<max distance=4.0> : " << endl;
		cout << "\t max distance for depth renderer" << endl;
		cout << "--renderer-step-distance=<step distance=0.005> : " << endl;
		cout << "\t step in distance for depth renderer (depth resolution)" << endl;
		cout << "--renderer-speed-of-sound=<speed of sound=1.0> : " << endl;
		cout << "\t speed of sound for depth renderer [m/s]" << endl;
		cout << "--renderer-stereo-distance=<stereo distance=0.2> : " << endl;
		cout << "\t distance between the depth renderer's \"ears\" [m]" << endl;
		cout << "--renderer-base-frequency=<frequency=1000.0> : " << endl;
		cout << "\t base frequency for depth renderer [Hz=1/s]" << endl;
		cout << "--renderer-freq-doubling-length=<frequency doubling=-1.> : " << endl;
		cout << "\t length at which the frequency doubles [m]" << endl;
		cout << "\t (if left unset, the frequency remains constant)" << endl;
		cout << "--renderer-base-amplitude=<amplitude=0.0> : " << endl;
		cout << "\t base amplitude (added to signal) [%]" << endl;
		cout << "--renderer-start-frequency=<frequency=1000.0> : " << endl;
		cout << "\t start signal frequency for depth renderer [Hz=1/s]" << endl;
		cout << "--renderer-start-duration=<duration=-1.> : " << endl;
		cout << "\t start signal duration [s]" << endl;
		cout << "\t (if left unset, the start signal is not played)" << endl;
		cout << "--renderer-start-amplitude=<amplitude=50.0> : " << endl;
		cout << "\t start signal amplitude [% of max]" << endl;
		cout << "--renderer-interval-extra-time=<extre time=0.1> : " << endl;
		cout << "\t extra time to wait between sound renders [s]" << endl;
		cout << "\t (time between renders is this time + <max distance> / <speed of sound>" << endl;

		cout << "--renderer-lower-distance=<lower distance=-1.> : " << endl;
		cout << "\t the lower renderer is this far below the main renderer [m]" << endl;
		cout << "\t if left unset, a negative value is set to signal that the lower channel should not be used" << endl;
		cout << "--renderer-lower-frequency=<lower frequency=500> : " << endl;
		cout << "\t base frequency of the lower renderer [Hz]" << endl;
		cout << "--renderer-lower-frequency-doubling-length=<lower freq. doubling=-1.> : " << endl;
		cout << "\t length at which the lower signal's frequency doubles [m]" << endl;
		cout << "\t (if left unset, the frequency remains constant)" << endl;
		cout << "--renderer-lower-amplitude=<amplitude=0.0> : " << endl;
		cout << "\t lower signal base amplitude (added to signal) [%]" << endl;

		return 0;
	}

	// in etc/fstab: "tmpfs       /mnt/ramdisk tmpfs   nodev,nosuid,noexec,nodiratime,size=1024M   0 0"
	// ram disk filename : "/mnt/ramdisk/pointcloud.dat"
	const std::string signal_depth_filename = cmdl("--signal-depth-filename","").str();
	cmdl("--signal-pid", -1) >> i_tmp;
	const int process_to_signal = i_tmp;

	cmdl("--camera-width", 1280 ) >> i_tmp;
	const int camera_width = i_tmp;
	cmdl("--camera-height", 720 ) >> i_tmp;
	const int camera_height = i_tmp;
	const int camera_fps = get_value(cmdl, "--camera-fps", 15 );

	const std::string depth_path = cmdl("--save-depth-to","").str();
	const bool save_depth = depth_path.length() > 0;

    cmdl("--renderer-max-distance", 4.0) >> f_tmp;
    const float param_max_distance = f_tmp;
    const float renderer_step_distance = get_value(cmdl, "--renderer-step-distance", 0.005);
    cmdl("--renderer-speed-of-sound", 1.0) >> f_tmp;
    const float param_speed_of_sound = f_tmp;
    cmdl("--renderer-stereo-distance", 0.2) >> f_tmp;
    const float param_stereo_distance = f_tmp;
	const float param_base_frequency = get_value(cmdl,"--renderer-base-frequency",1000.0);
	const float renderer_freq_doubling_length = get_value(cmdl,"--renderer-freq-doubling-length",
			-1.0 );
	const float renderer_base_amplitude = get_value(cmdl,"--renderer-base-amplitude",0.0) / 100.0;

	const float renderer_start_frequency = get_value(cmdl,"--renderer-start-frequency",1000.0);
	const float renderer_start_duration = get_value(cmdl,"--renderer-start-duration",
			-1.0 );
	const float renderer_start_amplitude = get_value(cmdl,"--renderer-start-amplitude", 50.0 ) / 100.0;
	const float renderer_interval_extra_time = get_value(cmdl,"--renderer-interval-extra-time",0.1);

	const float renderer_lower_distance = get_value(cmdl,"--renderer-lower-distance",
			-1.0 );
	const float renderer_lower_frequency = get_value(cmdl,"--renderer-lower-frequency",500.);
	const float renderer_lower_frequency_doubling_length = get_value(cmdl,
		"--renderer-lower-frequency-doubling-length",
		-1.0 );
	const float renderer_lower_background_amplitude = get_value(cmdl,
		"--renderer-lower-amplitude",0.0)/100.;

	const float renderer_interval_max_render_time = param_max_distance / param_speed_of_sound;
	const float renderer_interval_total_time = renderer_interval_extra_time +
			renderer_interval_max_render_time;

	std::cout << "Freq. doubling length = " << renderer_freq_doubling_length << std::endl;
	std::cout << "Rendering max time = " << renderer_interval_max_render_time << std::endl;
	std::cout << "Lower distance is = " << renderer_lower_distance << " | " << std::isnan(renderer_lower_distance) << std::endl;

	if(save_depth)
		std::cout << "Saving depth to : " << depth_path << std::endl;

#if DEBUGOMP == 1
	const unsigned int debug_vertices_n = camera_width*camera_height;
	rs2::vertex * debug_vertices_data = new rs2::vertex[debug_vertices_n];
	if( debug_vertices_data == NULL )
	{
		std::cerr << "Error allocating data for debug vertices" << std::endl;
		return 0;
	}
	{
		std::fstream f;
		f.open("./debug_vertices.dat", std::ios_base::in | std::ios_base::binary);
		if( f.is_open() == false )
		{
			std::cerr << "Error opening debug vertices data file" << std::endl;
			return 1;
		}
		std::cout << "Reading data" << std::endl;
		f.read( (char *)debug_vertices_data, debug_vertices_n * sizeof(rs2::vertex) );
	}
#endif


	rs2::pointcloud pc;
	rs2::points points;

	// Create a pipeline and start it
	rs2::pipeline pipe;
	rs2::config cfg;
	std::cout << "Configuring camera stream with parameters w,h = " << camera_width << "," << camera_height;
	std::cout << "     streaming at " << camera_fps << " frames per second" << std::endl;
    cfg.enable_stream(RS2_STREAM_DEPTH, camera_width, camera_height, RS2_FORMAT_Z16, camera_fps);
    if( save_depth )
		cfg.enable_stream( RS2_STREAM_COLOR, camera_width, camera_height, RS2_FORMAT_RGB8, camera_fps );
    rs2::pipeline_profile profile = pipe.start(cfg);

    rs2::align align( RS2_STREAM_DEPTH );

    // TODO: possibly migrate realsense code to a "Asynchronous method"

    /*
     * Main application loop:
     * wait for frames, if you get one and the time since the last frame is long enough,
     * render the points to sound and play the sound
     * (for now: export the points data and signal to python app)
     */

    SoundController sc;
    SimpleDepthRenderer sdr(
    	param_max_distance, renderer_step_distance,
		param_speed_of_sound, param_base_frequency, renderer_freq_doubling_length,
		renderer_base_amplitude,
		param_stereo_distance,
		renderer_lower_distance,
		renderer_lower_frequency,
		renderer_lower_frequency_doubling_length,
		renderer_lower_background_amplitude,
		save_depth
    		);
    const unsigned int sound_start_n =
    		renderer_start_duration > 0 ? renderer_start_duration * SAMPLE_RATE : 0;
    audio_t sound_start_data[sound_start_n*2];
    const unsigned int sound_render_n = SAMPLE_RATE *
    		renderer_interval_total_time * 2.; // 2. to remove beeps when we are late
    audio_t sound_render_data[sound_render_n*2];
    for(int i=0; i<sound_render_n*2; ++i )
    	sound_render_data[i] = 0;
    for( int i=0; (i<2) && (sound_start_n > 0); ++i )
    {
    	std::cout << "Preparing start sound" << std::endl;
    	const float amplitude_times[2] = { 0.0, 2.0*renderer_start_duration };
    	const float amplitude_values[2] = { renderer_start_amplitude, renderer_start_amplitude };
		SoundRenderer::RenderAmplitudesToFrequency(
			amplitude_times, amplitude_values, 2,
			sound_start_data, sound_start_n, i,
			renderer_start_frequency, audio_A, true
    		);
    }
    std::cout << "Start duration = " << renderer_start_duration << "; n = " << sound_start_n << std::endl;
    std::cout << "Start amplitude = " << renderer_start_amplitude << std::endl;

    auto time_last_sound = std::chrono::high_resolution_clock::now();
    const std::chrono::milliseconds scan_interval(10);
    const std::chrono::milliseconds sound_interval((long int)(renderer_interval_total_time*1000));
    rs2_error *e = NULL;
    while(true)
    {
		rs2::frameset data = pipe.wait_for_frames();
        auto time_now = std::chrono::high_resolution_clock::now();
        if (time_now - time_last_sound < sound_interval )
        {
    		std::this_thread::sleep_for(scan_interval);
    		continue;
        }

		rs2::frame depth_frame = data.get_depth_frame();
		if( !depth_frame )
			return EXIT_FAILURE; // This should not happen, as we have to get depth frames in all cases
		// Apply any spatial filters here and swap the depth_frame for the processed depth frame

        {
        	// TODO: make openMP parallelization of
        	// librealsense/src/proc/pointcloud.cpp deproject_depth
        	// Even better: parse the depth frame directly, as the deprojection seems to be nothing more than
        	// some linear multiplication (TODO as part of optimizations and speedups, if necessary)
        	// See librealsense/include/librealsense2/rsutil.h deproject_pixel_to_point
        	// OR MAKE AN OPENCL implementation of pointcloud calculation based on depth scale (see rs-align example)
        	// (RPi's GPU is much faster than its CPU so GPU calculations make a lot of sense)

//        	sc.PlayStartNow( sound_start_n, sound_start_data ); / THIS is the desired location of this call, provided that the rendering can be done fast enough

			points = pc.calculate(depth_frame);
        	std::cout << "Ping !" << std::endl;
        	std::cout << "max distance = " << param_max_distance << std::endl;
        	std::cout << "interval = " << renderer_interval_total_time << std::endl;
        	std::cout << "speed of sound = " << param_speed_of_sound << std::endl;
        	std::cout << "stereo distance = " << param_stereo_distance << std::endl;
        	time_last_sound = time_now;
        	std::cout << "Point size: " << points.size() << std::endl;
        	{ // Render the sound and play it
        		sdr.RenderPointcloudToSound(
#if DEBUGOMP==1
        				debug_vertices_data, debug_vertices_n,
#else
        				points.get_vertices(), points.size(),
#endif
						sound_render_data, sound_render_n );

        		// Time examples : line 195 of librealsense/wrappers/opencv/latency-tool/latency-detector.h
        		// rs2_time_t is miliseconds in double
        		// This should be correct, but it doesn't seem to be (that is device clock that can not be related to CPU clock)
//				rs2_time_t frame_timestamp = rs2_get_frame_timestamp(f.get(),&e);
        		// so instead we measure latency from the time the frame was released from the driver
				rs2_time_t frame_timestamp = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);
        		rs2_time_t time_rs2_at_rendering = rs2_get_time(NULL); // Gets current time
        		std::cout << "Time (frame generation) = " << frame_timestamp << std::endl;
        		std::cout << "Time (now)              = " << time_rs2_at_rendering << std::endl;
				sc.PlayStartNow( sound_start_n, sound_start_data ); // MOVED from previous location due to too long computation times
				auto time_chrono_at_rendering = std::chrono::high_resolution_clock::now();
        		sc.PlaySound(
					time_chrono_at_rendering + std::chrono::milliseconds(
							(long int)(frame_timestamp - time_rs2_at_rendering)),
					sound_render_n, sound_render_data,
					(renderer_start_duration > 0)
					);
        	}
        	if( (process_to_signal > 0) && (signal_depth_filename.length() > 0)  )
        	{
				std::ofstream of(signal_depth_filename.c_str(),std::ios_base::binary);
				of.write( (const char *)points.get_data(), points.size() * sizeof(rs2::vertex) );
				of.close();
        		std::cout << "Signaling process " << process_to_signal << std::endl;
        		const int rk = kill(process_to_signal,SIGUSR1);
        		if(rk != 0)
        		{
					std::cout << "Kill returned " << rk << std::endl;
					std::cerr << "Error is : ";
					perror(NULL);
					std::cerr << std::endl;
        		}
        	}
			if(save_depth)
			{
				auto now = std::chrono::system_clock::now();
				auto now_c = std::chrono::system_clock::to_time_t(now);
				// save pointcloud
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							".dat";
					std::cout << "Depth filename is : " << ss_tmp.str() << std::endl;
					std::ofstream of( ss_tmp.str(),std::ios_base::binary);
					of.write( (const char *)points.get_data(), points.size() * sizeof(rs2::vertex) );
					of.close();
				}
				// Save sound
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							".waveform";
					std::cout << "Waveform filename is : " << ss_tmp.str() << std::endl;
					std::ofstream of( ss_tmp.str(), std::ios_base::binary );
					of.write( (const char *) sound_render_data, 2*sound_render_n*sizeof(audio_t) );
					of.close();
				}

				// Align the color frame to depth frame
				auto processed = align.process(data);
				rs2::video_frame vf = processed.get_color_frame();
				if( vf )
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							"_aligned.png";
					std::cout << "Color (aligned) filename is : " << ss_tmp.str() << std::endl;
					stbi_write_png(ss_tmp.str().c_str(), vf.get_width(), vf.get_height(),
								   vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
				}
				vf = data.get_color_frame();
				if( vf )
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							"_raw.png";
					std::cout << "Color (raw) filename is : " << ss_tmp.str() << std::endl;
					stbi_write_png(ss_tmp.str().c_str(), vf.get_width(), vf.get_height(),
								   vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
				}
				// Save loudness
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							".loudness";
					std::cout << "Loudness filename is : " << ss_tmp.str() << std::endl;
					std::ofstream of( ss_tmp.str(), std::ios_base::binary );
					of.write( (const char *) sdr.get_loudness_data(),
							2*sdr.loudness_n_per_channel*sizeof(sdr.get_loudness_data()[0]) );
					of.close();
				}
				// Save amplitudes
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							".amp";
					std::cout << "Amplitudes filename is : " << ss_tmp.str() << std::endl;
					std::ofstream of( ss_tmp.str(), std::ios_base::binary );
					of.write( (const char *) sdr.get_amplitudes_data(),
							2*sdr.loudness_n_per_channel*sizeof(sdr.get_amplitudes_data()[0]) );
					of.close();
				}
				// Save the parameters and cli arguments
				{
					ss_tmp.str("");
					ss_tmp << depth_path << "__" <<
							std::put_time( std::localtime(&now_c), "%F__%H_%M_%S") <<
							".inf";
					std::cout << "CLI arguments filename is : " << ss_tmp.str() << std::endl;
					std::ofstream of( ss_tmp.str(), std::ios_base::binary );
					of << "Command line arguments:\n";
					for( int i=0; i<argc; ++i )
					{
						if( argv[i][0] == '-' )
							of << "\n";
						of << argv[i] << " ";
					}
					of << "\n";
					of << "Parameters:\n";
					of << "max_distance = " << sdr.max_distance << "\n";
					of << "step_distance = " << sdr.step_distance << "\n";
					of.close();
				}
			}
			std::cout << "... Pong " << std::endl;
//			save_debug_data(sound_render_data,sound_render_n*2, "AFTER FIRST CYCLE COMPLETE");
        }
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
	std::cerr << "RealSense error:" << std::endl;
	std::cerr << "(RealSense) function :" << e.get_failed_function();
	std::cerr << "(" << e.get_failed_args() << ")" << std::endl;
	std::cerr << "(RealSense) " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

