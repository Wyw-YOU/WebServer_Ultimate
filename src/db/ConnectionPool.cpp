#include "db/ConnectionPool.hpp"
#include "Log.hpp"

ConnectionPool* ConnectionPool::instance_ = nullptr;

ConnectionPool::ConnectionPool(const std::string& host, unsigned int port,
                               const std::string& user, const std::string& password,
                               const std::string& dbname, size_t poolSize)
    : host_(host), user_(user), password_(password), dbname_(dbname),
      port_(port), poolSize_(poolSize)
{
    for(size_t i = 0; i < poolSize_; ++i)
    {
        MYSQL* conn = CreateConnection();
        if(conn)
        {
            pool_.push(conn);
        }
    }
    LOG_NORMAL("ConnectionPool initialized with " + std::to_string(pool_.size()) + " connections");
}

ConnectionPool::~ConnectionPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while(!pool_.empty())
    {
        mysql_close(pool_.front());
        pool_.pop();
    }
}

void ConnectionPool::Init(const std::string& host, unsigned int port,
                          const std::string& user, const std::string& password,
                          const std::string& dbname, size_t poolSize)
{
    if(!instance_)
    {
        instance_ = new ConnectionPool(host, port, user, password, dbname, poolSize);
    }
}

ConnectionPool* ConnectionPool::Instance()
{
    return instance_;
}

void ConnectionPool::Destroy()
{
    delete instance_;
    instance_ = nullptr;
}

MYSQL* ConnectionPool::CreateConnection()
{
    MYSQL* conn = mysql_init(nullptr);
    if(!conn)
    {
        LOG_ERROR("mysql_init failed");
        return nullptr;
    }

    // 5 秒连接超时
    int timeout = 5;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if(!mysql_real_connect(conn, host_.c_str(), user_.c_str(),
                           password_.c_str(), dbname_.c_str(),
                           port_, nullptr, 0))
    {
        LOG_ERROR(std::string("mysql_real_connect failed: ") + mysql_error(conn));
        mysql_close(conn);
        return nullptr;
    }

    mysql_set_character_set(conn, "utf8mb4");
    return conn;
}

MYSQL* ConnectionPool::GetConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);

    // 池非空，直接取
    if(!pool_.empty())
    {
        MYSQL* conn = pool_.front();
        pool_.pop();
        return conn;
    }

    // 池空，等待最多 5 秒
    cond_.wait_for(lock, std::chrono::seconds(5), [this]{ return !pool_.empty(); });

    if(pool_.empty())
    {
        // 超时，尝试新建一个
        lock.unlock();
        return CreateConnection();
    }

    MYSQL* conn = pool_.front();
    pool_.pop();
    return conn;
}

void ConnectionPool::ReturnConnection(MYSQL* conn)
{
    if(!conn)
        return;

    // 检测连接是否存活
    if(mysql_ping(conn) != 0)
    {
        // 连接已断开，尝试重建
        mysql_close(conn);
        conn = CreateConnection();
        if(!conn)
            return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    cond_.notify_one();
}

// ========== DBGuard ==========

DBGuard::DBGuard(ConnectionPool* pool)
    : pool_(pool), conn_(pool ? pool->GetConnection() : nullptr)
{
}

DBGuard::~DBGuard()
{
    if(pool_ && conn_)
    {
        pool_->ReturnConnection(conn_);
    }
}
