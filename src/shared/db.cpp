#include "db.h"

#include "shared/lg.h"

#include <libpq-fe.h>

#include <sstream>
#include <string>
#include <string_view>

#define _LG_NAME "db"
static constexpr std::string_view LG_NAME{_LG_NAME};

std::expected<db::PGconnUR, std::string_view> db::connect() {
  PGconnUR conn{PQconnectdb(""), PQfinish};

  if (PQstatus(conn.get()) != CONNECTION_OK) {
    conn.release();

    return std::unexpected(PQerrorMessage(conn.get()));
  }

  return conn;
}

static constexpr std::string_view LG_NAME_EXEC{_LG_NAME ".exec"};
std::expected<db::PGresultUR, std::string_view> db::exec(
  const db::PGconnUR& c,
  std::string_view sql,
  std::vector<char*> params
) {
  lg::debug(LG_NAME_EXEC, (std::stringstream()
    << "executing ```"
    << sql
    << "```"
  ).view());
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

  const ExecStatusType s{PQresultStatus(res.get())};
  lg::debug(LG_NAME_EXEC, [&s](){
    return (std::stringstream()
      << "result `"
      << PQresStatus(s)
      << "`"
    ).str();
  });
  if (
    s != PGRES_TUPLES_OK
    && s != PGRES_TUPLES_CHUNK
    && s != PGRES_SINGLE_TUPLE
    && s != PGRES_COMMAND_OK
  ) {
    return std::unexpected(PQerrorMessage(c.get()));
  }

  return res;
}

std::expected<db::PGresultUR, std::string_view> db::exec(
  const db::PGconnUR& c,
  std::string_view sql
) {
  return db::exec(c, sql, std::vector<char*>{});
}

static constexpr std::string_view LG_NAME_PREPARE{_LG_NAME ".prepare"};
std::expected<void, std::string_view> db::prepare(
  const db::PGconnUR& c,
  std::string_view name,
  std::string_view sql
) {
  lg::debug(LG_NAME_PREPARE, (std::stringstream()
    << "preparing `"
    << name
    << "` as ```"
    << sql
    << "```"
  ).view());
  db::PGresultUR res{
    PQprepare(
      c.get(),
      name.data(),
      sql.data(),
      0,
      nullptr
    ),
    PQclear
  };

  const ExecStatusType s{PQresultStatus(res.get())};
  lg::debug(LG_NAME_PREPARE, [&s](){
    return (std::stringstream()
      << "result `"
      << PQresStatus(s)
      << "`"
    ).str();
  });
  if (
    s != PGRES_TUPLES_OK
    && s != PGRES_TUPLES_CHUNK
    && s != PGRES_SINGLE_TUPLE
    && s != PGRES_COMMAND_OK
  ) {
    return std::unexpected(PQerrorMessage(c.get()));
  }

  return {};
}

void join(
  std::stringstream& out,
  std::string_view delim,
  std::vector<char*> items
) {
  if (items.size() == 0) return;

  bool first{true};
  for (const char* item : items) {
    if (!first) out << delim;
    else first = false;

    out << item;
  }
}

static constexpr std::string_view LG_NAME_EXEC_PREPARED{
  _LG_NAME ".execPrepared"
};
std::expected<db::PGresultUR, std::string_view> db::execPrepared(
  const db::PGconnUR& c,
  std::string_view name,
  std::vector<char*> params
) {
  lg::debug(LG_NAME_EXEC_PREPARED, [&name, &params](){
    std::stringstream out{};
    out << "executing `" << name << "` with params `(";
    join(out, ", ", params);
    out << ")`";

    return out.str();
  });
  db::PGresultUR res{
    PQexecPrepared(
      c.get(),
      name.data(),
      params.size(),
      params.data(),
      nullptr,
      nullptr,
      1
    ),
    PQclear
  };

  const ExecStatusType s{PQresultStatus(res.get())};
  lg::debug(LG_NAME_EXEC_PREPARED, [&s](){
    return (std::stringstream()
      << "result `"
      << PQresStatus(s)
      << "`"
    ).str();
  });
  if (
    s != PGRES_TUPLES_OK
    && s != PGRES_TUPLES_CHUNK
    && s != PGRES_SINGLE_TUPLE
    && s != PGRES_COMMAND_OK
  ) {
    return std::unexpected(PQerrorMessage(c.get()));
  }

  return res;
}

std::expected<db::PGresultUR, std::string_view> db::execPrepared(
  const db::PGconnUR& c,
  std::string_view name
) {
  return db::execPrepared(c, name, {});
}
