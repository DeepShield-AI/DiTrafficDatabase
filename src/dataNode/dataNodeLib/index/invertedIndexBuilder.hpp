#pragma once
#include "invertedIndex.hpp"

class InvertedIndexBuilder {
public:
    virtual ~InvertedIndexBuilder() = default;
    virtual std::unique_ptr<InvertedIndexBlock> build(std::shared_ptr<Memtable> memtable,const std::string& column_name) = 0;
};

class InvertedIndexBuilderRegistry {
public:
    using Key = std::pair<ValueType, InvertedKind>;
    using Creator = std::function<std::unique_ptr<InvertedIndexBuilder>()>;

    static InvertedIndexBuilderRegistry& instance() {
        static InvertedIndexBuilderRegistry inst;
        return inst;
    }

    void register_builder(ValueType vt,InvertedKind kind,Creator creator){
        builders[{vt, kind}] = std::move(creator);
    }

    std::unique_ptr<InvertedIndexBuilder> create(ValueType vt,InvertedKind kind){
        return builders.at({vt, kind})();
    }

private:
    struct PairHash {
        size_t operator()(const Key& p) const {
            return std::hash<int>()((int)p.first)
                ^ (std::hash<int>()((int)p.second) << 1);
        }
    };

    std::unordered_map<Key, Creator, PairHash> builders;
};

template<typename BuilderT>
class BuilderRegistrar {
public:
    BuilderRegistrar(ValueType vt,InvertedKind kind){
        InvertedIndexBuilderRegistry::instance()
            .register_builder(
                vt,
                kind,
                []() {
                    return std::make_unique<BuilderT>();
                });
    }
};
