#include "math.h"
#include "dsp_common.h"
#include "dsps_math.h"
#include "dsps_fft2r.h"
#include "dsps_wind.h"
#include "spectrum.hpp"
#include "esp_heap_caps.h"
#include "esp32-hal-psram.h"
#include "esp_log.h"

static const char *TAG = "spectrum";

SpectraDSP::~SpectraDSP(){
  stop();
};

bool SpectraDSP::init(){
  if(!dsp_is_power_of_two(_fft_size) || _fft_size > CONFIG_DSP_MAX_FFT_SIZE){
    ESP_LOGE(TAG, "FFT size is too large or not a power of 2!");
    return false;
  }

  {
    // use scoped lock
    std::lock_guard lock(mtx);
    if ( psramFound() ) {
      // Alloc FFT table in PSRAM
      _fft_w_table_sc16 = (int16_t *)heap_caps_aligned_alloc(16, _fft_size * sizeof(int16_t), MALLOC_CAP_SPIRAM);
      // Allocate audio buffer
      _audio_buffer = (int16_t *)heap_caps_aligned_alloc(16, (_fft_size + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_SPIRAM);
      // Allocate buffer for window
      _wnd_buffer = (int16_t *)heap_caps_aligned_alloc(16, (_fft_size + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_SPIRAM);
      _result = (float *)heap_caps_malloc(_fft_size * sizeof(float), MALLOC_CAP_SPIRAM);
    } else {
      // Alloc FFT table
      _fft_w_table_sc16 = (int16_t *)heap_caps_aligned_alloc(16, _fft_size * sizeof(int16_t), MALLOC_CAP_DEFAULT);
      // Allocate audio buffer and check for result
      _audio_buffer = (int16_t *)heap_caps_aligned_alloc(16, (_fft_size + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_DEFAULT);
      // Allocate buffer for window
      _wnd_buffer = (int16_t *)heap_caps_aligned_alloc(16, (_fft_size + 16) * sizeof(int16_t) * _channels, MALLOC_CAP_DEFAULT);
      _result = (float *)heap_caps_malloc(_fft_size * sizeof(float), MALLOC_CAP_DEFAULT);
    }
  }
  // check if all alloc's were fine
  if (!_fft_w_table_sc16 || !_audio_buffer || !_wnd_buffer || !_result){
    stop(); // stop will create lock again
    return false;
  }
  // set FFT table
  dsps_fft2r_init_sc16(_fft_w_table_sc16, _fft_size);

  // Generate window and convert it to int16_t
  // dsps_wind_blackman_harris_f32(_result, _fft_size);
  dsps_wind_nuttall_f32(_result, _fft_size);
  for (int i = 0 ; i < _fft_size; i++) {
    _wnd_buffer[i * 2 + 0] = (int16_t)(_result[i] * 32767);
    _wnd_buffer[i * 2 + 1] = _wnd_buffer[i * 2 + 0];
  }

  _ready = true;
  return true;
}

void SpectraDSP::data_sink(const int16_t *dataBuff, size_t numSamples){
  if (!_ready) return;

  // grab mutex
  std::lock_guard lock(mtx);

  // here I'll just chop off all samples above fft size, which is not right for full signal analysis, but
  // it works pretty OKish for now and saves CPU resources
  // todo: track buffer filling and ordering probably, or implement some simplification
  uint32_t len = numSamples > _fft_size ? _fft_size : numSamples;
  // Multiply input stream with window coefficients
  dsps_mul_s16(dataBuff, _wnd_buffer, _audio_buffer, len * _channels, 1, 1, 1, 15);
  // complex FFT of radix 2
  dsps_fft2r_sc16(_audio_buffer, _fft_size);
  // Call FFT bit reverse
  dsps_bit_rev_sc16_ansi(_audio_buffer, _fft_size);
  // Convert spectrum from two input channels to two spectrums for two channels
  dsps_cplx2reC_sc16(_audio_buffer, _fft_size);

  // The output data array presented as moving average for input in dB
  dsps_mulc_f32(_result, _result, _fft_size, _avg, 1, 1);  // scale down existing values

  for (int i = 0 ; i != _fft_size ; i++) {
    float spectrum_sqr = _audio_buffer[i * 2 + 0] * _audio_buffer[i * 2 + 0] + _audio_buffer[i * 2 + 1] * _audio_buffer[i * 2 + 1];
    // Bels (I won't x10 here for dB but rather apply amp coefficient later to avoid extra multiplication)
    float spectrum_B = log10f(0.1 + spectrum_sqr);
    // Multiply with amp coefficient to fit it better to screen size
    _result[i] += _amp * spectrum_B;
  }
}

void SpectraDSP::stop(){
  std::lock_guard lock(mtx);
  _ready = false;
  dsps_fft2r_deinit_sc16();
  free (_audio_buffer);
  _audio_buffer = nullptr;
  free (_wnd_buffer);
  _wnd_buffer = nullptr;
  free (_result);
  _result = nullptr;
}

void SpectraDSP::reset(size_t fft_size, size_t sampling_rate, size_t channels){
  stop();
  _fft_size = fft_size;
  _sr = sampling_rate;
  _channels = channels;
  init();
}
