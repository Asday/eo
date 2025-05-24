#pragma once

#include <libpq-fe.h>

#include <expected>
#include <experimental/scope>
#include <string_view>
#include <vector>

namespace db {
  using PGconnUR = std::experimental::unique_resource<
    PGconn*,
    decltype(&PQfinish)
  >;
  std::expected<PGconnUR, std::string_view> connect();

  using PGresultUR = std::experimental::unique_resource<
    PGresult*,
    decltype(&PQclear)
  >;
  std::expected<PGresultUR, std::string_view> exec(
    const PGconnUR&,
    std::string_view sql
  );
  std::expected<PGresultUR, std::string_view> exec(
    const PGconnUR&,
    std::string_view sql,
    std::vector<char*> params
  );
}
