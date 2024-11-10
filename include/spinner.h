// spinner.h
#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace spinner {

class Spinner {
public:
  explicit Spinner(const std::string &initialMessage = "");

  void start();

  void updateMessage(const std::string &newMessage);

  void stop();

  ~Spinner();

private:
  // spin in separate thread

  void spin();

  std::string message;        // Message to display next to spinner
  std::atomic<bool> spinning; // Atomic flag for thread control
  std::thread spinnerThread;  // Thread for spinner animation
};

} // namespace spinner