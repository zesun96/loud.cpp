/*
Transcribe audio with whisper.cpp and diarize with sherpa-onnx

Prepare models:
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin

wget https://github.com/thewh1teagle/vibe/raw/refs/heads/main/samples/single.wav

wget \
  https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-segmentation-models/sherpa-onnx-pyannote-segmentation-3-0.tar.bz2
tar xvf sherpa-onnx-pyannote-segmentation-3-0.tar.bz2
rm sherpa-onnx-pyannote-segmentation-3-0.tar.bz2

wget \
  https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-recongition-models/nemo_en_titanet_large.onnx

Build:
cmake -B build .
cmake --build build
./target/bin/loud ggml-tiny.bin single.wav --json transcript.json
*/

#include "nlohmann/json_fwd.hpp"
#include <CLI/CLI.hpp>
#include <iomanip>

#include <iostream>
#include <nlohmann/json.hpp>
#include <sherpa-onnx/c-api/c-api.h>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <whisper.h>

static void cb_log_disable(enum ggml_log_level, const char *, void *) {}

int main(int argc, char *argv[]) {

  CLI::App app{"Speech to text with ONNX and Whisper"};

  std::string model_path;
  std::string audio_file;
  std::string json_path;
  bool debug = false;

  app.add_option("model", model_path, "Path to the model")->required();
  app.add_option("audio", audio_file, "Path to the audio file")->required();
  app.add_option("--json", json_path, "Path to save the JSON output");
  app.add_flag("--debug", debug, "Enable debug output");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  if (!debug) {
    // Supress whisper logs
    whisper_log_set(cb_log_disable, NULL);
  }

  const SherpaOnnxWave *wave = SherpaOnnxReadWave(audio_file.c_str());
  if (!wave) {
    std::cerr << "Failed to read audio file: " << audio_file << std::endl;
    return EXIT_FAILURE;
  }

  // Diarize
  SherpaOnnxOfflineSpeakerDiarizationConfig config;
  memset(&config, 0, sizeof(config));
#if defined(__APPLE__)
  config.segmentation.provider = "coreml";
  config.embedding.provider = "coreml";
#endif
  config.segmentation.pyannote.model =
      "sherpa-onnx-pyannote-segmentation-3-0/model.onnx";
  config.embedding.model = "nemo_en_titanet_large.onnx";
  config.clustering.num_clusters = 4;
  const SherpaOnnxOfflineSpeakerDiarization *sd =
      SherpaOnnxCreateOfflineSpeakerDiarization(&config);
  if (!sd) {
    std::cerr << "Failed to initialize offline speaker diarization"
              << std::endl;
    return EXIT_FAILURE;
  }

  const SherpaOnnxOfflineSpeakerDiarizationResult *result =
      SherpaOnnxOfflineSpeakerDiarizationProcess(sd, wave->samples,
                                                 wave->num_samples);
  if (!result) {
    std::cerr << "Failed to do speaker diarization" << std::endl;
    return EXIT_FAILURE;
  }

  int32_t num_segments =
      SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments(result);

  const SherpaOnnxOfflineSpeakerDiarizationSegment *segments =
      SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime(result);

  struct whisper_context_params cparams = whisper_context_default_params();
  struct whisper_context *ctx =
      whisper_init_from_file_with_params(model_path.c_str(), cparams);

  if (!ctx) {
    std::cerr << "Error: Failed to initialize whisper context." << std::endl;
    return EXIT_FAILURE;
  }

  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  wparams.strategy = WHISPER_SAMPLING_GREEDY;
  wparams.new_segment_callback = NULL;
  wparams.language = "en";
  wparams.print_realtime = false;
  wparams.debug_mode = false;
  wparams.no_timestamps = true;
  wparams.print_special = false;
  wparams.single_segment = true;
  // wparams.split_on_word = true;

  nlohmann::ordered_json result_json;
  for (int32_t i = 0; i != num_segments; ++i) {

    // Calculate start and end samples for the segment
    int32_t start_sample = static_cast<int32_t>(segments[i].start * 16000);
    int32_t end_sample = static_cast<int32_t>(segments[i].end * 16000);

    // skip segments that are less than 1s
    if ((end_sample - start_sample) < 16000) {
      continue;
    }

    // Ensure start and end are within bounds
    if (start_sample < 0)
      start_sample = 0;
    if (end_sample > wave->num_samples)
      end_sample = wave->num_samples;

    // Prepare buffer for the segment
    std::vector<float> segment_data(wave->samples + start_sample,
                                    wave->samples + end_sample);

    // Fill with zeros up to 10 seconds
    if (segment_data.size() < 16000 * 30) {
      segment_data.resize(16000 * 30, 0.0f);
    }
    // Process the buffered (padded) segment with Whisper
    if (whisper_full_parallel(ctx, wparams, segment_data.data(),
                              segment_data.size(), 4) != 0) {
      std::cerr << argv[0] << ": Failed to process segment." << std::endl;
      return EXIT_FAILURE;
    }

    // Get and print segment transcription
    const int n_segments = whisper_full_n_segments(ctx);

    std::ostringstream text;

    for (int j = 0; j < n_segments; j++) {
      const char *segment_text = whisper_full_get_segment_text(ctx, j);
      text << segment_text << " ";
    }

    if (!json_path.empty()) {
      result_json.push_back({{"text", text.str()},
                             {"start", segments[i].start},
                             {"end", segments[i].end},
                             {"speaker", segments[i].speaker}});
    }
    std::cout << std::fixed << std::setprecision(3) << segments[i].start
              << " -- " << segments[i].end << " speaker_" << std::setw(2)
              << std::setfill('0') << segments[i].speaker << ": " << text.str()
              << std::endl
              << std::flush;
  }

  // Write JSON file
  if (!json_path.empty()) {
    std::ofstream json_output(json_path);
    if (json_output.is_open()) {
      json_output << result_json.dump(4); // Pretty print JSON with indentation
      std::cout << "JSON result saved to: " << json_path << std::endl;
    } else {
      std::cerr << "Error: Could not open file " << json_path << " for writing."
                << std::endl;
    }
  }

  SherpaOnnxOfflineSpeakerDiarizationDestroySegment(segments);
  SherpaOnnxOfflineSpeakerDiarizationDestroyResult(result);
  SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
  SherpaOnnxFreeWave(wave);
  whisper_free(ctx);
  return 0;
}