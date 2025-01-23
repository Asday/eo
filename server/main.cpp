#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>

int serve() {
  auto s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s <= 0) return -1;
  {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    sockaddr_in a{
      .sin_family{AF_INET},
      .sin_port{htons(6562)},
      .sin_addr{.s_addr{INADDR_ANY}},
    };
    #pragma GCC diagnostic pop
    if (bind(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0) return -2;
  }
  uint8_t d[256]{};
  constexpr auto maxPacketSize{sizeof(d)};
  int8_t i{0};
  while (i < 3) {
    sockaddr_in source{};
    socklen_t sourceLength{sizeof(source)};
    ssize_t received{recvfrom(
      s,
      d,
      maxPacketSize,
      0,
      reinterpret_cast<sockaddr*>(&source),
      &sourceLength
    )};
    {
      const auto a{static_cast<in_addr_t>(ntohl(source.sin_addr.s_addr))};
      std::cout
        << "received " << received << " bytes from "
        << ((a & 0xFF000000) >> 24) << "."
        << ((a & 0xFF0000) >> 16) << "."
        << ((a & 0xFF00) >> 8) << "."
        << (a & 0xFF)
        << " data: ";
    }
    for (auto j{0}; j < received; j++) {
      std::cout << d[j];
    }
    std::cout << std::endl;

    i++;
  }
  close(s);

  return 0;
}

#include <libpq-fe.h>

#include <iostream>
#include <string>

int main() {
  PGconn* c{PQconnectdb("")};
  if (PQstatus(c) != CONNECTION_OK) {
    std::cout << PQerrorMessage(c) << std::endl;
    PQfinish(c);
    return -1;
  }

  {
    PGresult* r{PQexec(
      c,
      "INSERT INTO server_cluster (ip, port, heartbeat) "
      "VALUES ('127.0.0.1', 6562, NOW()::timestamp) "
      "RETURNING id;"
    )};
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
      std::cout << PQerrorMessage(c) << std::endl;
    } else {
      int32_t cols{PQnfields(r)};
      for (uint8_t i{0}; i < cols; i++) {
        std::cout << PQfname(r, i) << std::endl;
      }
      for (uint8_t i{0}; i < PQntuples(r); i++) {
        for (uint8_t j{0}; j < cols; j++) {
          std::cout << PQgetvalue(r, i, j);
        }
        std::cout << std::endl;
      }
    }
    PQclear(r);
  }

  PQfinish(c);

  return 0;
}
