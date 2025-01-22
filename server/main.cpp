#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>

int main() {
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
    uint8_t received = recvfrom(
      s,
      d,
      maxPacketSize,
      0,
      reinterpret_cast<sockaddr*>(&source),
      &sourceLength
    );
    std::cout
      << "received from "
      << ntohl(source.sin_addr.s_addr)
      << " data: ";
    for (auto j = 0; j < received; j++) {
      std::cout << d[j];
    }
    std::cout << std::endl;

    i++;
  }
  close(s);
}
