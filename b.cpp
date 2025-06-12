#include <experimental/array>
#include <experimental/scope>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

int execvp(const char* file, std::vector<char const*> args) {
  return execvp(file, const_cast<char**>(args.data()));
}

enum class RebuildResult { UNNEEDED, FAILED };

RebuildResult latest(char* argv[]) {
  {
    auto sourceTime{std::filesystem::last_write_time("b.cpp")};
    std::filesystem::file_time_type binTime;

    try { binTime = std::filesystem::last_write_time("b"); }
    catch (const std::exception&) {}

    if (sourceTime <= binTime) return RebuildResult::UNNEEDED;
  }

  std::clog << "rebuilding build system" << std::endl;
  {
    pid_t pid{fork()};
    if (pid == -1) {
      std::clog << "failed to rebuild: " << std::strerror(errno) << std::endl;

      return RebuildResult::FAILED;
    } else if (pid == 0) {
      execvp("g++", {
        "g++",
        "-std=c++23",
        "-Wall", "-Werror", "-Wextra", "-Wsign-conversion", "-pedantic-errors",
        "-o", "b",
        "b.cpp",
        NULL
      });

      std::clog << "failed to rebuild: " << std::strerror(errno) << std::endl;

      return RebuildResult::FAILED;
    } else {
      int status;
      waitpid(pid, &status, WUNTRACED);
      if (WIFEXITED(status) && WEXITSTATUS(status)) {
        std::clog
          << "failed to rebuild: " << +WEXITSTATUS(status) << std::endl
        ;

        return RebuildResult::FAILED;
      }
    }
  }

  std::clog << "re-executing build system" << std::endl;
  execv("./b", argv);

  std::clog << "failed to reexec: " << std::strerror(errno) << std::endl;

  return RebuildResult::FAILED;
}

int main(int, char* argv[]) {
  if (latest(argv) == RebuildResult::FAILED) return -1;

  std::cout << "version 5" << std::endl;
}
