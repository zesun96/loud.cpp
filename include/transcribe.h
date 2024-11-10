#pragma once

#include <string>
#include <whisper.h>

namespace transcribe {
whisper_full_params create_whisper_params(std::string language);
}