#include "Audio.h"

#define SPECTRA_DSP_SAMPLE_RATE     (48000)		// hardcoded Audio lib
#define SPECTRA_DSP_BITS_PER_CHANNEL     16
// Input buffer size
#define SPECTRA_DSP_BUFFER_PROCESS_SIZE 512

class SpectraDSP {
	uint32_t _channels{2};

	int16_t *_audio_buffer, *_wind_buffer;
	float* _result;

public:
	bool init();

	// i2s data sink
	void data_sink(const int16_t* outBuff, int32_t validSamples, bool *continueI2S);

	size_t getDataSize() const { return SPECTRA_DSP_BUFFER_PROCESS_SIZE; }
	const float* getData() const { return _result; }
	
};