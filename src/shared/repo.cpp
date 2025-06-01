#include "repo.h"

#include "shared/db.h"
#include "shared/lg.h"

#include <libpq-fe.h>

#include <array>
#include <expected>
#include <netinet/in.h>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#define _LG_NS "repo"

std::expected<void, std::string_view>
prepareGetCluster(const db::PGconnUR& conn) {
  static constexpr std::string_view LG_NAME{_LG_NS ".prepareGetCluster"};
  lg::debug(LG_NAME, "preparing `getCluster`");
  auto maybeRes{db::prepare(
    conn,
    "getCluster",
    "SELECT name, status FROM cluster WHERE clusterID = $1::int"
  )};
  if (!maybeRes.has_value()) {
    return std::unexpected((std::stringstream()
      << "failed to prepare `getCluster`: "
      << maybeRes.error()
    ).view());
  }

  return {};
}

std::expected<void, std::string_view>
prepareGet3Launchers(const db::PGconnUR& conn) {
  static constexpr std::string_view LG_NAME{_LG_NS ".prepareGet3Launchers"};
  lg::debug(LG_NAME, "preparing `get3Launchers`");
  auto maybeRes{db::prepare(
    conn,
    "get3Launchers",
    "SELECT ip, port FROM (\n"
    "  SELECT ip, port, heartbeat, 1 AS n FROM launcher\n"
    "  UNION ALL\n"
    "  SELECT ip, port, heartbeat, 2 AS n FROM launcher\n"
    "  UNION ALL\n"
    "  SELECT ip, port, heartbeat, 3 AS n FROM launcher\n"
    "  ORDER BY n, heartbeat DESC\n"
    "  LIMIT 3\n"
    ");"
  )};
  if (!maybeRes.has_value()) {
    return std::unexpected((std::stringstream()
      << "failed to prepare `get3Launchers`: "
      << maybeRes.error()
    ).view());
  }

  return {};
}

static constexpr std::array<
  std::string_view,
  std::to_underlying(repo::ClusterStatus::LAST) + 1
>
clusterStatusTitles {{
  "offline",
  "startup",
  "adminOnly",
  "online",
  "shutdownRequested",
  "shutdown"
}};

struct InvalidTitle {};
std::expected<repo::ClusterStatus, InvalidTitle>
fromTitle(const std::string_view title) {
  for (const auto& [i, t] : clusterStatusTitles | std::views::enumerate) {
    if (t == title) return repo::ClusterStatus{static_cast<std::uint8_t>(i)};
  }

  return std::unexpected(InvalidTitle{});
}

constexpr std::string_view title(repo::ClusterStatus cs) {
  if (std::to_underlying(cs) > clusterStatusTitles.size()) return "UNKNOWN";
  return clusterStatusTitles[std::to_underlying(cs)];
}

std::ostream&
repo::operator<<(std::ostream& os, const repo::ClusterStatus& cs) {
  os << title(cs);

  return os;
}

std::ostream&
repo::operator<<(std::ostream& os, const repo::UnexpectedEnumValue& uev) {
  os << '`' << uev.table << "." << uev.column << "`: `" << uev.value << '`';

  return os;
}

std::expected<repo::Cluster, std::variant<
  repo::NoClusters,
  repo::Not1Cluster,
  repo::UnexpectedEnumValue,
  std::string_view
>>
repo::getCluster(const db::PGconnUR& conn, std::uint8_t clusterID) {
  static constexpr std::string_view LG_NAME{_LG_NS ".getCluster"};
  db::PGresultUR res;
  {
    auto maybeRes{db::execPrepared(
      conn,
      "getCluster",
      std::vector<char*>{(std::stringstream() << +clusterID).str().data()}
    )};
    if (!maybeRes.has_value()) {
      return std::unexpected((std::stringstream()
        << "failed to execute `getCluster`: "
        << maybeRes.error()
      ).view());
    }
    res = std::move(maybeRes).value();
  }

  int rowCount{PQntuples(res.get())};
  lg::debug(LG_NAME, (std::stringstream()
    << "got " << rowCount << " rows"
  ).view());
  switch(rowCount) {
    case 0: return std::unexpected(NoClusters{});
    case 1: {
      int nameCol{PQfnumber(res.get(), "name")};
      int statusCol{PQfnumber(res.get(), "status")};

      ClusterStatus status;
      {
        auto value{static_cast<char*>(PQgetvalue(res.get(), 0, statusCol))};
        auto maybeStatus{fromTitle(value)};
        if (!maybeStatus.has_value()) {
          return std::unexpected(repo::UnexpectedEnumValue{
            .table{"cluster"},
            .column{"status"},
            .value{value}
          });
        }
        status = std::move(maybeStatus).value();
      }

      return Cluster{
        .name{static_cast<char*>(PQgetvalue(res.get(), 0, nameCol))},
        .status{status}
      };
    }

    default: return std::unexpected(Not1Cluster{});
  }
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
  db::PGresultUR res;
  static constexpr std::string_view LG_NAME{_LG_NS ".get3Launchers"};
  {
    lg::debug(LG_NAME, "executing `get3Launchers`");
    auto maybeRes{db::execPrepared(conn, "get3Launchers")};
    if (!maybeRes.has_value()) {
      return std::unexpected((std::stringstream()
        << "failed to execute `get3Launchers`: "
        << maybeRes.error()
      ).view());
    }
    res = std::move(maybeRes).value();
  }

  int rowCount{PQntuples(res.get())};
  lg::debug(LG_NAME, (std::stringstream()
    << "got " << rowCount << " rows"
  ).view());
  switch (rowCount) {
    case 0: return std::unexpected(NoLaunchers{});
    case 3: {
      int ipCol{PQfnumber(res.get(), "ip")};
      int portCol{PQfnumber(res.get(), "port")};

      return std::array<sockaddr_in, 3>{{
        fromLauncher(res.get(), 0, ipCol, portCol),
        fromLauncher(res.get(), 1, ipCol, portCol),
        fromLauncher(res.get(), 2, ipCol, portCol)
      }};
    }

    default: return std::unexpected(Not3Launchers{});
  }
}

std::expected<void, std::vector<std::string_view>>
repo::init(const db::PGconnUR& conn) {
  static constexpr std::string_view LG_NAME{_LG_NS ".init"};
  lg::debug(LG_NAME, "initialising repo");

  std::array<decltype(&prepareGet3Launchers), 2> todo = {
    prepareGetCluster,
    prepareGet3Launchers
  };
  std::vector<std::string_view> errors;
  errors.reserve(todo.size());
  for (const auto& fn : todo) {
    auto maybeRes{fn(conn)};
    if (!maybeRes.has_value()) errors.push_back(maybeRes.error());
  }

  if (errors.size()) return std::unexpected(errors);

  return {};
}

#undef _LG_NS
