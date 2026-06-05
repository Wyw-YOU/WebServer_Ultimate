---
name: db-pool
description: Implement or modify the database connection pool. Use when working on MySQL/SQLite connections, query execution, or connection management.
---

# Database Connection Pool Skill

Guide for implementing a database connection pool.

## Architecture

```
Application Threads
  │  │  │  │
  ▼  ▼  ▼  ▼
┌──────────────────┐
│  Connection Pool │
│  ┌──┬──┬──┬──┐   │
│  │C1│C2│C3│C4│   │  ← idle connections
│  └──┴──┴──┴──┘   │
│  max_size = 8    │
│  min_size = 2    │
└──────────────────┘
        │
        ▼
   MySQL Server
```

## Implementation

```cpp
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <chrono>

class MySqlConnection {
public:
    MySqlConnection(const std::string& host, int port,
                    const std::string& user, const std::string& pass,
                    const std::string& db);
    ~MySqlConnection();

    bool Connect();
    void Disconnect();
    bool Ping();  // keep-alive check

    // Execute query (returns affected rows or result set)
    bool Execute(const std::string& sql);

    bool IsValid() const;
    std::chrono::steady_clock::time_point GetLastUsed() const;

private:
    void* conn_;  // MYSQL* (avoid header dependency)
    std::string host_, user_, pass_, db_;
    int port_;
    std::chrono::steady_clock::time_point last_used_;
};

class ConnectionPool {
public:
    static ConnectionPool& Instance();

    void Init(const std::string& host, int port,
              const std::string& user, const std::string& pass,
              const std::string& db, size_t pool_size);

    std::shared_ptr<MySqlConnection> GetConnection();
    void ReturnConnection(std::shared_ptr<MySqlConnection> conn);

    ~ConnectionPool();

private:
    ConnectionPool();

    std::queue<std::shared_ptr<MySqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_size_ = 8;
    size_t min_size_ = 2;
    bool running_ = true;

    std::shared_ptr<MySqlConnection> CreateConnection();
    void ReapIdleConnections();  // background thread
};
```

## RAII Wrapper

```cpp
class DbGuard {
public:
    DbGuard(ConnectionPool& pool) : conn_(pool.GetConnection()), pool_(pool) {}
    ~DbGuard() { pool_.ReturnConnection(conn_); }

    MyConnection* operator->() { return conn_.get(); }

private:
    std::shared_ptr<MyConnection> conn_;
    ConnectionPool& pool_;
};

// Usage:
DbGuard db(ConnectionPool::Instance());
db->Execute("SELECT * FROM users");
```

## Keep-Alive

- Run a background thread to `PING` idle connections every 60s
- Remove connections idle for > 300s
- Recreate connections that fail the ping

## Configuration

```cpp
// Config file or command-line args
db.host     = "127.0.0.1"
db.port     = 3306
db.user     = "root"
db.password = ""
db.database = "webserver"
db.pool_size = 8
```

## Important

- Use RAII to ensure connections are always returned to the pool
- Handle `MySQL server has gone away` by reconnecting
- Use prepared statements to prevent SQL injection
- Set connection timeout (e.g., 5s) to avoid hanging on bad DB
- Log connection creation/destruction for debugging
