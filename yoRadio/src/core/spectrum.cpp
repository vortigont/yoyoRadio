#include "spectrum.hpp"
#include "dsps_math.h"
#include "dsps_fft2r.h"
#include "dsps_wind_blackman_harris.h"
#include "esp_heap_caps.h"

static const char *TAG = "spectra";

bool SpectraDSP::init(){
  esp_err_t ret = dsps_fft2r_init_sc16(NULL, CONFIG_DSP_MAX_FFT_SIZE);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Not possible to initialize FFT esp-dsp from library!");
    return false;
  }

  if ( psramFound() ) {
    // Allocate audio buffer and check for result
    _audio_buffer = (int16_t *)heap_caps_aligned_alloc(16, (SPECTRA_DSP_BUFFER_PROCESS_SIZE + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_SPIRAM);
    // Allocate buffer for window
    _wind_buffer = (int16_t *)heap_caps_aligned_alloc(16, (SPECTRA_DSP_BUFFER_PROCESS_SIZE + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_SPIRAM);
    _result = (float *)heap_caps_malloc(SPECTRA_DSP_BUFFER_PROCESS_SIZE * sizeof(float), MALLOC_CAP_SPIRAM);
  } else {
    // Allocate audio buffer and check for result
    _audio_buffer = (int16_t *)heap_caps_aligned_alloc(16, (SPECTRA_DSP_BUFFER_PROCESS_SIZE + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_DEFAULT);
    // Allocate buffer for window
    _wind_buffer = (int16_t *)heap_caps_aligned_alloc(16, (SPECTRA_DSP_BUFFER_PROCESS_SIZE + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_DEFAULT);
    _result = (float *)heap_caps_malloc(SPECTRA_DSP_BUFFER_PROCESS_SIZE * sizeof(float), MALLOC_CAP_DEFAULT);
  }

  // Generate window and convert it to int16_t
  dsps_wind_blackman_harris_f32(_result, SPECTRA_DSP_BUFFER_PROCESS_SIZE);
  for (int i = 0 ; i < SPECTRA_DSP_BUFFER_PROCESS_SIZE; i++) {
    _wind_buffer[i * 2 + 0] = (int16_t)(_result[i] * 32767);
    _wind_buffer[i * 2 + 1] = _wind_buffer[i * 2 + 0];
  }

  return (_audio_buffer && _wind_buffer && _result);
}

void SpectraDSP::data_sink(const int16_t* outBuff, int32_t validSamples, bool *continueI2S){
  uint32_t len = validSamples > SPECTRA_DSP_BUFFER_PROCESS_SIZE ? SPECTRA_DSP_BUFFER_PROCESS_SIZE : validSamples;
  // Multiply input stream with window coefficients
  dsps_mul_s16(outBuff, _wind_buffer, _audio_buffer, len * _channels, 1, 1, 1, 15);

  // Call FFT bit reverse
  dsps_fft2r_sc16_arp4(_audio_buffer, SPECTRA_DSP_BUFFER_PROCESS_SIZE);   // TOD: change this!
  dsps_bit_rev_sc16_ansi(_audio_buffer, SPECTRA_DSP_BUFFER_PROCESS_SIZE);
  // Convert spectrum from two input channels to two
  // spectrums for two channels.
  dsps_cplx2reC_sc16(_audio_buffer, SPECTRA_DSP_BUFFER_PROCESS_SIZE);
  // The output data array presented as moving average for input in dB
  for (int i = 0 ; i < SPECTRA_DSP_BUFFER_PROCESS_SIZE ; i++) {
    float spectrum_sqr = _audio_buffer[i * 2 + 0] * _audio_buffer[i * 2 + 0] + _audio_buffer[i * 2 + 1] * _audio_buffer[i * 2 + 1];
    float spectrum_dB = 10 * log10f(0.1 + spectrum_sqr);
    // Multiply with sime coefficient for better view data on screen
    spectrum_dB = 4 * spectrum_dB;
    // Apply moving average of spectrum
    _result[i] = 0.8 * _result[i] + 0.2 * spectrum_dB;
  }

}
