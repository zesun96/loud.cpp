
#include "whisper.h"
#include <iostream>

int main() {
  struct whisper_context_params cparams = whisper_context_default_params();
  std::cout << "Hello from loud.cpp!" << std::endl;

  return 0;
}