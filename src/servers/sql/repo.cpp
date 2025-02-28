namespace repo {
  // Registers this launcher with the database for service discovery.
  const auto sql{"INSERT INTO launcher (ip, port, heartbeat)
VALUES (inet_client_addr(), $1, NOW()::timestamp)
RETURNING id"};
  PGresult* r{PQexecParams(
    conn,
    sql,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    1
  )};
  const ExecStatusType s{PQresultStatus(r)};
  if (s != PGRES_TUPLES_OK){
    return std::unexpected(PQerrorMessage(conn));
  }
  // Pings the database as a service discovery health check.
  const auto sql{"UPDATE launcher
SET heartbeat = NOW()::timestamp
WHERE id = $1"};
  PGresult* r{PQexecParams(
    conn,
    sql,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    1
  )};
  const ExecStatusType s{PQresultStatus(r)};
  if (s != PGRES_COMMAND_OK){
    return std::unexpected(PQerrorMessage(conn));
  }
  // Removes this launcher from the service discovery table.
  const auto sql{"DELETE FROM launcher
WHERE id = $1"};
  PGresult* r{PQexecParams(
    conn,
    sql,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    1
  )};
  const ExecStatusType s{PQresultStatus(r)};
  if (s != PGRES_COMMAND_OK){
    return std::unexpected(PQerrorMessage(conn));
  }
}
