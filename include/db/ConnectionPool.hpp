#pragma once

#include <mysql/mysql.h>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <cstddef>

class ConnectionPool
{
public:
    static void Init(const std::string& host, unsigned int port,
                     const std::string& user, const std::string& password,
                     const std::string& dbname, size_t poolSize);
    static ConnectionPool* Instance();
    static void Destroy();

    MYSQL* GetConnection();
    void ReturnConnection(MYSQL* conn);

private:
    ConnectionPool(const std::string& host, unsigned int port,
                   const std::string& user, const std::string& password,
                   const std::string& dbname, size_t poolSize);
    ~ConnectionPool();

    MYSQL* CreateConnection();

    static ConnectionPool* instance_;

    std::string host_, user_, password_, dbname_;
    unsigned int port_;
    size_t poolSize_;

    std::queue<MYSQL*> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

// RAII guard: 自动获取和归还连接
class DBGuard
{
public:
    explicit DBGuard(ConnectionPool* pool);
    ~DBGuard();

    MYSQL* Get() const { return conn_; }

    DBGuard(const DBGuard&) = delete;
    DBGuard& operator=(const DBGuard&) = delete;

private:
    ConnectionPool* pool_;
    MYSQL* conn_;
};
