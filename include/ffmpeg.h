#pragma once

#include <string>

namespace ffmpeg {

void show_ffmpeg_normalize_suggestion(const std::string &audio_path, int argc,
                                      char *argv[]);

void normalize_audio(std::string input, std::string output);

} // namespace ffmpeg
