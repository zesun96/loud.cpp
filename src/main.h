#include <whisper.h>

#ifndef CHECK_NULL
#define CHECK_NULL(ptr)                                                        \
  if (!ptr) {                                                                  \
    std::cerr << "Error: Null pointer encountered: " << #ptr << std::endl;     \
    return EXIT_FAILURE;                                                       \
  }
#endif

static void cb_log_disable(enum ggml_log_level, const char *, void *) {}