#include "config.h"
#include "diarization.h"
#include "download.h"
#include "ffmpeg.h"
#include "spinner.h"
#include "transcribe.h"
#include "utils.h"
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <whisper.h>

namespace fs = std::filesystem;
using download::download_file;
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

  if (download_models) {

    if (!fs::exists(config::segmentation_name)) {
      download_file(config::segmentation_url, config::segmentation_name);
    }
    if (!fs::exists(config::embedding_name)) {
      download_file(config::embedding_url, config::embedding_name);
    }
    if (!fs::exists(config::ggml_tiny_name)) {
      download_file(config::ggml_tiny_url, config::ggml_tiny_name);
    }
  }

  if (!fs::exists(embedding_model_path)) {
    std::cout << "Embedding model not found at " << embedding_model_path
              << std::endl
              << std::endl
              << "Please execute the following command to download models "
                 "automatically:"
              << std::endl
              << utils::get_argv_line(argc, argv) << " --download-models"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check file extension
  if (!fs::exists(audio_file)) {
    std::cerr << "audio file " << audio_file << " not exists!";
    return EXIT_FAILURE;
  }

  if (!fs::exists(whisper_model_path)) {
    std::cerr << "whisper model " << whisper_model_path << " not exists!";
    return EXIT_FAILURE;
  }

  // Diarize
  auto wave = diarization::read_wave(audio_file);
  if (wave == nullptr) {
    if (utils::is_program_installed("ffmpeg")) {
      auto random_path = utils::get_random_path(".wav");
      std::cout << "normalize audio..." << std::endl;
      ffmpeg::normalize_audio(audio_file, random_path);
      wave = diarization::read_wave(random_path);
    } else {
      ffmpeg::show_ffmpeg_normalize_suggestion(audio_file, argc, argv);
      return EXIT_FAILURE;
    }
  }
  CHECK_NULL(wave);

  if (wave->sample_rate != 16000) {
    std::cerr << "Error: The audio file must have a sample rate of 16,000 "
                 "Hz. Found "
              << wave->sample_rate << " Hz." << std::endl;
    ffmpeg::show_ffmpeg_normalize_suggestion(audio_file, argc, argv);
    return EXIT_FAILURE;
  }

  Spinner spinner("Starting diarization...");
  auto *sd =
      diarization::create_sd(segmentation_model_path, embedding_model_path,
                             num_speakers, onnx_provider, onnx_num_threads);
  CHECK_NULL(sd);

  spinner.start();
  auto *result = SherpaOnnxOfflineSpeakerDiarizationProcessWithCallback(
      sd, wave->samples, wave->num_samples,
      [](int32_t num_processed_chunk, int32_t num_total_chunks,
         void *arg) -> int32_t {
        return diarization::diarization_progress_callback(
            num_processed_chunk, num_total_chunks, static_cast<Spinner *>(arg));
      },
      &spinner);
  spinner.stop();
  CHECK_NULL(result);
  int32_t num_segments =
      SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments(result);
  auto *segments =
      SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime(result);

  if (!debug) {
    whisper_log_set([](enum ggml_log_level, const char *, void *) {}, NULL);
  }
  auto cparams = whisper_context_default_params();
  auto *ctx =
      whisper_init_from_file_with_params(whisper_model_path.c_str(), cparams);
  CHECK_NULL(ctx);
  auto wparams = transcribe::create_whisper_params(language);

  nlohmann::ordered_json result_json;
  // Iterate diarize segments

  for (int32_t i = 0; i != num_segments; ++i) {

    // Calculate start and end samples for the segment
    int32_t start_sample = static_cast<int32_t>(segments[i].start * 16000);
    int32_t end_sample = static_cast<int32_t>(segments[i].end * 16000);

    // skip segments that are less than 0.5s
    if ((end_sample - start_sample) < 16000 / 2) {
      continue;
    }

    // Ensure start and end are within bounds
    if (start_sample < 0) {
      start_sample = 0;
    }
    if (end_sample > wave->num_samples) {
      end_sample = wave->num_samples;
    }

    int32_t segment_length = end_sample - start_sample;
    int32_t chunk_size = 16000 * 30; // 30 seconds in samples

    // Process longer segments in chunks (no more than 30 seconds each)
    if (segment_length > chunk_size) {
      for (int32_t chunk_start = start_sample; chunk_start < end_sample;
           chunk_start += chunk_size) {
        int32_t chunk_end = std::min(chunk_start + chunk_size, end_sample);

        // Prepare buffer for the chunk
        std::vector<float> chunk_data(wave->samples + chunk_start,
                                      wave->samples + chunk_end);

        // Fill with zeros up to 30 seconds if the chunk is smaller than 30
        // seconds
        if (chunk_data.size() < chunk_size) {
          chunk_data.resize(chunk_size, 0.0f);
        }

        // Process the chunk with Whisper
        if (whisper_full(ctx, wparams, chunk_data.data(), chunk_data.size()) !=
            0) {
          std::cerr << argv[0] << ": Failed to process chunk." << std::endl;
          return EXIT_FAILURE;
        }

        // Get and print chunk transcription
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
        diarization::print_segment(segments[i], text.str());
      }
    } else {
      // Process the segment if it's <= 30 seconds
      std::vector<float> segment_data(wave->samples + start_sample,
                                      wave->samples + end_sample);

      // Fill with zeros up to 30 seconds if the segment is smaller than 30
      // seconds
      if (segment_data.size() < chunk_size) {
        segment_data.resize(chunk_size, 0.0f);
      }

      // Process the buffered (padded) segment with Whisper
      if (whisper_full(ctx, wparams, segment_data.data(),
                       segment_data.size()) != 0) {
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
      diarization::print_segment(segments[i], text.str());
    }
  }

  // Write JSON file
  if (!json_path.empty()) {
    utils::save_json(json_path, result_json);
  }

  // Cleanup
  SherpaOnnxOfflineSpeakerDiarizationDestroySegment(segments);
  SherpaOnnxOfflineSpeakerDiarizationDestroyResult(result);
  SherpaOnnxDestroyOfflineSpeakerDiarization(sd);
  SherpaOnnxFreeWave(wave);
  whisper_free(ctx);
  return 0;
}