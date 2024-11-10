#include <iostream>
#include <sstream>
#include <string>
#include <whisper.h>

namespace transcribe {
whisper_full_params create_whisper_params(std::string language, bool debug) {

  if (!debug) {
    whisper_log_set([](enum ggml_log_level, const char *, void *) {}, NULL);
  }

  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  // https://github.com/ggerganov/whisper.cpp/blob/master/examples/talk/talk.cpp
  wparams.new_segment_callback = NULL;
  wparams.language = language.c_str();
  wparams.print_realtime = false;
  wparams.debug_mode = false;
  wparams.no_timestamps = true;
  wparams.print_special = false;
  wparams.translate = false;
  wparams.single_segment = true;
  wparams.no_context = true;
  wparams.max_tokens = 32;
  // wparams.split_on_word = true;

  return wparams;
}

std::string transcribe_audio_chunk(whisper_context *ctx,
                                   const whisper_full_params &params,
                                   const float *samples, int n_samples) {
  // Process the chunk with Whisper
  if (whisper_full(ctx, params, samples, n_samples) != 0) {
    std::cerr << "Failed to process audio chunk." << std::endl;
    return "";
  }

  // Get and return the chunk transcription
  const int n_segments = whisper_full_n_segments(ctx);
  std::ostringstream transcription;

  for (int j = 0; j < n_segments; j++) {
    const char *segment_text = whisper_full_get_segment_text(ctx, j);
    transcription << segment_text << " ";
  }

  return transcription.str();
}

} // namespace transcribe