#include "trie.hpp"

static BuilderRegistrar<
    TrieIndexBlock<std::string>
> reg_uint64_sorted(
    ValueType::STRING,
    InvertedKind::TRIE
);

static BuilderRegistrar<
    TrieIndexBuilder<std::string>
> reg_uint64_sorted(
    ValueType::STRING,
    InvertedKind::TRIE
);