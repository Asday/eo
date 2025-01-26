// * [x] connect to database
// * [ ] advertise IP in the database
//   * [x] ask postgres for IP `inet_client_addr()`
//   * [ ] record compute/memory stats
//   * [x] store returned server id
// * [ ] heartbeat to the database
// * [ ] open a socket on private WAN
// * [ ] wait for orders from cluster
// * [ ] launch server instances as requested
// * [ ] destroy server instances as requested

#include <libpq-fe.h>

#include <array>
#include <chrono>
#include <expected>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using namespace std::literals;

static constexpr const auto RETRY_DELAY{1s};

void heartbeat() noexcept {}

void serve() noexcept {}

class DBConnectionError: public std::runtime_error {
  public:
  DBConnectionError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class DB {
  PGconn* c;

  DB() :
    c([]{
      PGconn* c{PQconnectdb("")};
      if (PQstatus(c) != CONNECTION_OK) {
        throw DBConnectionError(PQerrorMessage(c));
      }
      return c;
    }()) {}

  public:
  static std::expected<DB, std::string> tryCreate() noexcept {
    try {
      return DB();
    } catch (const DBConnectionError& exc) {
      return std::unexpected(exc.what());
    }
  }
  static const DB create() noexcept {
    while (true) {
      auto maybeDB{DB::tryCreate()};
      if (maybeDB.has_value()) {
        return std::move(maybeDB).value();
      } else {
        std::cout << maybeDB.error() << std::endl;
        std::this_thread::sleep_for(RETRY_DELAY);
      }
    }
  }
  DB(const DB&) = delete;
  DB(DB&& other) noexcept: c(std::exchange(other.c, nullptr)) {}
  DB& operator=(const DB&) = delete;
  DB& operator=(DB&& other) noexcept {
    std::swap(c, other.c);

    return *this;
  }
  ~DB() { PQfinish(c); }

  const PGconn* getConn() const noexcept { return c; }
};

struct UUID {
  std::array<unsigned char, 16> uuid;

  UUID() {}
  UUID(const char* v) : uuid([v]{
    std::array<unsigned char, 16> uuid{};
    for (size_t i{0}; i < 16; i++) uuid[i] = static_cast<unsigned char>(v[i]);

    return uuid;
  }()) {}

  friend std::ostream& operator<<(std::ostream& os, const UUID& uuid) {
    auto flags = os.flags();
    os << std::hex;
    for (const auto& [i, c] : uuid.uuid | std::views::enumerate) {
      os << std::setw(2) << std::setfill('0') << +c;
      if (i == 3 || i == 5 || i == 7 || i == 9) os << '-';
    }
    os.flags(flags);

    return os;
  }
};

class QueryError: public std::runtime_error {
  public:
  QueryError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class Query {
  PGresult* r;
  Query(const PGconn* conn, const std::string& sql) :
    r([conn, sql]{
      PGresult* r{PQexecParams(
        const_cast<PGconn*>(conn),
        sql.c_str(),
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        1
      )};
      if (PQresultStatus(r) != PGRES_TUPLES_OK) {
        throw QueryError(PQerrorMessage(conn));
      }

      return r;
    }()) {}

  public:
  static std::expected<Query, std::string>
  tryCreate(PGconn* conn, const std::string& sql) noexcept {
    try {
      return Query(conn, sql);
    } catch (const QueryError& exc) {
      return std::unexpected(exc.what());
    }
  }
  Query(const Query&) = delete;
  Query(Query&& other) noexcept: r(std::exchange(other.r, nullptr)) {}
  Query& operator=(const Query&) = delete;
  Query& operator=(Query&& other) noexcept {
    std::swap(r, other.r);

    return *this;
  }
  ~Query() { PQclear(r); }

  void fetchScalar(auto& out) const noexcept { out = PQgetvalue(r, 0, 0); }
};

class LauncherRepository {
  const DB& db;

  public:
  LauncherRepository(const DB& db) noexcept: db(db) {}

  const std::expected<const UUID, std::string> tryRegister() const noexcept {
    const auto maybeQ{Query::tryCreate(
      const_cast<PGconn*>(db.getConn()),
      "INSERT INTO launcher (ip, port, heartbeat) "
      "VALUES (inet_client_addr(), 32232, NOW()::timestamp) "
      "RETURNING id;"
    )};

    if (!maybeQ.has_value()) return std::unexpected(maybeQ.error());

    UUID uuid;
    maybeQ.value().fetchScalar(uuid);

    return uuid;
  }

  const UUID register_() const noexcept {
    while (true) {
      auto maybeUUID{tryRegister()};
      if (maybeUUID.has_value()) {
        return std::move(maybeUUID).value();
      } else {
        std::cout << maybeUUID.error() << std::endl;
        std::this_thread::sleep_for(RETRY_DELAY);
      }
    }
  }
};

static UUID SERVER_ID{};

int main() noexcept {
  const DB db{DB::create()};
  const LauncherRepository repo{db};
  SERVER_ID = repo.register_();

  std::cout << "I am " << SERVER_ID << std::endl;

  std::jthread heartbeat_worker{heartbeat};
  std::jthread serve_worker{serve};
}
