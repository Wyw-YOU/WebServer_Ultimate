-- WebServer_Ultimate 数据库初始化脚本
-- 使用方式: mysql -u root -p < sql/init.sql

CREATE DATABASE IF NOT EXISTS webserver DEFAULT CHARACTER SET utf8mb4;
USE webserver;

CREATE TABLE IF NOT EXISTS users (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    username   VARCHAR(50)  NOT NULL UNIQUE,
    password   VARCHAR(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 测试数据
INSERT IGNORE INTO users (username, password) VALUES ('admin', '123456');
INSERT IGNORE INTO users (username, password) VALUES ('test',  'test123');
