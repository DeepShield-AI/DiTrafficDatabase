#include "sortedArray.hpp"

// UINT64 + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBlock<u_int64_t>
> reg_uint64_sorted(
    ValueType::UINT64,
    InvertedKind::SORTED_ARRAY
);

// INT64 + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBlock<int64_t>
> reg_int64_sorted(
    ValueType::INT64,
    InvertedKind::SORTED_ARRAY
);

// DOUBLE + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBlock<double>
> reg_double_sorted(
    ValueType::DOUBLE,
    InvertedKind::SORTED_ARRAY
);

// UINT64 + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBuilder<u_int64_t>
> reg_uint64_sorted(
    ValueType::UINT64,
    InvertedKind::SORTED_ARRAY
);

// INT64 + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBuilder<int64_t>
> reg_int64_sorted(
    ValueType::INT64,
    InvertedKind::SORTED_ARRAY
);

// DOUBLE + SORTED_ARRAY
static BuilderRegistrar<
    SortedArrayIndexBuilder<double>
> reg_double_sorted(
    ValueType::DOUBLE,
    InvertedKind::SORTED_ARRAY
);