#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
extern "C" {
    #include "extmem.h"
}

#ifndef ADAPTER_H
#define ADAPTER_H

#define FAIL    -1
#define OK      0
#define BLK_START_ADDR  0
#define END_OF_FILE     0

typedef unsigned int    file_t;
typedef unsigned int    cursor_t;
typedef unsigned int    addr_t;
typedef unsigned char   blkData_t;

const int numOfBufBlock = 8;    // 缓冲区中block的数量
const int sizeOfAttr = 4;       // 一个属性值的长度
const int sizeOfRow = 8;        // 一条记录的长度
const int numOfRowInBlk = 7;    // 一个block中可容纳的记录数量

const int addrOfLastRow = numOfRowInBlk * sizeOfRow;    // 块中最后一条记录的地址
const int endOfBlock = numOfRowInBlk * sizeOfRow + 8;   // 块的末尾地址
const int MAX_ATTR_VAL = 10000; // 属性的最大值 + 1，可用于设置空记录

// 磁盘和内存的一些参数
const int ADDR_NOT_EXISTS = -1;
const int DEFAULT_ADDR = 0;
const int addrOfScan_1 = 100;   // 第一遍排序结果的起始地址
const int addrOfScan_2 = 200;   // 第二遍排序结果的起始地址

Buffer buff;   // 全局只定义一个缓冲区

/**
 * @brief 二元元组
 * 有两个属性A和B，并且根据要求定义了一些比较运算符
 * 由于二元组可以对应键值对，因此也可用作索引
 */
typedef struct Row {
    bool isFilled = false;
    int A, B;
    Row() {
        isFilled = false;
        A = MAX_ATTR_VAL, B = MAX_ATTR_VAL;
    }
    bool join_A(const Row &R) { return A == R.A; }
    bool operator>(const Row &R) { return A > R.A; }
    bool operator<(const Row &R) { return A < R.A; }
    bool operator==(const Row &R) { return ((A == R.A) && (B == R.B)); }
} row_t, index_t;

/**
 * @brief 缓冲区（映射层）
 * 在extmem的基础上再抽象一层，将读写、申请释放等操作封装了进去
 * 同时还在一定程度上增加了缓冲区管理功能
 */
class Block {
public:
    Block();
    void freeBlock();
    void loadFromDisk(addr_t addr, cursor_t endPos = addrOfLastRow);
    void writeInit(const file_t filename, int numOfRows = numOfRowInBlk);
    addr_t writeLastBlock();
    row_t getNewRow();
    addr_t writeRow(const row_t R);
    addr_t readNextAddr();

private:
    blkData_t *blkData;
    addr_t readBlkAddr, writeBlkAddr;
    cursor_t cursor;        // 缓冲区当前的读写位置
    cursor_t endAddrOfData; // 缓冲区中的最后一条记录的位置
    void _writeAddr(unsigned int nextAddr);
    void _writeToDisk(addr_t addr);
};

typedef Block block_t;

/**
 * @brief 记录表的一些基本信息
 */
typedef struct Table {
    addr_t start;   // 表的开始磁盘地址
    addr_t end;     // 表的结束磁盘地址
    int size;       // 表的记录总数量
    int rowSize;    // 表的记录长度
    Table(addr_t newStart = 0, int newSize = 0, addr_t newEnd = 0, int newRowSize = sizeOfRow): 
        start(newStart), end(newEnd), size(newSize), rowSize(newRowSize)
    {}
} table_t;

#endif // !ADAPTER_H
