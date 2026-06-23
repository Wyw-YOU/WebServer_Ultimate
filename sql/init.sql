-- WebServer_Ultimate 数据库初始化脚本
-- 使用方式: mysql -u root -p < sql/init.sql

CREATE DATABASE IF NOT EXISTS webserver DEFAULT CHARACTER SET utf8mb4;
USE webserver;

CREATE TABLE IF NOT EXISTS users (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    username   VARCHAR(50)  NOT NULL UNIQUE,
    password   VARCHAR(100) NOT NULL  -- 存储格式：salt:hash（97 字符）
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 测试数据（使用 SHA-256 哈希密码）
-- 密码格式：salt_hex:hash_hex
-- 示例：admin 用户密码为 "123456"
-- salt: a1b2c3d4e5f67890a1b2c3d4e5f67890
-- hash: SHA-256(salt + "123456")

-- 注意：实际部署时应使用应用程序生成哈希密码
-- 这里使用 MySQL 的 SHA2 函数作为示例
INSERT IGNORE INTO users (username, password) VALUES
('admin', CONCAT(
    HEX(RANDOM_BYTES(16)),  -- 生成 32 字符的随机盐
    ':',
    SHA2(CONCAT(HEX(RANDOM_BYTES(16)), '123456'), 256)  -- SHA-256 哈希
));

INSERT IGNORE INTO users (username, password) VALUES
('test', CONCAT(
    HEX(RANDOM_BYTES(16)),  -- 生成 32 字符的随机盐
    ':',
    SHA2(CONCAT(HEX(RANDOM_BYTES(16)), 'test123'), 256)  -- SHA-256 哈希
));

-- 创建索引（如果需要）
-- CREATE INDEX idx_username ON users(username);
