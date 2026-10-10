#include "../include/AudioStreamer.h"

#include <string.h>

AudioStreamer* AudioStreamer::current_streamer = nullptr;

mm_word AudioStreamer::stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
  if (current_streamer != nullptr) {
    return current_streamer->fill_buffer(length, dest, format);
  }
  memset(dest, 0, length);
  return length;
}

AudioStreamer::AudioStreamer(void) {
}

AudioStreamer::~AudioStreamer(void) {
  this->stop();
  if (current_streamer == this) {
    current_streamer = nullptr;
  }
}

void AudioStreamer::init(void) {
  static bool maxmod_initialized = false;
  if (!maxmod_initialized) {
    mmInitNoSoundbank();
    maxmod_initialized = true;
  }
}

FILE* AudioStreamer::open_audio_file(const std::string& path) {
  // Candidate path variations to try
  std::string candidates[] = {
    path,
    path + ".raw",
    "nitro:/" + path,
    "nitro:/" + path + ".raw",
    "bgm/" + path,
    "bgm/" + path + ".raw",
    "nitro:/bgm/" + path,
    "nitro:/bgm/" + path + ".raw"
  };

  for (const auto& candidate : candidates) {
    FILE* f = fopen(candidate.c_str(), "rb");
    if (f != nullptr) {
      return f;
    }
  }

  return nullptr;
}

mm_word AudioStreamer::fill_buffer(mm_word length, mm_addr dest, mm_stream_formats format) {
  (void)format;

  // When paused or file is closed, output digital silence without advancing file position:
  if (this->file == nullptr || this->is_stream_paused) {
    memset(dest, 0, length);
    return length;
  }

  u8* target = reinterpret_cast<u8*>(dest);
  size_t bytes_read = fread(target, 1, length, this->file);

  // If reached end-of-file:
  if (bytes_read < length) {
    if (this->should_loop) {
      fseek(this->file, 0, SEEK_SET);
      size_t remaining = length - bytes_read;
      size_t second_read = fread(target + bytes_read, 1, remaining, this->file);
      bytes_read += second_read;
    }

    if (bytes_read < length) {
      memset(target + bytes_read, 0, length - bytes_read);
    }
  }

  return length;
}

bool AudioStreamer::play(const std::string& filepath, u32 sample_rate, bool loop) {
  this->stop();

  this->file = this->open_audio_file(filepath);
  if (this->file == nullptr) {
    return false;
  }

  this->should_loop = loop;
  this->current_sample_rate = sample_rate;

  current_streamer = this;

  mm_stream stream;
  stream.sampling_rate = sample_rate;
  stream.buffer_length = 4096;  // 4096 samples double-buffered (~185ms safety buffer)
  stream.callback = AudioStreamer::stream_callback;
  stream.format = MM_STREAM_8BIT_MONO;
  stream.timer = MM_TIMER0;
  stream.manual = true;

  mmStreamOpen(&stream);
  mmStreamVolume(this->volume);

  this->is_playing = true;
  this->is_stream_paused = false;

  return true;
}

void AudioStreamer::update(void) {
  if (this->is_playing) {
    mmStreamUpdate();
  }
}

void AudioStreamer::pause(void) {
  this->is_stream_paused = true;
}

void AudioStreamer::resume(void) {
  this->is_stream_paused = false;
}

void AudioStreamer::stop(void) {
  if (this->is_playing) {
    mmStreamClose();
    this->is_playing = false;
  }

  if (this->file != nullptr) {
    fclose(this->file);
    this->file = nullptr;
  }

  this->is_stream_paused = false;
}

void AudioStreamer::set_volume(u8 vol) {
  this->volume = (vol > 127) ? 127 : vol;
  if (this->is_playing) {
    mmStreamVolume(this->volume);
  }
}

u8 AudioStreamer::get_volume(void) const {
  return this->volume;
}

bool AudioStreamer::is_active(void) const {
  return this->is_playing;
}

bool AudioStreamer::is_paused(void) const {
  return this->is_stream_paused;
}
