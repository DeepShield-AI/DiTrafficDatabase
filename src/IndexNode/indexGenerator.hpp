#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../lib/util.hpp"
#include "../lib/memoryIndex.hpp"
#include "../lib/singleRingBuffer.hpp"


class IndexGenerator{
private:
    PointerRingBuffer* indexRing;
    MemoryIndex* memoryIndex;

    bool stop;
    FlowIndex* readIndexFromBuffer();
    void putIndexToCache(FlowIndex* index);

public:
    IndexGenerator(PointerRingBuffer* index_ring, MemoryIndex* memory_index);
    ~IndexGenerator();
    void run();
    void asynchronousStop();
};

#endif