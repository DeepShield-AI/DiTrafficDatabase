#include "indexGenerator.hpp"

IndexGenerator::IndexGenerator(PointerRingBuffer* index_ring, MemoryIndex* memory_index){
    this->indexRing = index_ring;
    this->memoryIndex = memory_index;
    this->stop = true;
}
IndexGenerator::~IndexGenerator(){
}
FlowIndex* IndexGenerator::readIndexFromBuffer(){
    void* data = this->indexRing->get();
    return (FlowIndex*)data;
}
void IndexGenerator::putIndexToCache(FlowIndex* index){
    this->memoryIndex->insertIndex(index);
    delete index;
}
void IndexGenerator::run(){
    this->stop = false;
    while(true){
        FlowIndex* index = this->readIndexFromBuffer();
        if(this->stop){
            break;
        }
        if(index == nullptr){
            continue;
        }
        this->putIndexToCache(index);
    }
    printf("FlowIndex Generator log: stopping...\n");
    while(true){
        FlowIndex* index = this->readIndexFromBuffer();
        if(index == nullptr){
            break;
        }
        this->putIndexToCache(index);
    }
}
void IndexGenerator::asynchronousStop(){
    this->stop = true;
}