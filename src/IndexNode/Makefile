# 编译器与编译选项
CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2
LDFLAGS := -pthread

# 源文件与目标目录
SRC := main.cpp heartbeatClient.cpp
OBJ_DIR := ../../build/indexNode
OBJS := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))
TARGET := $(OBJ_DIR)/indexer

# 默认目标
all: $(TARGET)

# 链接规则
$(TARGET): $(OBJS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译规则
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -rf $(OBJ_DIR)

# 伪目标
.PHONY: all clean
