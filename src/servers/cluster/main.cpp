#include "shared/db.h"
#include "shared/lg.h"
#include "shared/repo.h"
#include "shared/signal.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <sstream>
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
    lg::fatal(LG_NAME, "fatal: invalid clusterID");

    return -1;
  }

  lg::info(LG_NAME, "connecting to DB");
  db::PGconnUR conn;
  {
    auto maybeConn{db::connect()};
    if (!maybeConn.has_value()) {
      lg::fatal(LG_NAME, (std::stringstream()
        << "fatal: failed to connect to DB: "
        << maybeConn.error()
      ).view());

      return -1;
    }
    conn = std::move(maybeConn).value();
  }

  lg::info(LG_NAME, "initialising repo");
  repo::init(conn);

  std::array<sockaddr_in, 3> launchers;
  {
    while (true) {
      lg::info(LG_NAME, "getting 3 launchers");
      auto maybeLaunchers{repo::get3Launchers(conn)};
      if (maybeLaunchers.has_value()) {
        launchers = std::move(maybeLaunchers).value();

        break;
      } else {
        if ( /* should die */ std::visit(Visitor{
          [](const std::string_view& err) {
            lg::fatal(LG_NAME, (std::stringstream()
              << "failed to get 3 launchers: "
              << err
            ).view());

            return true;  // die
          },
          []([[maybe_unused]] const repo::Not3Launchers&) {
            lg::fatal(
              LG_NAME,
              "Adam your SQL sucks and you got some non-3 amount of"
              " launchers with that SQL that should definitely only"
              " return 0 or 3 rows"
            );

            return true;  // die
          },
          []([[maybe_unused]] const repo::NoLaunchers&) {
            lg::info(LG_NAME, "no launchers found (yet), retrying soon");

            return false;  // retry
          }
        }, maybeLaunchers.error())) return -1;
      }

      std::this_thread::sleep_for(5s);
    }
  }

  signal::waitForInterrupt();

  lg::info(LG_NAME, "caught SIG{INT,TERM}, shutting down");
}
