#ifndef AUDIOSTREAMER_H
#define AUDIOSTREAMER_H

#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include <string>

/**
 * @brief Handles background music streaming in chunks directly from NitroFS
 *        using Maxmod's streaming API to avoid loading multi-megabyte audio tracks
 *        entirely into RAM.
 */
class AudioStreamer {
 private:
  static AudioStreamer* current_streamer;
  static mm_word stream_callback(mm_word length, mm_addr dest, mm_stream_formats format);

  FILE* file = nullptr;
  bool is_playing = false;
  bool is_stream_paused = false;
  bool should_loop = true;
  u32 current_sample_rate = 22050;
  u8 volume = 110;  // 0 - 127

  FILE* open_audio_file(const std::string& path);
  mm_word fill_buffer(mm_word length, mm_addr dest, mm_stream_formats format);

 public:
  AudioStreamer(void);
  ~AudioStreamer(void);

  void init(void);
  bool play(const std::string& filepath, u32 sample_rate = 22050, bool loop = true);
  void update(void);
  void pause(void);
  void resume(void);
  void stop(void);

  void set_volume(u8 vol);
  u8 get_volume(void) const;

  bool is_active(void) const;
  bool is_paused(void) const;
};

#endif  // AUDIOSTREAMER_H
