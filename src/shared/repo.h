#pragma once

#include "db.h"

#include <cstdint>
#include <expected>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace repo {
  struct UnexpectedEnumValue {
    std::string table;
    std::string column;
    std::string value;
  };
  std::ostream&
  operator<<(std::ostream& os, const repo::UnexpectedEnumValue& uev);


  struct NoClusters {};
  struct Not1Cluster {};
  enum class ClusterStatus : std::uint8_t {
    offline = 0,
    startup = 1,
    adminOnly = 2,
    online = 3,
    shutdownRequested = 4,
    shutdown = 5,

    LAST = shutdown,
  };
  std::ostream& operator<<(std::ostream& os, const repo::ClusterStatus& cs);
  struct Cluster {
    std::string name;
    ClusterStatus status;
  };
  std::expected<Cluster, std::variant<
    NoClusters,
    Not1Cluster,
    UnexpectedEnumValue,
    std::string_view
  >>
  getCluster(const db::PGconnUR& conn, std::uint8_t clusterID);

  struct NoLaunchers {};
  struct Not3Launchers {};
  std::expected<
    std::array<sockaddr_in, 3>,
    std::variant<NoLaunchers, Not3Launchers, std::string_view>
  > get3Launchers(const db::PGconnUR& conn);

  std::expected<void, std::vector<std::string_view>>
  init(const db::PGconnUR& conn);
}
