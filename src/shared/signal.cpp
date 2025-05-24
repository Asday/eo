#include "signal.h"

namespace {
  #include <signal.h>
}

void signal::waitForInterrupt() {
  sigset_t sigint;
  sigemptyset(&sigint);
  sigaddset(&sigint, SIGINT);
  sigaddset(&sigint, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &sigint, nullptr);

  int signum{0};
  sigwait(&sigint, &signum);
}
