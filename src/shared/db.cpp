#include "db.h"

#include <libpq-fe.h>

#include <string>

std::expected<db::PGconnUR, std::string_view> db::connect() {
  PGconnUR conn{PQconnectdb(""), PQfinish};

  if (PQstatus(conn.get()) != CONNECTION_OK) {
    conn.release();

    return std::unexpected(PQerrorMessage(conn.get()));
  }

  return conn;
}

std::expected<db::PGresultUR, std::string_view> db::exec(
  const db::PGconnUR& c,
  std::string_view sql,
  std::vector<char*> params
) {
  db::PGresultUR res{
    PQexecParams(
      c.get(),
      std::string(sql).c_str(),
      params.size(),
      nullptr,
      params.data(),
      nullptr,
      nullptr,
      1
    ),
    PQclear
  };

  {
    const ExecStatusType s{PQresultStatus(res.get())};
    if (s != PGRES_TUPLES_OK && s != PGRES_COMMAND_OK) {
      res.release();

      return std::unexpected(PQerrorMessage(c.get()));
    }
  }

  return res;
}

std::expected<db::PGresultUR, std::string_view> db::exec(
  const db::PGconnUR& c,
  std::string_view sql
) {
  return db::exec(c, sql, std::vector<char*>{});
}
