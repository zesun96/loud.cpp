
/*
clang-format off

Transcribe audio with whisper.cpp and diarize with sherpa-onnx

Prepare files:
wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin

wget https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-segmentation-models/sherpa-onnx-pyannote-segmentation-3-0.tar.bz2
tar xvf sherpa-onnx-pyannote-segmentation-3-0.tar.bz2
mv sherpa-onnx-pyannote-segmentation-3-0/model.onnx sherpa-onnx-pyannote-segmentation-3-0.onnx
rm -rf sherpa-onnx-pyannote-segmentation-3-0.tar.bz2 sherpa-onnx-pyannote-segmentation-3-0

wget https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-recongition-models/nemo_en_titanet_small.onnx

wget https://github.com/thewh1teagle/vibe/raw/refs/heads/main/samples/single.wav

Build:
cmake -B build .
cmake --build build

Run:
./build/bin/loud ggml-tiny.bin single.wav --json transcript.json
*/

#include "main.h"

#include "nlohmann/json_fwd.hpp"
#include <CLI/CLI.hpp>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sherpa-onnx/c-api/c-api.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <whisper.h>

void save_json(const std::string &json_path,
               const nlohmann::ordered_json &result_json) {
  std::ofstream json_output(json_path);
  if (json_output.is_open()) {
    json_output << result_json.dump(4); // Pretty print
  } else {
    std::cerr << "Error: Could not open file for writing: " << json_path
              << std::endl;
  }
}

void print_segment(const SherpaOnnxOfflineSpeakerDiarizationSegment &segment,
                   const std::string &text) {
  std::cout << std::fixed << std::setprecision(3) << segment.start << " -- "
            << segment.end << " speaker_" << std::setw(2) << std::setfill('0')
            << segment.speaker << ": " << text << std::endl
            << std::flush;
}

whisper_full_params create_whisper_params() {
  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  wparams.strategy = WHISPER_SAMPLING_GREEDY;
  wparams.new_segment_callback = NULL;
  wparams.language = "en";
  wparams.print_realtime = false;
  wparams.debug_mode = false;
  wparams.no_timestamps = true;
  wparams.print_special = false;
  wparams.single_segment = false;
  // wparams.split_on_word = true;

  return wparams;
}

const SherpaOnnxWave *read_wave(const std::string &path) {
  const SherpaOnnxWave *wave = SherpaOnnxReadWave(path.c_str());
  if (!wave) {
    std::cerr << "Failed to read audio file: " << path << std::endl;
    return nullptr;
  }
  return wave;
}

std::string get_default_provider() {
#if defined(__APPLE__)
  return "coreml";
#endif
  return "cpu";
}

const SherpaOnnxOfflineSpeakerDiarization *
create_sd(const std::string &segmentation_model_path,
          const std::string &embedding_model_path, int32_t num_clusters,
          std::string provider) {
  SherpaOnnxOfflineSpeakerDiarizationConfig config;
  memset(&config, 0, sizeof(config));
  config.segmentation.provider = provider.c_str();
  config.embedding.provider = provider.c_str();
  config.segmentation.pyannote.model = segmentation_model_path.c_str();
  config.embedding.model = embedding_model_path.c_str();
  config.clustering.num_clusters = num_clusters;
  const SherpaOnnxOfflineSpeakerDiarization *sd =
      SherpaOnnxCreateOfflineSpeakerDiarization(&config);
  if (!sd) {
    std::cerr << "Failed to initialize offline speaker diarization"
              << std::endl;
    return nullptr;
  }
  return sd;
}

int32_t diarization_progress_callback(int32_t num_processed_chunk,
                                      int32_t num_total_chunks, void *arg) {
  float progress =
      (static_cast<float>(num_processed_chunk) / num_total_chunks) * 100.0f;

  std::cout << "Diarization... " << static_cast<int>(progress) << "%"
            << std::endl;

  return 0;
}

int main(int argc, char *argv[]) {

  CLI::App app{"Speech to text with ONNX and Whisper"};

  std::string model_path;
  std::string audio_file;
  std::string json_path;
  std::string segmentation_model_path =
      "sherpa-onnx-pyannote-segmentation-3-0.onnx";
  std::string embedding_model_path = "nemo_en_titanet_small.onnx";
  int32_t num_speakers = 4;
  std::string provider = get_default_provider();
  bool debug = false;

  app.add_option("model", model_path, "Path to the model")->required();
  app.add_option("audio", audio_file, "Path to the audio file")->required();
  app.add_option("--json", json_path, "Path to save the JSON output");
  app.add_option("--segmentation-model", segmentation_model_path,
                 "Path to the segmentation model");
  app.add_option("--embedding-model", embedding_model_path,
                 "Path to the embedding model");
  app.add_option("--num-speakers", num_speakers,
                 "Number of speakers in the file");
  app.add_option("--provider", provider, "Onnx execution provider");
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

  // Diarize
  auto *wave = read_wave(audio_file);
  CHECK_NULL(wave);
  auto *sd = create_sd(segmentation_model_path, embedding_model_path,
                       num_speakers, provider);
  CHECK_NULL(sd);

  auto *result = SherpaOnnxOfflineSpeakerDiarizationProcessWithCallback(
      sd, wave->samples, wave->num_samples, diarization_progress_callback,
      nullptr);
  CHECK_NULL(result);
  int32_t num_segments =
      SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments(result);
  auto *segments =
      SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime(result);

  struct whisper_context_params cparams = whisper_context_default_params();
  struct whisper_context *ctx =
      whisper_init_from_file_with_params(model_path.c_str(), cparams);
  CHECK_NULL(ctx);
  whisper_full_params wparams = create_whisper_params();

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
    print_segment(segments[i], text.str());
  }

  // Write JSON file
  if (!json_path.empty()) {
    save_json(json_path, result_json);
  }

  // Cleanup
  SherpaOnnxOfflineSpeakerDiarizationDestroySegment(segments);
  SherpaOnnxOfflineSpeakerDiarizationDestroyResult(result);
  SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
  SherpaOnnxFreeWave(wave);
  whisper_free(ctx);
  return 0;
}