#pragma once

#include <string>

namespace download {
void download_file(std::string url, std::string path);
void download_models_if_needed();
} // namespace download