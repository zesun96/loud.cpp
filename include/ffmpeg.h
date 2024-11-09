#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace utils {

bool is_ffmpeg_installed();

std::string get_relative_path(const std::string &absolute_path);

void show_ffmpeg_normalize_suggestion(const std::string &audio_path, int argc,
                                      char *argv[]);

void normalize_audio(std::string input, std::string output);
std::string get_random_path(std::string suffix);
std::string get_random_string(const int len);

} // namespace utils

#endif