#pragma once

#include <string>
#include <whisper.h>

namespace transcribe {
std::string transcribe_audio_chunk(whisper_context *ctx,
                                   const whisper_full_params &params,
                                   const float *samples, int n_samples);

whisper_full_params create_whisper_params(std::string &language, bool debug);
} // namespace transcribe