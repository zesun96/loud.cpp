
#include "diarization.h"
#include "ffmpeg.h"
#include "sherpa-onnx/c-api/c-api.h"
#include "spinner.h"
#include "utils.h"
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdio.h>
#include <string>
#include <subprocess.hpp>
#include <termcolor/termcolor.hpp>
#include <vector>
#include <whisper.h>

using spinner::Spinner;

namespace diarization {

// Predefined set of colors to cycle through (represented as termcolor
// constants)
static const std::vector<std::ostream &(*)(std::ostream &)> colors = {
    termcolor::green,   termcolor::yellow,   termcolor::blue,
    termcolor::magenta, termcolor::cyan,     termcolor::bright_yellow,
    termcolor::white,   termcolor::grey,     termcolor::bright_blue,
    termcolor::bold,    termcolor::underline};

// Map to store speaker ID -> color mapping
std::map<int, std::ostream &(*)(std::ostream &)> speaker_colors;

int32_t diarization_progress_callback(int32_t num_processed_chunk,
                                      int32_t num_total_chunks,
                                      Spinner *spinner) {

  CHECK_NULL(spinner);
  float progress =
      (static_cast<float>(num_processed_chunk) / num_total_chunks) * 100.0f;

  spinner->updateMessage("Diarization... " +
                         std::to_string(static_cast<int>(progress)) + "%");

  // Stop the spinner if progress is complete
  if (progress >= 100.0f) {
    spinner->stop();
    std::cout << termcolor::green << "✓" << termcolor::reset
              << " Diarization complete!" << std::endl;
  }

  return 0;
}

std::string format_timestamp(double seconds) {
  int total_seconds = static_cast<int>(seconds);
  int hours = total_seconds / 3600;
  int minutes = (total_seconds % 3600) / 60;
  int secs = total_seconds % 60;

  std::ostringstream oss;
  if (hours > 0) {
    oss << std::setw(2) << std::setfill('0') << hours << ":";
  }
  oss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
      << std::setfill('0') << secs;
  return oss.str();
}

void print_segment(const SherpaOnnxOfflineSpeakerDiarizationSegment &segment,
                   const std::string &text) {
  // Check if speaker has been assigned a color; if not, assign a color
  if (speaker_colors.find(segment.speaker) == speaker_colors.end()) {
    // Cycle through the colors based on the speaker ID
    speaker_colors[segment.speaker] = colors[segment.speaker % colors.size()];
  }

  // Get the color for the current speaker
  auto color = speaker_colors[segment.speaker];

  // Print the segment with the appropriate color
  std::cout << std::fixed << std::setprecision(0)
            << format_timestamp(segment.start) << " -- "
            << format_timestamp(segment.end) << " (" << std::setw(2)
            << std::setfill('0') << segment.speaker << "): ";

  // Apply the color to the text and print
  std::cout << color << text << termcolor::reset << std::endl << std::flush;
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
          std::string provider, int32_t onnx_num_threads) {
  SherpaOnnxOfflineSpeakerDiarizationConfig config;
  memset(&config, 0, sizeof(config));
  config.segmentation.provider = provider.c_str();
  config.embedding.provider = provider.c_str();
  config.embedding.num_threads = onnx_num_threads;
  config.segmentation.num_threads = onnx_num_threads;
  config.segmentation.pyannote.model = segmentation_model_path.c_str();
  config.embedding.model = embedding_model_path.c_str();
  config.clustering.num_clusters = num_clusters;
  auto *sd = SherpaOnnxCreateOfflineSpeakerDiarization(&config);
  if (!sd) {
    std::cerr << "Failed to initialize offline speaker diarization"
              << std::endl;
    return nullptr;
  }
  return sd;
}
const SherpaOnnxWave *read_wave(const std::string &path) {
  const SherpaOnnxWave *wave = SherpaOnnxReadWave(path.c_str());
  if (!wave) {

    return nullptr;
  }
  return wave;
}

const SherpaOnnxWave *prepare_audio_file(const std::string &audio_file,
                                         int argc, char *argv[]) {
  auto wave = diarization::read_wave(audio_file);
  if (wave == nullptr) {
    if (utils::is_program_installed("ffmpeg")) {
      auto random_path = utils::get_random_path(".wav");
      std::cout << "normalize audio..." << std::endl;
      ffmpeg::normalize_audio(audio_file, random_path);
      wave = diarization::read_wave(random_path);
    } else {
      ffmpeg::show_ffmpeg_normalize_suggestion(audio_file, argc, argv);
      return nullptr;
    }
  }

  if (wave->sample_rate != 16000) {
    std::cerr
        << "Error: The audio file must have a sample rate of 16,000 Hz. Found "
        << wave->sample_rate << " Hz." << std::endl;
    ffmpeg::show_ffmpeg_normalize_suggestion(audio_file, argc, argv);
    return nullptr;
  }

  return wave;
}

const SherpaOnnxOfflineSpeakerDiarizationResult *
run_diarization(const SherpaOnnxOfflineSpeakerDiarization *sd,
                const SherpaOnnxWave *wave, Spinner &spinner) {
  return SherpaOnnxOfflineSpeakerDiarizationProcessWithCallback(
      sd, wave->samples, wave->num_samples,
      [](int32_t num_processed_chunk, int32_t num_total_chunks,
         void *arg) -> int32_t {
        return diarization::diarization_progress_callback(
            num_processed_chunk, num_total_chunks, static_cast<Spinner *>(arg));
      },
      &spinner);
}

} // namespace diarization