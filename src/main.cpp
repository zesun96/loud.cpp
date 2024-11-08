
/*
Transcribe audio with whisper.cpp and diarize with sherpa-onnx

Prepare models:
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin
wget https://github.com/thewh1teagle/vibe/raw/refs/heads/main/samples/single.wav

Build:
cmake -B build .
cmake --build build
./target/bin/loud ggml-tiny.bin single.wav
*/

#include "sherpa-onnx/c-api/c-api.h"
#include "whisper.h"
#include <iostream>
#include <stdio.h>

static void whisper_print_segment_callback(struct whisper_context *ctx,
                                           struct whisper_state * /*state*/,
                                           int n_new, void *user_data) {

  const int n_segments = whisper_full_n_segments(ctx);

  int64_t t0 = 0;
  int64_t t1 = 0;

  // print the last n_new segments
  const int s0 = n_segments - n_new;

  if (s0 == 0) {
    printf("\n");
  }

  for (int i = s0; i < n_segments; i++) {
    const char *text = whisper_full_get_segment_text(ctx, i);
    printf("%s", text);
    fflush(stdout);
  }
}

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

  const SherpaOnnxWave *wave = SherpaOnnxReadWave(audio_file);
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

  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  wparams.strategy = WHISPER_SAMPLING_GREEDY;
  wparams.new_segment_callback = whisper_print_segment_callback;
  wparams.language = "en";
  wparams.print_realtime = false;
  wparams.debug_mode = false;

  if (whisper_full_parallel(ctx, wparams, wave->samples, wave->num_samples,
                            4) != 0) {
    fprintf(stderr, "%s: failed to process audio\n", argv[0]);
    return 10;
  }

  SherpaOnnxFreeWave(wave);
  whisper_free(ctx);
  return 0;
}