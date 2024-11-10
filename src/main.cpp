#include "config.h"
#include "diarization.h"
#include "download.h"
#include "segments.h"
#include "sherpa-onnx/c-api/c-api.h"
#include "spinner.h"
#include "transcribe.h"
#include "utils.h"
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <whisper.h>

namespace fs = std::filesystem;
using spinner::Spinner;

int main(int argc, char *argv[]) {

  CLI::App app{"Loud.cpp\nSpeech to text with ONNX and Whisper\n"};

  std::string whisper_model_path = config::ggml_tiny_name;
  std::string audio_file;
  std::string json_path;
  std::string segmentation_model_path = config::segmentation_name;
  std::string embedding_model_path = config::embedding_name;
  std::string language = "en";
  int32_t num_speakers = 4;
  int32_t onnx_num_threads = 4;
  std::string onnx_provider = diarization::get_default_provider();
  bool debug = false;
  bool download_models = false;

  app.add_option("audio", audio_file, "Path to the audio file")->required();
  app.add_option("--language", language,
                 "Language to transcribe with (Default: en)");
  app.add_flag(
      "--download-models", download_models,
      "Download models (pyannote segment, whisper tiny, nemo small en)");
  app.add_option("--json", json_path, "Path to save the JSON output");
  app.add_option("--whisper-model", whisper_model_path, "Path to the model");
  app.add_option("--segmentation-model", segmentation_model_path,
                 "Path to the segmentation model");
  app.add_option("--embedding-model", embedding_model_path,
                 "Path to the embedding model");
  app.add_option("--num-speakers", num_speakers,
                 "Number of speakers in the file");
  app.add_option("--onnx-provider", onnx_provider, "Onnx execution provider");
  app.add_option("--onnx-num-threads", onnx_num_threads,
                 "Onnx number of threads (Default: 4)");
  app.add_flag("--debug", debug, "Enable debug output");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  // Download models
  if (download_models) {
    download::download_models_if_needed();
  }

  if (!fs::exists(audio_file)) {
    std::cerr << "audio file " << audio_file << " not exists!";
    return EXIT_FAILURE;
  }

  // Check if models exists
  if (!utils::check_model_exists(embedding_model_path, argc, argv))
    return EXIT_FAILURE;
  if (!utils::check_model_exists(segmentation_model_path, argc, argv))
    return EXIT_FAILURE;
  if (!utils::check_model_exists(whisper_model_path, argc, argv))
    return EXIT_FAILURE;

  // Read wave file
  auto wave = diarization::prepare_audio_file(audio_file, argc, argv);
  if (!wave)
    return EXIT_FAILURE;

  // Start diarization
  Spinner spinner("Starting diarization...");
  auto *sd =
      diarization::create_sd(segmentation_model_path, embedding_model_path,
                             num_speakers, onnx_provider, onnx_num_threads);
  CHECK_NULL(sd);
  spinner.start();

  const auto *result = diarization::run_diarization(sd, wave, spinner);
  spinner.stop();
  CHECK_NULL(result);
  const auto num_segments =
      SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments(result);
  const auto *segments =
      SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime(result);

  // Start transcribe
  const auto cparams = whisper_context_default_params();
  auto *ctx =
      whisper_init_from_file_with_params(whisper_model_path.c_str(), cparams);
  CHECK_NULL(ctx);
  const auto params = transcribe::create_whisper_params(language, debug);
  auto json =
      segments::process_segments(segments, num_segments, wave, ctx, params);

  // Write JSON file
  if (!json_path.empty()) {
    utils::save_json(json_path, json);
  }

  // Cleanup
  SherpaOnnxOfflineSpeakerDiarizationDestroySegment(segments);
  SherpaOnnxOfflineSpeakerDiarizationDestroyResult(result);
  SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
  SherpaOnnxFreeWave(wave);
  whisper_free(ctx);
  return 0;
}