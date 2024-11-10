#pragma once

#include "sherpa-onnx/c-api/c-api.h"
#include "whisper.h"
#include <nlohmann/json.hpp>

namespace segments {

// Function to process all segments and return a JSON result
nlohmann::ordered_json
process_segments(const SherpaOnnxOfflineSpeakerDiarizationSegment *segments,
                 int32_t num_segments, const SherpaOnnxWave *wave,
                 whisper_context *ctx, whisper_full_params params);

} // namespace segments
