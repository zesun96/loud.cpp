
/*
Transcribe audio with whisper.cpp and diarize with sherpa-onnx

Prepare models:
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin
wget https://github.com/thewh1teagle/vibe/raw/refs/heads/main/samples/short.wav

Build:
cmake -B build .
cmake --build build
./target/bin/loud ggml-tiny.bin short.wav
*/

#include "whisper.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <model_path> <audio_file>"
              << std::endl;
    return 1;
  }

  const char *model_path = argv[1];
  const char *audio_file = argv[2];


  std::cout << "Model path: " << model_path << std::endl;
  std::cout << "Audio file path: " << audio_file << std::endl;

  const SherpaOnnxWave* wave = SherpaOnnxReadWave(audio_file);
  if (!wave) {
    std::cerr << "Failed to read audio file: " << audio_file << std::endl;
    return 2;
  }

  struct whisper_context_params cparams = whisper_context_default_params();
  struct whisper_context *ctx =
      whisper_init_from_file_with_params(model_path, cparams);

  if (ctx == nullptr) {
    fprintf(stderr, "error: failed to initialize whisper context\n");
    return 3;
  }

  std::cout << "Hello from loud.cpp!" << std::endl;

  return 0;
}