#include <string>
#include <whisper.h>

namespace transcribe {
whisper_full_params create_whisper_params(std::string language) {
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

} // namespace transcribe