#ifndef BITMAP_HPP_
#define BITMAP_HPP_
#include <iostream>
#include <sys/mman.h>
#include <algorithm>
#include <climits>
#include <atomic>
#include <limits>

// 使用位图来表示索引

class BitMap{
private:
    u_int8_t* bitmap;
    u_int64_t row_count; // equal to Bloom filter num
    u_int64_t col_count;
    u_int64_t backup_col_count; 
    u_int64_t size;

    u_int64_t getByteIndex(u_int64_t row, u_int64_t col) const{
        if (row >= this->row_count || col >= this->col_count){
            printf("Bitmap error: row %lu and col %lu out of range!\n",row,col);
            return std::numeric_limits<u_int64_t>::max();
        }
        u_int64_t byte_col = col % (this->col_count/8);
        // printf("col_count:%lu, bitmap_size:%lu\n", this->col_count, this->size);
        return row * (this->col_count / 8) + byte_col;
    }
    u_int8_t getBitIndex(u_int64_t col) const{
        if (col >= this->col_count){
            printf("Bitmap error: col %lu out of range!\n", col);
            return std::numeric_limits<u_int8_t>::max();
        }
        return col / (this->col_count/8);
    }
public:
    // backup_col_count should be bigger than RSS_NUM*4
    BitMap(u_int64_t row_count, u_int64_t logic_col_count, u_int64_t backup_col_count): 
        row_count(row_count), col_count(logic_col_count + backup_col_count), backup_col_count(backup_col_count) {
        if (this->col_count % 8){
            printf("Bitmap error: col_count %lu is not a multiple of u_int8_t size!\n", col_count);
            throw std::runtime_error("Invalid column count for bitmap");
        }
        if (this->backup_col_count * 8 > this->col_count){
            printf("Bitmap error: logic_col_count %lu is not bigger than 8 times of backup_col_count %lu!\n", col_count, backup_col_count);
            throw std::runtime_error("Invalid column count for bitmap");
        }
        this->size = this->row_count * this->col_count / 8;
        // this->bitmap = (u_int8_t*)mmap(nullptr, this->size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        // if (this->bitmap == MAP_FAILED){
        //     printf("Disk buffer error: mmap failed for disk metas!\n");
        //     throw std::runtime_error("Disk buffer mmap failed");
        // }
        this->bitmap = new u_int8_t[this->size];
        for (u_int64_t i = 0; i < this->size; ++i) {
            this->bitmap[i] = 0; // Initialize the bitmap to zero
        }
    }
    ~BitMap() {
        // munmap((void*)this->bitmap, this->size);
        delete[] this->bitmap;
    }

    u_int64_t getRowCount() const {
        return this->row_count;
    }
    u_int64_t getColCount() const {
        return this->col_count;
    }
    u_int64_t getBackupColCount() const {
        return this->backup_col_count;
    }

    bool get(u_int64_t row, u_int64_t col) const{
        u_int64_t byte_index = this->getByteIndex(row, col);
        if (byte_index == std::numeric_limits<u_int64_t>::max()) return false;
        u_int8_t bit_index = this->getBitIndex(col);
        if (bit_index == std::numeric_limits<u_int8_t>::max()) return false;
        bool ret = (bitmap[byte_index] & (1 << bit_index)) != 0;
        // if (col == this->cleaning_col.load()) return false;
        return ret;
    }
    void clearCol(u_int64_t col) {
        u_int8_t bit_index = getBitIndex(col);
        if (bit_index == std::numeric_limits<u_int8_t>::max()) return;
        for (u_int64_t row = 0; row < this->row_count; ++row){
            u_int64_t byte_index = getByteIndex(row, col);           
            bitmap[byte_index] &= ~(1 << bit_index);
        }
    }
    // set col should in the write field (cleaning - 2*backup, cleanning)
    bool set(u_int64_t row, u_int64_t col) { 
        u_int64_t byte_index = this->getByteIndex(row, col);
        if (byte_index == std::numeric_limits<u_int64_t>::max()) return false;
        u_int8_t bit_index = this->getBitIndex(col);
        if (bit_index == std::numeric_limits<u_int8_t>::max()) return false;
        // printf("byte_index:%lu, bit_index:%lu\n",byte_index,bit_index);
        bitmap[byte_index] |= (((u_int8_t)1) << bit_index);
        // printf("finsh\n");
        return true;
    }
};

#endif