#pragma once
#include "invertedIndex.hpp"

class InvertedIndexLoaderRegistry {
public:
    using Key = std::pair<ValueType, InvertedKind>;
    using Creator = std::function<std::unique_ptr<InvertedIndexBlock>(char*)>;

    static InvertedIndexLoaderRegistry& instance() {
        static InvertedIndexLoaderRegistry inst;
        return inst;
    }

    void register_loader(ValueType vt,InvertedKind kind,Creator creator){
        loaders[{vt, kind}] = std::move(creator);
    }

    std::unique_ptr<InvertedIndexBlock> create(ValueType vt,InvertedKind kind,char* buffer){
        return loaders.at({vt, kind})(buffer);
    }

private:
    struct PairHash {
        size_t operator()(const Key& p) const {
            return std::hash<int>()((int)p.first)
                ^ (std::hash<int>()((int)p.second) << 1);
        }
    };

    std::unordered_map<Key, Creator, PairHash> loaders;
};

template<typename IndexT>
class IndexLoaderRegistrar {
public:
    IndexLoaderRegistrar(ValueType vt,InvertedKind kind){
        InvertedIndexLoaderRegistry::instance().register_loader(
            vt,
            kind,
            [](char* buffer) {
                return std::make_unique<IndexT>(buffer);
            });
    }
};
