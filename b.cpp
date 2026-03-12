#include <algorithm>
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

std::ostream& operator<<(
  std::ostream& os,
  const std::filesystem::file_time_type& t
) {
  const auto s{
    std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(t))
  };
  os << std::put_time(std::localtime(&s), "%c");

  return os;
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
    runAsync({"rm", "-rf", "../.build"})
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

std::filesystem::path getBinPath(const std::filesystem::path& target) {
  return std::filesystem::path{} / "../.build/o/" / target;
}

std::filesystem::path getExePath(const std::filesystem::path& target) {
  return std::filesystem::path{} / "../.build/bin/" / target.stem();
}

std::filesystem::path getHeaderPath(std::filesystem::path target) {
  return target.replace_extension("h");
}

bool mkdirs(const std::filesystem::path& dest) {
  const auto maybeExitCode{runSync({"mkdir", "-p", dest.parent_path().c_str()})};
  if (maybeExitCode.has_value()) {
    if (maybeExitCode.value() == 0) return true;
  }

  std::clog << "failed to make output dir for `" << dest << "`: ";

  if (!maybeExitCode.has_value()) std::clog << maybeExitCode.error();
  else std::clog << "exit code: " << maybeExitCode.value();

  std::clog << std::endl;

  return false;
}

bool compile(
  [[maybe_unused]] std::vector<std::string_view> flags,
  const std::filesystem::path& target,
  const std::filesystem::path& dest
) {
  std::clog << "compiling " << target << std::endl;

  if (!mkdirs(dest)) return false;

  auto src{getSource(target)};
  if (!src) {
    std::clog << "failed to compile: cannot find source" << std::endl;
    return false;
  }

  const auto& r{runSync({
    "g++",
    "-std=c++23",
    "-O3",
    "-c",
    "-g",
    "-Wall", "-Werror", "-Wextra", "-Wsign-conversion", "-pedantic-errors",
    "-I.",
    src.value().string().c_str(),
    "-o", dest.string().c_str()
  })};
  if (!r) {
    std::clog << "failed to compile: " << r.error() << std::endl;
    return false;
  }

  return true;
}

bool link_(
  [[maybe_unused]] std::vector<std::string_view> flags,
  [[maybe_unused]] const Dependencies& d,
  const std::filesystem::path& dest
) {
  std::clog << "linking " << d << std::endl;

  if (!mkdirs(dest)) return false;

  const std::string destString{dest.string()};
  const std::string targetString{getBinPath(d.target).string()};
  std::vector<char const*> cmd{{ "g++", "-g", "-o", destString.c_str(), targetString.c_str() }};
  std::vector<std::string> anchors{};
  for (const auto& o : d.objects) { anchors.push_back(getBinPath(o.string())); }
  for (const auto& iDir : d.includeDirs) {
    anchors.push_back((std::stringstream() << "-I" << iDir).str().c_str());
  }
  for (const auto& lDir : d.libDirs) {
    anchors.push_back((std::stringstream() << "-L" << lDir).str().c_str());
  }
  for (const auto& l : d.libs) {
    anchors.push_back((std::stringstream() << "-l" << l).str().c_str());
  }
  for (const auto& a : anchors) { cmd.push_back(a.c_str()); }
  const auto& r{runSync(cmd)};
  if (!r) {
    std::clog << "failed to link: " << r.error() << std::endl;
    return false;
  }

  return true;
}

bool buildTargets(
  std::vector<std::string_view> flags,
  std::vector<std::filesystem::path> targets
) {
  std::vector<Dependencies> deps{};
  std::vector<std::filesystem::path> seen{};
  for (const auto& target : targets) {
    deps.push_back(divineDependencies(target));
  }

  for (decltype(deps)::size_type i{0}; i < deps.size(); i++) {
    const auto d{deps[i]};
    for (const auto& o : d.objects) {
      for (const auto& s : seen) {
        if (s == o) goto skip;
      }
      deps.push_back(divineDependencies(o));
      seen.push_back(o);

      skip:
    }
  }

  // Sort by `.objects` dependencies.
  std::vector<Dependencies> sortedDeps{};
  seen.clear();
  auto lastSize{sortedDeps.size()};
  while (sortedDeps.size() != deps.size()) {
    for (const auto& d : deps) {
      for (const auto& sD : sortedDeps) {
        if (d.target == sD.target) goto alreadySorted;
      }
      for (const auto& o : d.objects) {
        for (const auto& s : seen) {
          if (s == o) goto accountedFor;
        }

        goto cannotBuildYet;

        accountedFor:  // This object is already in `sortedDeps`.
      }

      // If we're here, every object is already in `sortedDeps` so this
      // `Dependencies{}` will be buildable at this point.
      sortedDeps.push_back(d);
      seen.push_back(d.target);

      cannotBuildYet:  // Need more prerequisites.
      alreadySorted:
    }

    if (sortedDeps.size() == lastSize) {
      std::clog << "include cycle detected, fix it dingus" << std::endl;
      return false;
    }
  }

  // Propagate libs and dirs from child dependencies.
  for (auto& d : sortedDeps) {
    for (const auto& o : d.objects) {
      for (const auto& otherDep : sortedDeps) {
        if (otherDep.target == o) {
          for (const auto& otherIDir : otherDep.includeDirs) {
            for (const auto& iDir : d.includeDirs) {
              if (otherIDir == iDir) goto includeDirPropagated;
            }
            d.includeDirs.push_back(otherIDir);
            includeDirPropagated:
          }

          for (const auto& otherLDir : otherDep.libDirs) {
            for (const auto& lDir : d.libDirs) {
              if (otherLDir == lDir) goto libDirPropagated;
            }
            d.libDirs.push_back(otherLDir);
            libDirPropagated:
          }

          for (const auto& otherL : otherDep.libs) {
            for (const auto& l : d.libs) {
              if (otherL == l) goto libPropagated;
            }
            d.libs.push_back(otherL);
            libPropagated:
          }

          break;
        }
      }
    }
  }

  for (const auto& d : sortedDeps) {
    bool exe{d.target.extension() == ""};
    std::filesystem::path source;
    {
      const auto maybeSource{getSource(d.target)};
      if (!maybeSource.has_value()) {
        std::clog << "couldn't find source for " << d.target << std::endl;
        return false;
      }
      source = std::move(maybeSource).value();
    }

    // If the source or any of the depended upon headers are newer than
    // the binary, need to recompile.
    auto newestDependencyTime{std::filesystem::last_write_time(source)};
    std::filesystem::file_time_type binTime;
    const std::filesystem::path binPath{getBinPath(d.target)};
    try { binTime = std::filesystem::last_write_time(binPath); }
    catch (const std::exception&) {
      binTime = std::filesystem::file_time_type::min();
    }

    for (const auto& o : d.objects) {
      newestDependencyTime = std::max(
        newestDependencyTime,
        std::filesystem::last_write_time(getHeaderPath(o))
      );
    }

    if (newestDependencyTime <= binTime) goto alreadyCompiled;

    if (!compile(flags, d.target, binPath)) return false;
    binTime = std::filesystem::last_write_time(binPath);

    alreadyCompiled:
    if (exe) {
      std::filesystem::file_time_type exeTime;
      const std::filesystem::path exePath{getExePath(d.target)};
      try { exeTime = std::filesystem::last_write_time(exePath); }
      catch (const std::exception&) {
        exeTime = std::filesystem::file_time_type::min();
      }
      if (exeTime <= binTime) goto needsLinking;

      for (const auto& o : d.objects) {
        std::filesystem::file_time_type objectTime;
        try { objectTime = std::filesystem::last_write_time(getBinPath(o)); }
        catch ( const std::exception&) {}

        if (exeTime <= objectTime) goto needsLinking;
      }

      continue;

      needsLinking:
      if (!link_(flags, d, exePath)) return false;
    }
  }

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
  if (latest(argv) == RebuildResult::FAILED) return false;

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
