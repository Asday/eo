#include "repo.h"

#include "shared/db.h"
#include "shared/lg.h"

#include <libpq-fe.h>

#include <array>
#include <expected>
#include <netinet/in.h>
#include <sstream>
#include <string_view>
#include <variant>
#include <vector>

static constexpr std::string_view LG_NAME{"repo"};

std::expected<void, std::string_view>
prepareGet3Launchers(const db::PGconnUR& conn) {
  lg::debug(
    (std::stringstream() << LG_NAME << ".prepareGet3Launchers").view(),
    "preparing `get3Launchers`"
  );
  auto maybeRes{db::prepare(
    conn,
    "get3Launchers",
    "SELECT ip, port FROM ( "
    "  SELECT ip, port, heartbeat, 1 AS n FROM launcher "
    "  UNION ALL "
    "  SELECT ip, port, heartbeat, 2 AS n FROM launcher "
    "  UNION ALL "
    "  SELECT ip, port, heartbeat, 3 AS n FROM launcher "
    "  ORDER BY n, heartbeat DESC "
    "  LIMIT 3 "
    "); "
  )};
  if (!maybeRes.has_value()) {
    return std::unexpected((std::stringstream()
      << "failed to prepare `get3Launchers`: "
      << maybeRes.error()
    ).view());
  }

  return {};
}

constexpr sockaddr_in
fromLauncher(PGresult* res, int row, int ipCol, int portCol) {
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  return {
    .sin_family{AF_INET},
    .sin_port{static_cast<in_port_t>(*PQgetvalue(res, row, portCol))},
    .sin_addr{.s_addr{static_cast<in_addr_t>(*PQgetvalue(res, row, ipCol))}}
  };
  #pragma GCC diagnostic pop
}

std::expected<
  std::array<sockaddr_in, 3>,
  std::variant<repo::NoLaunchers, repo::Not3Launchers, std::string_view>
> repo::get3Launchers(const db::PGconnUR& conn) {
  PGresult* res;
  {
    lg::debug(
      (std::stringstream() << LG_NAME << ".get3Launchers").view(),
      "executing `get3Launchers`"
    );
    auto maybeRes{db::execPrepared(conn, "get3Launchers")};
    if (!maybeRes.has_value()) {
      return std::unexpected((std::stringstream()
        << "failed to execute `get3Launchers`: "
        << maybeRes.error()
      ).view());
    }
    res = std::move(maybeRes).value().get();
  }

  int rowCount{PQntuples(res)};
  lg::debug(
    (std::stringstream() << LG_NAME << ".get3Launchers").view(),
    (std::stringstream() << "got " << rowCount << " rows").view()
  );
  switch (rowCount) {
    case 0: return std::unexpected(NoLaunchers{});
    case 3: {
      int ipCol{PQfnumber(res, "ip")};
      int portCol{PQfnumber(res, "port")};

      return std::array<sockaddr_in, 3>{{
        fromLauncher(res, 0, ipCol, portCol),
        fromLauncher(res, 1, ipCol, portCol),
        fromLauncher(res, 2, ipCol, portCol)
      }};
    }

    default: return std::unexpected(Not3Launchers{});
  }
}

std::expected<void, std::vector<std::string_view>>
repo::init(const db::PGconnUR& conn) {
  lg::debug(
    (std::stringstream() << LG_NAME << ".init").view(),
    "initialising repo"
  );
  std::array<decltype(&prepareGet3Launchers), 1> todo = {prepareGet3Launchers};
  std::vector<std::string_view> errors;
  errors.reserve(todo.size());
  for (const auto& fn : todo) {
    auto maybeRes{fn(conn)};
    if (!maybeRes.has_value()) errors.push_back(maybeRes.error());
  }

  if (errors.size()) return std::unexpected(errors);

  return {};
}
