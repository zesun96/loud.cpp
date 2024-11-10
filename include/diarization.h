#pragma once
#include "spinner.h"
#include <sherpa-onnx/c-api/c-api.h>
#include <string>

using spinner::Spinner;

namespace diarization {

const SherpaOnnxOfflineSpeakerDiarization *
create_sd(const std::string &segmentation_model_path,
          const std::string &embedding_model_path, int32_t num_clusters,
          std::string provider, int32_t onnx_num_threads);
const SherpaOnnxWave *read_wave(const std::string &path);
int32_t diarization_progress_callback(int32_t num_processed_chunk,
                                      int32_t num_total_chunks,
                                      Spinner *spinner);
void print_segment(const SherpaOnnxOfflineSpeakerDiarizationSegment &segment,
                   const std::string &text);
std::string get_default_provider();
} // namespace diarization