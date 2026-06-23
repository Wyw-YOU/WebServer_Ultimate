#!/bin/bash

# WebServer_Ultimate 启动脚本
# 使用方法：./start.sh [端口号]

# 默认端口
PORT=${1:-8080}

# 检查 .env 文件是否存在
if [ -f .env ]; then
    echo "Loading configuration from .env file..."
    # 加载 .env 文件（忽略注释和空行）
    export $(cat .env | grep -v '^#' | grep -v '^$' | xargs)
else
    echo "Warning: .env file not found"
    echo "Please create .env file from .env.example:"
    echo "  cp .env.example .env"
    echo "  vi .env  # Edit and set DB_PASSWORD"
    exit 1
fi

# 检查 DB_PASSWORD 是否设置
if [ -z "$DB_PASSWORD" ]; then
    echo "Error: DB_PASSWORD is not set in .env file"
    echo "Please edit .env file and set DB_PASSWORD"
    exit 1
fi

# 启动服务器
echo "Starting WebServer on port $PORT..."
./server $PORT
