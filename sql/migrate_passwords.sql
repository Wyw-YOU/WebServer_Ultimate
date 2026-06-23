-- WebServer_Ultimate 密码哈希迁移脚本
-- 使用方式: mysql -u root -p webserver < sql/migrate_passwords.sql
--
-- ⚠️ 警告：此脚本会修改数据库中的密码字段！
-- 请在执行前备份数据库！
--
-- 执行步骤：
-- 1. 停止 WebServer 服务
-- 2. 备份数据库：mysqldump -u root -p webserver > backup.sql
-- 3. 执行此脚本：mysql -u root -p webserver < migrate_passwords.sql
-- 4. 部署新版本代码
-- 5. 启动 WebServer 服务
-- 6. 验证登录功能

USE webserver;

-- 创建临时表用于备份（可选，额外安全措施）
-- CREATE TABLE users_backup AS SELECT * FROM users;

-- 修改 password 字段长度（如果需要）
-- VARCHAR(100) 已经足够存储 salt:hash 格式（97 字符）
-- ALTER TABLE users MODIFY COLUMN password VARCHAR(100) NOT NULL;

-- 迁移函数：将明文密码转换为 salt:hash 格式
DELIMITER //

CREATE FUNCTION IF NOT EXISTS hash_password(plain_password VARCHAR(100))
RETURNS VARCHAR(100) DETERMINISTIC
BEGIN
    DECLARE salt_hex VARCHAR(32);
    DECLARE hash_hex VARCHAR(64);
    DECLARE result VARCHAR(100);

    -- 生成 16 字节随机盐，转换为 32 字符的 hex 串
    SET salt_hex = HEX(RANDOM_BYTES(16));

    -- 计算 SHA-256(salt + password)
    SET hash_hex = SHA2(CONCAT(salt_hex, plain_password), 256);

    -- 返回 salt:hash 格式
    SET result = CONCAT(salt_hex, ':', hash_hex);

    RETURN result;
END //

DELIMITER ;

-- 更新现有用户的密码（将明文转换为哈希）
-- 注意：这个操作需要为每个用户单独执行，因为需要生成不同的盐
UPDATE users
SET password = hash_password(password)
WHERE password NOT LIKE '%:%'  -- 只更新没有 ':' 的记录（即明文密码）
   OR LENGTH(password) != 97;  -- 或长度不是 97 的记录

-- 删除临时函数
DROP FUNCTION IF EXISTS hash_password;

-- 验证迁移结果
SELECT
    id,
    username,
    LENGTH(password) as password_length,
    CASE
        WHEN password LIKE '%:%' AND LENGTH(password) = 97 THEN '已哈希'
        ELSE '未迁移'
    END as status
FROM users;

-- 显示迁移统计
SELECT
    COUNT(*) as total_users,
    SUM(CASE WHEN password LIKE '%:%' AND LENGTH(password) = 97 THEN 1 ELSE 0 END) as hashed_users,
    SUM(CASE WHEN password NOT LIKE '%:%' OR LENGTH(password) != 97 THEN 1 ELSE 0 END) as unhashed_users
FROM users;

-- 完成提示
SELECT 'Password migration completed!' as message;
SELECT 'Please verify login functionality with the new code.' as next_step;
