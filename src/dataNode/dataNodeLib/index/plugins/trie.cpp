#include "trie.hpp"

static BuilderRegistrar<
    TrieIndexBlock<uint64_t>
> reg_uint64_sorted(
    ValueType::STRING,
    InvertedKind::TRIE
);

static BuilderRegistrar<
    TrieIndexBuilder<uint64_t>
> reg_uint64_sorted(
    ValueType::STRING,
    InvertedKind::TRIE
);