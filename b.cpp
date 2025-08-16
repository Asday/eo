#include <cstring>
#include <expected>
#include <experimental/array>
#include <experimental/scope>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std::chrono_literals;

int execvp(const char* file, std::vector<char const*> args) {
  return execvp(file, const_cast<char**>(args.data()));
}

std::expected<pid_t, std::string> runAsync(std::vector<char const*> cmd) {
  pid_t pid{fork()};
  if (pid > 0) return pid;
  if (pid == -1) return std::unexpected(std::strerror(errno));

  cmd.push_back(NULL);
  execvp(cmd[0], cmd);

  return std::unexpected(std::strerror(errno));
}

struct Empty{};
using WaitResult = std::expected<int, Empty>;
WaitResult waitFor(pid_t pid) {
  int status;
  waitpid(pid, &status, WUNTRACED);
  if (WIFEXITED(status)) return WEXITSTATUS(status);

  return std::unexpected(Empty{});
}

std::vector<WaitResult> waitForAll(const std::vector<pid_t>& pids) {
  std::vector<WaitResult> results;
  std::vector<std::jthread> workers;
  results.resize(pids.size());
  workers.reserve(pids.size());
  /*
  for (const auto& [r, pid] : std::views::zip(results, pids)) {
    workers.push_back(std::jthread{
      [&r, pid](){ r = waitFor(pid); }
    });
  }
  */

  decltype(results)::size_type i{0};
  for (const auto& pid : pids) {
    workers.push_back(std::jthread{
      [&results, i, pid](){ results[i] = waitFor(pid); }
    });
    i++;
  }

  return results;
}

std::expected<int, std::string> runSync(std::vector<char const*> cmd) {
  auto maybePID{runAsync(cmd)};
  if (!maybePID.has_value()) return std::unexpected(maybePID.error());

  auto maybeExitCode{waitFor(maybePID.value())};
  if (!maybeExitCode.has_value()) {
    return std::unexpected(
      (std::stringstream() << cmd[0] << " was terminated").str()
    );
  }

  return maybeExitCode.value();
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

///////////////////////////////////////////////////////////////////////////////

bool usage() {
  std::clog << "usage: b <command>" << std::endl;

  return true;
}

bool help(std::vector<std::string_view> args) {
  if (args.size() == 0) { usage(); return true; }

  return true;
}

bool clean() {
  /*
  waitForAll(
    std::views::all(std::vector{
      runAsync({"rm", "-f", "cluster", "launcher", "login", "client"}),
      runAsync({"rm", "-rf", "build/artefacts"})
    })
    | std::views::filter([](auto r){ return r.has_value(); })
    | std::views::transform([](auto r){ return r.value(); })
  );
  */

  auto maybePIDs{std::array{
    runAsync({"rm", "-f", "cluster", "launcher", "login", "client"}),
    runAsync({"rm", "-rf", "build/artefacts"})
  }};
  std::vector<pid_t> pids;
  for (const auto& maybePID : maybePIDs) {
    if (maybePID.has_value()) pids.push_back(maybePID.value());
  }
  waitForAll(pids);

  return true;
}

struct Dependencies {
  std::filesystem::path target;
  std::vector<std::filesystem::path> objects;
  std::vector<std::filesystem::path> includeDirs;
  std::vector<std::filesystem::path> libDirs;
  std::vector<std::string> libs;
};

void join(
  std::ostream& os,
  const auto& items,
  const std::string_view& delimeter,
  const std::string_view& empty = ""
) {
  if (!items.size()) { os << empty; return; }
  os << items[0];
  for (const auto& i : std::ranges::drop_view{items, 1}) os << delimeter << i;
}

std::ostream& operator<<(std::ostream& os, const Dependencies& d) {
  os << d.target << '\n';
  os << "\tobjects: "; join(os, d.objects, ", ", "none"); os << '\n';
  os << "\tincludeDirs: "; join(os, d.includeDirs, ", ", "none"); os << '\n';
  os << "\tlibDirs: "; join(os, d.libDirs, ", ", "none"); os << '\n';
  os << "\tlibs: "; join(os, d.libs, ", ", "none");

  return os;
}

void updateDependencies(
  Dependencies& d,
  const std::filesystem::path& source,
  std::string_view include
) {
  std::filesystem::path target{include.substr(10, include.size() - 1 - 10)};
  if (include[9] == '<') {
    // Third party.
    if (target == "libpq-fe.h") {
      d.includeDirs.emplace_back("/usr/include");
      d.libDirs.emplace_back("/usr/lib");
      d.libs.emplace_back("pq");

      return;
    }

    return; // stdlib.
  }

  // Local.
  else if (include[9] == '"') {
    if (target.stem() == source.stem()) return;  // Own header.
    target.replace_extension("o");
    d.objects.emplace_back(target);
  }

  else std::clog << "wtf is " << include << '?' << std::endl;
}

std::expected<std::filesystem::path, Empty>
getSource(std::filesystem::path target) {
  const auto ext{target.extension()};
  if (ext == "") {  // Excutable, find a `main.cpp`.
    for (const auto& de : std::filesystem::recursive_directory_iterator(".")) {
      std::filesystem::path p{de.path().lexically_normal()};
      if (p.filename() == "main.cpp" && p.parent_path() == target) {
        return p;
      }
    }
    return std::unexpected(Empty{});
  }

  if (ext == ".o") { target.replace_extension("cpp"); return target; }

  return std::unexpected(Empty{});
}

Dependencies divineDependencies(std::filesystem::path target) {
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  Dependencies r{.target{target}};
  #pragma GCC diagnostic pop
  std::filesystem::path source;
  {
    const auto maybeSource{getSource(target)};
    if (!maybeSource.has_value()) return r;
    source = std::move(maybeSource).value();
  }
  std::ifstream f{source};
  std::string include;
  while (std::getline(f, include)) {
    if (include == "") continue;  // Spacing only.
    if (!include.starts_with("#include ")) break;  // Source starts.

    updateDependencies(r, source, include);
  }

  return r;
}

bool buildTargets(
  [[maybe_unused]] std::vector<std::string_view> flags,
  [[maybe_unused]] std::vector<std::filesystem::path> targets
) {
  std::vector<Dependencies> deps{};
  std::vector<std::filesystem::path> checked{};
  for (const auto& target : targets) {
    deps.push_back(divineDependencies(target));
  }

  for (decltype(deps)::size_type i{0}; i < deps.size(); i++) {
    const auto d{deps[i]};
    for (const auto& o : d.objects) {
      for (const auto& c : checked) {
        if (c == o) goto skip;
      }
      deps.push_back(divineDependencies(o));
      checked.push_back(o);

      skip:
    }
  }

  // for (const auto& d : deps) std::clog << d << '\n';
  // std::clog << std::flush;
  // TODO:
  //
  // * sort by `objects` dependencies?  (ok but how?  Flattening a tree
  //   is fine but...)
  // * for each `Dependencies`:
  //   * check the age of the target vs the source and objects
  //   * if it's older than any, recompile it
  //   * if it's an executable, relink it
  return true;
}

bool all(std::vector<std::string_view> flags) {
  if (flags.size() != 0) {
    std::clog << "unsupported flags" << std::endl;
    return false;
  }

  // Walk over src to find `main.cpp`s.
  std::vector<std::filesystem::path> targets;
  for (const auto& de : std::filesystem::recursive_directory_iterator(".")) {
    std::filesystem::path path{de.path()};
    if (path.filename() == "main.cpp") {
      targets.push_back(path.parent_path().lexically_normal());
    }
  }

  return buildTargets(flags, targets);
}

bool all() { return all({}); }

bool main_(int argc, char* argv[]) {
  if (latest(argv) == RebuildResult::FAILED) return -1;

  std::filesystem::current_path("src");

  std::vector<std::string_view> args{argv + 1, argv + argc};
  if (args.size() == 0) return all();

  if (args[0] == "usage") return usage();
  if (args[0] == "help") return help({args.begin() + 1, args.end()});
  if (args[0] == "clean") return clean();

  std::vector<std::string_view> flags;
  std::vector<std::filesystem::path> targets;
  for (const auto& a : args) {
    if (a.starts_with("--")) { flags.push_back(a); }
    else { targets.push_back(a); }
  }

  return buildTargets(flags, targets);
}

int main(int argc, char* argv[]) {
  if (main_(argc, argv)) return 0;
  else return -1;
}
