#include <libpq-fe.h>

#include <charconv>

namespace repo {
  struct registerResult {
    UUID id;
  }
  // Registers this launcher with the database for service discovery.
  std::expected<registerResult, std::string_view> register_(
    const std::int8_t port,
  ) noexcept {
    static constexpr char* sql{
      "INSERT INTO launcher (ip, port, heartbeat) "
      "VALUES (inet_client_addr(), $1, NOW()::timestamp) "
      "RETURNING id "
    };
    constexpr std::size_t nParams{1};
    const char* paramValues[nParams]{
      port,
    };
    PGresult* r{PQexecParams(
      conn,
      sql,
      nParams,
      nullptr,  // `paramTypes[]` - `nullptr` means infer from context.
      paramValues,
      nullptr,  // `paramLengths[]` - ignored for text format params.
      nullptr,  // `paramFormats[]` - `nullptr` means all text.
      1
    )};
    const ExecStatusType s{PQresultStatus(r)};
    if (s != PGRES_TUPLES_OK){
      return std::unexpected(PQerrorMessage(conn));
    }
  }
  struct heartbeatResult {
  }
  // Pings the database as a service discovery health check.
  std::expected<void, std::string_view> heartbeat(
    const UUID id,
  ) noexcept {
    static constexpr char* sql{
      "UPDATE launcher "
      "SET heartbeat = NOW()::timestamp "
      "WHERE id = $1 "
    };
    constexpr std::size_t nParams{1};
    const char* paramValues[nParams]{
      id,
    };
    PGresult* r{PQexecParams(
      conn,
      sql,
      nParams,
      nullptr,  // `paramTypes[]` - `nullptr` means infer from context.
      paramValues,
      nullptr,  // `paramLengths[]` - ignored for text format params.
      nullptr,  // `paramFormats[]` - `nullptr` means all text.
      1
    )};
    const ExecStatusType s{PQresultStatus(r)};
    if (s != PGRES_COMMAND_OK){
      return std::unexpected(PQerrorMessage(conn));
    }
  }
  struct unregisterResult {
  }
  // Removes this launcher from the service discovery table.
  std::expected<void, std::string_view> unregister(
    const UUID id,
  ) noexcept {
    static constexpr char* sql{
      "DELETE FROM launcher "
      "WHERE id = $1 "
    };
    constexpr std::size_t nParams{1};
    const char* paramValues[nParams]{
      id,
    };
    PGresult* r{PQexecParams(
      conn,
      sql,
      nParams,
      nullptr,  // `paramTypes[]` - `nullptr` means infer from context.
      paramValues,
      nullptr,  // `paramLengths[]` - ignored for text format params.
      nullptr,  // `paramFormats[]` - `nullptr` means all text.
      1
    )};
    const ExecStatusType s{PQresultStatus(r)};
    if (s != PGRES_COMMAND_OK){
      return std::unexpected(PQerrorMessage(conn));
    }
  }
  struct launchersResult {
    UUID id;
    inet ip;
    int4 port;
    timestamp heartbeat;
  }
  std::expected<std::vector<launchersResult>, std::string_view> launchers(
  ) noexcept {
    static constexpr char* sql{
      "SELECT id, ip, port, heartbeat "
      "FROM launcher "
    };
    constexpr std::size_t nParams{0};
    const char* paramValues[]{nullptr};
    PGresult* r{PQexecParams(
      conn,
      sql,
      nParams,
      nullptr,  // `paramTypes[]` - `nullptr` means infer from context.
      paramValues,
      nullptr,  // `paramLengths[]` - ignored for text format params.
      nullptr,  // `paramFormats[]` - `nullptr` means all text.
      1
    )};
    const ExecStatusType s{PQresultStatus(r)};
    if (s != PGRES_TUPLES_OK){
      return std::unexpected(PQerrorMessage(conn));
    }
  }
}
