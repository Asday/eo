-- name: register :one
-- Registers this launcher with the database for service discovery.
INSERT INTO launcher (ip, port, heartbeat)
VALUES (inet_client_addr(), sqlc.arg(port), NOW()::timestamp)
RETURNING id
;

-- name: heartbeat :exec
-- Pings the database as a service discovery health check.
UPDATE launcher
SET heartbeat = NOW()::timestamp
WHERE id = sqlc.arg(id)
;

-- name: unregister :exec
-- Removes this launcher from the service discovery table.
DELETE FROM launcher
WHERE id = sqlc.arg(id)
;
