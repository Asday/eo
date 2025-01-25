// * [ ] connect to database
// * [ ] advertise IP in the database
//   * [ ] ask postgres for IP `inet_client_addr()`
// * [ ] open a socket on private WAN
// * [ ] wait for orders from cluster
// * [ ] launch server instances as requested
// * [ ] destroy server instances as requested

#include <libpq-fe.h>

#include <chrono>
#include <expected>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using namespace std::literals;

void heartbeat() noexcept {}

void serve() noexcept {}

class DBConnectionIssue: public std::runtime_error {
  public:
  DBConnectionIssue(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class DB {
  PGconn* c;

  DB() :
    c([]{
      PGconn* c{PQconnectdb("")};
      if (PQstatus(c) != CONNECTION_OK) {
        throw DBConnectionIssue(PQerrorMessage(c));
      }
      return c;
    }()) {}

  public:
  static std::expected<DB, std::string> tryCreate() noexcept {
    try {
      return DB();
    } catch (const DBConnectionIssue& exc) {
      return std::unexpected(exc.what());
    }
  }
  static const DB create() noexcept {
    while (true) {
      auto db{DB::tryCreate()};
      if (db.has_value()) {
        return std::move(db).value();
      } else {
        std::cout << db.error() << std::endl;
        std::this_thread::sleep_for(1s);
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

  void use() const noexcept {}
};

int main() noexcept {
  const auto db{DB::create()};
  db.use();

  std::jthread heartbeat_worker{heartbeat};
  std::jthread serve_worker{serve};
}
