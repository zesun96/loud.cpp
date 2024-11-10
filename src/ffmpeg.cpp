#include "subprocess/ProcessBuilder.hpp"
#include "subprocess/basic_types.hpp"
#include "utils.h"
#include <CLI/CLI.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sherpa-onnx/c-api/c-api.h>
#include <string>
#include <subprocess.hpp>
#include <whisper.h>

namespace ffmpeg {

void normalize_audio(std::string input, std::string output) {
  std::cout << "Normalizing audio from " << input << " to " << output;
  using subprocess::CompletedProcess;
  using subprocess::PipeOption;
  using subprocess::RunBuilder;

  CompletedProcess proc = subprocess::run(
      {
          "ffmpeg",
          "-i",
          input,
          "-ar",
          "16000",
          "-ac",
          "1",
          "-c:a",
          "pcm_s16le",
          output,
      },
      RunBuilder().cout(PipeOption::cerr).cerr(PipeOption::pipe));
}

void show_ffmpeg_normalize_suggestion(const std::string &audio_path, int argc,
                                      char *argv[]) {

  std::cout << "Here's how you can normalize your audio using ffmpeg:\n";

  if (!utils::is_program_installed("ffmpeg")) {
    std::cout << "**Install ffmpeg:**\n";
#ifdef __APPLE__
    std::cout << "brew install ffmpeg\n";
#elif __linux__
    std::cout << "sudo apt install ffmpeg\n";
#else
    std::cout << "winget install --id=Gyan.FFmpeg\n";
#endif
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
      std::string relative_path = utils::get_relative_path(arg);
      std::cout << relative_path << " ";
    } else if (arg == audio_path) {
      std::cout << "output.wav" << " ";
    } else {
      std::cout << arg << " ";
    }
  }
  std::cout << "\n";
}

} // namespace ffmpeg