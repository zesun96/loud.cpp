// spinner.h
#ifndef SPINNER_H
#define SPINNER_H

#include <atomic>
#include <string>
#include <thread>

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

#endif // SPINNER_H
