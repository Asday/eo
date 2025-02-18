#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>

#include <errno.h>

int main() {
  auto s{socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)};

  if (s <= 0) return -1;

  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  sockaddr_in dest{
    .sin_family{AF_INET},
    .sin_port{htons(6562)},
    .sin_addr{.s_addr{htonl((127 << 24) | (0 << 16) | (0 << 8) | 1)}},
  };
  #pragma GCC diagnostic pop
  uint8_t d[256]{'m', 'e', 's', 's', 'a', 'g', 'e', ' ', '1'};
  constexpr auto packetSize{sizeof(d)};
  const auto sentBytes{sendto(
    s,
    d,
    packetSize,
    0,
    reinterpret_cast<sockaddr*>(&dest),
    sizeof(dest)
  )};

  if (sentBytes != packetSize) return errno;

  close(s);

  return 0;
}
