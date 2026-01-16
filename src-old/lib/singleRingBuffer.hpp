#ifndef SINGLERINGBUFFER_HPP_
#define SINGLERINGBUFFER_HPP_
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <sys/mman.h>
#include "header.hpp"
// #include "util.hpp"
#define CACHE_LINE_LEN 64

// write Not covered, read covered
class PointerRingBuffer{
private:
    const u_int32_t capacity_;
    alignas(CACHE_LINE_LEN) u_int64_t writePos;
    char writepadding[CACHE_LINE_LEN - sizeof(uint64_t)];
    alignas(CACHE_LINE_LEN) std::atomic_uint_fast64_t readPos;
    char readpadding[CACHE_LINE_LEN - sizeof(uint64_t)];

    void** pointers;

    std::atomic_bool stop;

    bool isPowerOfTwo(u_int32_t n) {
        return (n & (n - 1)) == 0;
    }
public:
    PointerRingBuffer(u_int32_t capacity):capacity_(capacity){
        if(this->capacity_ & (this->capacity_ - 1)){
            printf("PointerRingBuffer error: capacity %u is not power of 2!\n",capacity);
            this->pointers = nullptr;
            return;
        }
        // this->pointers = (void**)mmap(nullptr, this->capacity_ * sizeof(void*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        // if (this->pointers == MAP_FAILED){
        //     printf("Pointer ring buffer error: mmap failed for blocks!\n");
        //     throw std::runtime_error("memory manager mmap failed");
        // }
        this->pointers = new void*[this->capacity_];
        for(u_int32_t i = 0;i<this->capacity_;++i){
            this->pointers[i] = nullptr;
        }
        this->writePos = 0;
        this->readPos = 0;
        
        this->stop = false;
    }
    ~PointerRingBuffer(){
        delete this->pointers;
    }
    bool put(void* data){
        u_int64_t pos = this->writePos;
        pos &= this->capacity_ - 1;
        
        while(this->writePos == this->capacity_ - 1 + this->readPos){
            printf("ring buffer wait\n");
        } // wait util not writed

        u_int64_t real_pos = ((pos & ((this->capacity_ >> 3) - 1)) << 3) + pos / (this->capacity_ >> 3);

        this->pointers[real_pos] = data;
        this->writePos ++;

        return true;
    }
    void* get(){
        u_int64_t pos = this->readPos;
        pos &= this->capacity_ - 1;
        if(this->writePos == this->readPos){
            return nullptr;
        }

        u_int64_t real_pos = ((pos & ((this->capacity_ >> 3) - 1)) << 3) + pos / (this->capacity_ >> 3);

        void* data = this->pointers[real_pos];
        this->readPos ++;
 
        return data;
    }
    bool initSucceed()const{
        return this->pointers != nullptr;
    }
    void asynchronousStop(){
        this->stop = true;
    }
};
#endif