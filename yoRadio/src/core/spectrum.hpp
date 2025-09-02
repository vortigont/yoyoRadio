#pragma once
#include <mutex>

/**
 * @brief Audio Spectrum analyser
 * it uses esp-dsp lib's assembly optimized FFT processing
 * best used with esp32-p4 and esp32-s3 chips
 * based on https://github.com/espressif/esp-dsp/tree/master/applications/spectrum_box_lite
 * @note channel bith depth is always 16 here
 */
class SpectraDSP {
	size_t _fft_size, _sr, _channels;

public:
	/**
	 * @brief Construct a new Spectra DSP object
	 * 
	 * @param fft_size - FFT window size, num of freq bands is fft_size/2
	 * @note fft_size must be a power of 2 and <= CONFIG_DSP_MAX_FFT_SIZE
	 * @param sampling_rate - input sampling rate
	 * @param channels - num of input channels, must be 1 or 2
	 */
	SpectraDSP(size_t fft_size = 512, size_t sampling_rate = 48000, size_t channels = 2) :
		_fft_size(fft_size), _sr(sampling_rate), _channels(channels) {};

	~SpectraDSP();

	/**
	 * @brief init the instance, allocate mem buffers
	 * 
	 * @return true 
	 * @return false 
	 */
	bool init();

	// stop processing and release buffers memory
	void stop();

	// reinit FFT
	void reset(size_t fft_size = 512, size_t sampling_rate = 48000, size_t channels = 2);

	void setAmp(float amp){ _amp = amp; }

	float getAmp(){ return _amp; }

	/**
	 * @brief audio data sink
	 * could be used in callback for Audio lib
	 * 
	 * @param dataBuff 
	 * @param numSamples 
	 */
	void data_sink(const int16_t *dataBuff, size_t numSamples);

	size_t getDataSize() const { return _fft_size; }
	const float* getData() const { return _result; }

	// mutex for locking data buffers
	std::mutex mtx;

private:
	// flag that indicates that all buffs are alloc'ed
	bool _ready{false};
	// amplifying coefficient for scaling resulting data
	float _amp{8};
	// FFT, data and window buffers
	int16_t *_fft_w_table_sc16{nullptr}, *_audio_buffer{nullptr}, *_wnd_buffer{nullptr};
	// FFT result buffer
	float *_result{nullptr};
};