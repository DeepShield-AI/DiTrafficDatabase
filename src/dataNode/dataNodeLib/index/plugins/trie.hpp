#pragma once
#include <queue>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string.h>
#include "../invertedIndexBuilder.hpp"
#include "../invertedIndexLoader.hpp"

#pragma pack(push, 1)
struct TrieNode {
    u_int32_t first_child;   // 子节点在 nodes[] 中的起始下标
    u_int16_t child_count;   // 子节点数量
    u_int8_t  ch;            // 当前字符（root 节点可忽略）
    u_int8_t  is_end;        // 是否为完整 key

    u_int32_t value_offset;  // value 区域在 buffer 中的偏移
    u_int32_t value_count;   // value 个数
};
#pragma pack(pop)

class TrieIndexBlock : public TypedInvertedIndexBlock<std::string> {
    TrieNode* nodes;
    u_int32_t* node_count_ptr;
    u_int32_t  node_count;
public:
    // 新建 block
    TrieIndexBlock(u_int64_t buffer_size): TypedInvertedIndexBlock<std::string>(buffer_size, InvertedKind::TRIE){
        char* ptr = this->buffer + sizeof(InvertedIndexBlockInfo);
        node_count_ptr = reinterpret_cast<u_int32_t*>(ptr);
        nodes = reinterpret_cast<TrieNode*>(ptr + sizeof(u_int32_t));
    }
    // 从磁盘恢复
    explicit TrieIndexBlock(char* buffer): TypedInvertedIndexBlock<std::string>(buffer){
        char* ptr = this->buffer + sizeof(InvertedIndexBlockInfo);
        node_count_ptr = reinterpret_cast<u_int32_t*>(ptr);
        node_count = *node_count_ptr;
        nodes = reinterpret_cast<TrieNode*>(ptr + sizeof(u_int32_t));
    }

    void setNodeCount(u_int32_t n) {
        *node_count_ptr = n;
        node_count = n;
    }

    TrieNode* getNodes() { return nodes; }
    u_int32_t getNodeCount() const { return node_count; }

    void lookup(const std::string& key, std::vector<RowId>& result) const override{
        if (node_count == 0) return;
        u_int32_t current = 0;  // root

        for (char c : key) {
            bool found = false;
            TrieNode& node = nodes[current];
            for (u_int16_t i = 0; i < node.child_count; ++i) {
                u_int32_t child_idx = node.first_child + i;
                if (nodes[child_idx].ch == c) {
                    current = child_idx;
                    found = true;
                    break;
                }
            }
            if (!found) return;
        }

        TrieNode& target = nodes[current];

        if (!target.is_end) return;

        RowId* values = reinterpret_cast<RowId*>(
            this->buffer + target.value_offset
        );

        for (u_int32_t i = 0; i < target.value_count; ++i) {
            result.push_back(values[i]);
        }
    }
};

struct TempTrieNode {
    std::unordered_map<char, std::unique_ptr<TempTrieNode>> children;
    std::vector<RowId> values;
};

template<typename T>
class TrieIndexBuilder : public InvertedIndexBuilder {
private:
    void insert(TempTrieNode* root,const std::string& key,RowId row_id){
        TempTrieNode* node = root;
        for (char c : key) {
            auto& child = node->children[c];
            if (!child) {
                child = std::make_unique<TempTrieNode>();
            }
            node = child.get();
        }
        node->values.push_back(row_id);
    }

    // 统计节点数量
    u_int32_t countNodes(TempTrieNode* node) {
        u_int32_t total = 1;
        for (auto& [_, child] : node->children){
            total += countNodes(child);
        }
        return total;
    }

    // 统计 value 总数
    u_int32_t countValues(TempTrieNode* node) {
        u_int32_t total = node->values.size();
        for (auto& [_, child] : node->children)
            total += countValues(child);
        return total;
    }

    // 扁平化
    u_int32_t flatten(TempTrieNode* node, TrieNode* nodes, u_int32_t& node_cursor, char* buffer,u_int32_t& value_offset){
        u_int32_t current = node_cursor++;
        TrieNode& n = nodes[current];

        n.child_count = node->children.size();
        n.first_child = node_cursor;
        n.is_end = !node->values.empty();
        n.ch = 0;  // root 会覆盖

        if (n.is_end) {
            n.value_offset = value_offset;
            n.value_count = node->values.size();
            memcpy(buffer + value_offset,node->values.data(),node->values.size() * sizeof(RowId));
            value_offset += node->values.size() * sizeof(RowId);
        } else {
            n.value_offset = 0;
            n.value_count = 0;
        }

        for (auto& [ch, child] : node->children) {
            nodes[node_cursor].ch = ch;
            flatten(child.get(), nodes, node_cursor,buffer, value_offset);
        }

        return current;
    }

public:
    std::unique_ptr<InvertedIndexBlock> build(std::shared_ptr<Memtable> memtable,const std::string& column_name) override{
        auto table = memtable->getTable();
        auto column_id = memtable->getColumnId(column_name);
        TempTrieNode* root = std::make_unique<TempTrieNode>();

        RowId row_id = 0;
        for (auto& row : table) {
            T key = row.data[column_id];
            insert(root, key, row_id);
            row_id++;
        }

        u_int32_t node_count = countNodes(root);
        u_int32_t value_count = countValues(root);

        u_int64_t total_size = sizeof(InvertedIndexBlockInfo) + sizeof(u_int32_t) + node_count * sizeof(TrieNode) + value_count * sizeof(RowId);
        TrieIndexBlock* block = new TrieIndexBlock(total_size);

        TrieNode* nodes = block->getNodes();
        u_int32_t node_cursor = 0;
        u_int32_t value_offset = sizeof(InvertedIndexBlockInfo) + sizeof(u_int32_t) + node_count * sizeof(TrieNode);

        flatten(root,nodes,node_cursor,block->buffer,value_offset);
        block->setNodeCount(node_count);
        return std::make_unique<InvertedIndexBlock>(block);
    }
};
