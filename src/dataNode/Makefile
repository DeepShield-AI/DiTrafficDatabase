# 编译器与编译选项
CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2
LDFLAGS := -lpcap

# 源文件与目标目录
SRC := main.cpp dataNode.cpp regionServer.cpp mitoEngine.cpp regionWorker.cpp flusher.cpp
OBJ_DIR := ../../build/dataNode
OBJS := $(addprefix $(OBJ_DIR)/, $(SRC:.cpp=.o))
TARGET := $(OBJ_DIR)/dataNode

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
	mkdir -p $(OBJ_DIR)

# 伪目标
.PHONY: all clean
