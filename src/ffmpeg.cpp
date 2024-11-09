#include "main.h"
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sherpa-onnx/c-api/c-api.h>
#include <string>
#include <whisper.h>

namespace fs = std::filesystem;

namespace utils {

bool is_ffmpeg_installed() {
  // Check for ffmpeg on Unix-like systems
#ifdef __unix__
  return std::system("ffmpeg -h > /dev/null 2>&1") == 0;
#elif _WIN32
  // Check for ffmpeg.exe on Windows
  return std::system("ffmpeg -h >NUL 2>NUL") == 0;
#else
  // Unknown platform, assume ffmpeg is not installed
  return false;
#endif
}

std::string get_relative_path(const std::string &absolute_path) {
  fs::path cwd = fs::current_path();
  fs::path file_path(absolute_path);

  // Make paths absolute to ensure correct relative path calculation
  cwd = fs::absolute(cwd);
  file_path = fs::absolute(file_path);

  // Calculate the relative path
  fs::path relative_path = fs::relative(file_path, cwd);

  return relative_path.string();
}

void show_ffmpeg_normalize_suggestion(const std::string &audio_path, int argc,
                                      char *argv[]) {
  std::string platform = "";

#ifdef __APPLE__
  platform = "macOS";
#elif _WIN32
  platform = "Windows";
#elif __linux__
  platform = "Linux";
#else
  platform = "Unknown";
#endif

  std::cout << "It seems like you're on " << platform
            << ". Here's how you can normalize your audio using ffmpeg:\n";

  if (!is_ffmpeg_installed()) {
    std::cout << "**Install ffmpeg:**\n";
    if (platform == "macOS") {
      std::cout << "brew install ffmpeg\n";
    } else if (platform == "Windows") {
      std::cout << "winget install --id=Gyan.FFmpeg\n";
    } else if (platform == "Linux") {
      std::cout << "sudo apt install ffmpeg\n";
    }
  }

  std::cout << "\n**Normalize the audio:**\n";
  std::cout << "ffmpeg -i " << audio_path
            << " -ar 16000 -ac 1 -c:a pcm_s16le output.wav\n";

  // Suggest running the command again on the output file, replacing audio_path
  // with output.wav
  std::cout << "\n**Once the normalization process is complete, you can run "
               "the command again on the output file:**\n";
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];
    if (i == 0) {
      std::string relative_path = get_relative_path(arg);
      std::cout << relative_path << " ";
    } else if (arg == audio_path) {
      std::cout << "output.wav" << " ";
    } else {
      std::cout << arg << " ";
    }
  }
  std::cout << "\n";
}

} // namespace utils