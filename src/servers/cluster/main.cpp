#include "shared/db.h"
#include "shared/lg.h"
#include "shared/repo.h"
#include "shared/signal.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

static constexpr std::string_view USAGE{
  "usage: cluster <clusterID>\n"
  " where `clusterID` is an integer in the range [0-255]"
};
static constexpr std::string_view LG_NAME{"cluster"};

// What the hell does any of this mean?
template <typename... Ts>
struct Visitor: Ts... { using Ts::operator()...; };

int main(int argc, char* argv[]) {
  lg::init();

  if (argc != 2) {
    std::clog << USAGE << std::endl;

    return 0;
  }

  std::string_view rawCID{argv[1]};
  std::uint8_t clusterID;
  if (std::from_chars(
    rawCID.data(),
    rawCID.data() + rawCID.size(),
    clusterID
  ).ec != std::errc{}) {
    std::clog << "fatal: invalid clusterID" << std::endl;

    return -1;
  }

  lg::debug(LG_NAME, "connecting to DB");
  db::PGconnUR conn;
  {
    auto maybeConn{db::connect()};
    if (!maybeConn.has_value()) {
      std::clog
        << "fatal: failed to connect to DB: "
        << maybeConn.error()
        << std::endl
      ;
      return -1;
    }
    conn = std::move(maybeConn).value();
  }

  std::clog << "debug: initialising repo" << std::endl;
  repo::init(conn);

  std::array<sockaddr_in, 3> launchers;
  {
    while (true) {
      auto maybeLaunchers{repo::get3Launchers(conn)};
      if (maybeLaunchers.has_value()) {
        launchers = std::move(maybeLaunchers).value();

        break;
      } else {
        std::clog << "debug: getting 3 launchers" << std::endl;
        if ( /* should die */ std::visit(Visitor{
          [](const std::string_view& err) {
            std::clog
              << "fatal: failed to get 3 launchers: "
              << err
              << std::endl;
            ;

            return true;  // die
          },
          []([[maybe_unused]] const repo::Not3Launchers&) {
            std::clog
              << "fatal: Adam your SQL sucks and you got some non-3"
                " amount of launchers with that SQL that should"
                " definitely only return 0 or 3 rows"
              << std::endl
            ;

            return true;  // die
          },
          []([[maybe_unused]] const repo::NoLaunchers&) {
            std::clog
              << "debug: no launchers found (yet), retrying soon"
              << std::endl
            ;

            return false;  // retry
          }
        }, maybeLaunchers.error())) return -1;
      }

      std::this_thread::sleep_for(5s);
    }
  }

  signal::waitForInterrupt();

  std::cout << "info: caught SIG{INT,TERM}, shutting down" << std::endl;
}
