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
#include <cerrno>
#include <chrono>
#include <cstring>
#include <expected>
#include <iostream>
#include <netinet/in.h>
#include <ranges>
#include <signal.h>
#include <stdexcept>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <variant>

using namespace std::literals;
using notManyMilliseconds = std::chrono::duration<int, std::milli>;

static constexpr const auto RETRY_DELAY{1s};
static constexpr const uint16_t PORT{32232};


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

static UUID SERVER_ID{};

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

class QueryError: public std::runtime_error {
  public:
  QueryError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class Query {
  PGresult* r;
  Query(const PGconn* conn, const std::string& sql) :
    r([&conn, &sql]{
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
      {
        const ExecStatusType s{PQresultStatus(r)};
        if (s != PGRES_TUPLES_OK && s != PGRES_COMMAND_OK) {
          throw QueryError(PQerrorMessage(conn));
        }
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
  // No `.create()` because it's expected that trying the same query a
  // second time will have similar results.
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
      (std::stringstream()
        << "INSERT INTO launcher (ip, port, heartbeat) "
        << "VALUES (inet_client_addr(), " << PORT << ", NOW()::timestamp) "
        << "RETURNING id;"
      ).str().c_str()
    )};

    if (!maybeQ.has_value()) return std::unexpected(maybeQ.error());

    UUID uuid;
    maybeQ.value().fetchScalar(uuid);

    return uuid;
  }

  const UUID register_() const noexcept {
    while (true) {
      auto maybeUUID{tryRegister()};
      if (maybeUUID.has_value()) return std::move(maybeUUID).value();

      std::cout << maybeUUID.error() << std::endl;
      std::this_thread::sleep_for(RETRY_DELAY);
    }
  }

  const std::expected<void, std::string> tryHeartbeat() const noexcept {
    const auto maybeQ{Query::tryCreate(
      const_cast<PGconn*>(db.getConn()),
      (std::stringstream()
        << "UPDATE launcher "
        << "SET heartbeat = NOW()::timestamp "
        << "WHERE id = '" << SERVER_ID << "';"
      ).str().c_str()
    )};

    if (!maybeQ.has_value()) return std::unexpected(maybeQ.error());

    return {};
  }

  const std::expected<void, std::string> tryUnregister() const noexcept {
    const auto maybeQ{Query::tryCreate(
      const_cast<PGconn*>(db.getConn()),
      (std::stringstream()
        << "DELETE FROM launcher "
        << "WHERE id = '" << SERVER_ID << "';"
      ).str().c_str()
    )};

    if (!maybeQ.has_value()) return std::unexpected(maybeQ.error());

    return {};
  }

  void unregister() const noexcept {
    while (true) {
      auto maybeVoid{tryUnregister()};
      if (maybeVoid.has_value()) return;

      std::cout << maybeVoid.error() << std::endl;
      std::this_thread::sleep_for(RETRY_DELAY);
    }
  }
};

class SocketCreateError: public std::runtime_error {
  public:
  SocketCreateError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class Socket {
  int fd;
  Socket() : fd([]{
    int fd{socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)};
    if (fd == -1) throw SocketCreateError(std::strerror(errno));

    return fd;
  }()) {}

  public:
  static std::expected<Socket, SocketCreateError> tryCreate() noexcept {
    try { return Socket{}; }
    catch (const SocketCreateError& exc) { return std::unexpected(exc); }
  }
  // No `.create()` because it's not expected that retrying socket
  // creation will have a different outcome.
  Socket(const Socket&) = delete;
  Socket(Socket&& other) noexcept: fd(std::exchange(other.fd, -1)) {}
  Socket& operator=(const Socket&) = delete;
  Socket& operator=(Socket&& other) noexcept {
    std::swap(fd, other.fd);

    return *this;
  }
  ~Socket() { close(fd); }

  int getFileDescriptor() const noexcept { return fd; }
};

class EPollCreateError: public std::runtime_error {
  public:
  EPollCreateError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class EPoll {
  int fd_;  // Named funny to give `.tryWatch()` a nicer signature.
  EPoll() : fd_([]{
    int fd_ = epoll_create1(0);
    if (fd_ == -1) throw EPollCreateError(std::strerror(errno));

    return fd_;
  }()) {}

  public:
  static std::expected<EPoll, EPollCreateError> tryCreate() noexcept {
    try { return EPoll{}; }
    catch (const EPollCreateError& exc) { return std::unexpected(exc); }
  }
  EPoll(const EPoll&) = delete;
  EPoll(EPoll&& other) noexcept: fd_(std::exchange(other.fd_, -1)) {}
  EPoll& operator=(const EPoll&) = delete;
  EPoll& operator=(EPoll&& other) noexcept {
    std::swap(fd_, other.fd_);

    return *this;
  }
  ~EPoll() { close(fd_); }

  std::expected<void, std::string> tryWatch(int fd) const noexcept {
    epoll_event ev{.events{EPOLLIN}, .data{.fd{fd}}};
    if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
      return std::unexpected(std::strerror(errno));
    }

    return {};
  }

  bool wait(const std::optional<notManyMilliseconds>& timeout) const noexcept {
    const int t{[&timeout]{
      if (!timeout.has_value()) return -1;
      return timeout.value().count();
    }()};

    epoll_event e;

    return epoll_wait(fd_, &e, 1, t) > 0;
  }
};

class SocketBindError: public std::runtime_error {
  public:
  SocketBindError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};


class EPollSetupError: public std::runtime_error {
  public:
  EPollSetupError(const std::string& error) noexcept:
    std::runtime_error(error) {}
};

class BoundSocket {
  Socket s;
  EPoll ep;
  BoundSocket(uint32_t addr, uint16_t port) :
    s([]{
      auto maybeS{Socket::tryCreate()};
      if (maybeS.has_value()) return std::move(maybeS).value();
      throw maybeS.error();
    }()),
    ep([]{
      auto maybeEP{EPoll::tryCreate()};
      if (maybeEP.has_value()) return std::move(maybeEP).value();
      throw maybeEP.error();
    }()) {
    // Bind the socket.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    sockaddr_in a{
      .sin_family{AF_INET},
      .sin_port{htons(port)},
      .sin_addr{.s_addr{addr}},
    };
    #pragma GCC diagnostic pop
    if (
      bind(
        s.getFileDescriptor(),
        reinterpret_cast<sockaddr*>(&a),
        sizeof(a)
      ) == -1
    ) {
      throw SocketBindError(std::strerror(errno));
    }
    // Add the socket to epoll.
    if (const auto& w{ep.tryWatch(s.getFileDescriptor())}; !w.has_value()) {
      throw EPollSetupError(w.error());
    }
  }

  public:
  static std::expected<
    BoundSocket,
    std::variant<SocketCreateError, SocketBindError>
  >
  tryCreate(uint32_t addr, uint16_t port) noexcept {
    try { return BoundSocket{addr, port}; }
    catch (const SocketCreateError& exc) { return std::unexpected(exc); }
    catch (const SocketBindError& exc) { return std::unexpected(exc); }
  }

  std::expected<bool, std::string> receive(
    std::array<unsigned char, 65536>& buf,
    ssize_t& receivedBytes,
    uint32_t& from,
    const std::optional<notManyMilliseconds>& timeout
  ) const noexcept {
    if (!ep.wait(timeout)) return false;

    sockaddr_in a{};
    constexpr unsigned int aL{sizeof(a)};

    receivedBytes = recvfrom(
      s.getFileDescriptor(),
      buf.data(),
      buf.size(),
      0,
      reinterpret_cast<sockaddr*>(&a),
      const_cast<unsigned int*>(&aL)
    );

    from = ntohl(a.sin_addr.s_addr);

    if (receivedBytes == -1) {
      return std::unexpected(std::strerror(errno));
    }

    return true;
  }
};

void heartbeat(std::stop_token st, const LauncherRepository& repo) noexcept {
  while (!st.stop_requested()) {
    const auto& maybeVoid{repo.tryHeartbeat()};
    if (!maybeVoid.has_value()) {
      std::cout
        << maybeVoid.has_value()
        << "error: heart skipped a beat: "
        << maybeVoid.error()
        << std::endl;
    };

    std::this_thread::sleep_for(1s);
  }
}

void listen_(
  std::stop_token st,
  const BoundSocket& bs
) noexcept {
  std::array<unsigned char, 65536> buf{};
  ssize_t receivedBytes{};
  uint32_t from{};
  std::expected<bool, std::string> receiveResult;

  std::cout << "  Listening..." << std::endl;
  while (!st.stop_requested()) {
    receiveResult = bs.receive(buf, receivedBytes, from, 10ms);
    if (!receiveResult.has_value()) continue;
    if (!receiveResult.value()) continue;  // Timed out.

    std::cout
      << "received "
      << receivedBytes
      << " bytes from "
      << ((from & 0xFF000000) >> 24) << "."
      << ((from & 0xFF0000) >> 16) << "."
      << ((from & 0xFF00) >> 8) << "."
      << (from & 0xFF)
      << " data: ";

    for (uint16_t i{0}; i < receivedBytes; i++) std::cout << buf[i];

    std::cout << std::endl;
  }
}

void run(const BoundSocket& bs, const LauncherRepository& repo) noexcept {
  std::cout << "Starting server as ID " << SERVER_ID << std::endl;

  std::cout << "Starting heart...";
  std::jthread heartbeat_worker{heartbeat, std::ref(repo)};
  std::cout << "  Heart is beating." << std::endl;

  std::cout << "Starting listener...";
  std::jthread listen_worker{listen_, std::ref(bs)};

  sigset_t sigint;
  sigemptyset(&sigint);
  sigaddset(&sigint, SIGINT);
  sigaddset(&sigint, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &sigint, nullptr);

  {
    int signum{0};
    sigwait(&sigint, &signum);
  }

  std::cout << "Caught SIG{INT,TERM}, shutting down..." << std::endl;
}

int main() {
  const BoundSocket bs{[]{
    // TODO: private WAN address here?  Honestly might want to ask the
    // DB for it...
    auto maybeBS{BoundSocket::tryCreate(INADDR_ANY, PORT)};
    if (!maybeBS.has_value()) {
      std::cout
        << "fatal: couldn't create socket: "
        << std::visit([](auto&& arg){ return arg.what(); }, maybeBS.error())
        << std::endl;
      std::terminate();
    }

    return std::move(maybeBS).value();
  }()};

  const DB db{DB::create()};
  const LauncherRepository repo{db};
  SERVER_ID = repo.register_();

  run(bs, repo);

  repo.unregister();
}
