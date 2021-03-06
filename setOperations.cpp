#include "utils.cpp"
#include "distinct.cpp"
#pragma once


const addr_t setOperationResultStart = 4000;    // 集合操作结果的起始存放地址


/**
 * @brief 表的交操作
 * 采用嵌套循环的方法检索符合条件的记录
 * 
 * @param table1 第一个表的相关信息
 * @param table2 第二个表的相关信息
 * @param resTable 交操作结果存放区域的信息表
 */
void tablesIntersect(table_t table1, table_t table2, table_t &resTable) {
    table_t bigTable, smallTable;
    if (table1.size > table2.size) {
        bigTable = table1;
        smallTable = table2;
    } else {
        bigTable = table2;
        smallTable = table1;
    }
    int numOfSeriesBlk = 6;
    int numOfRows_1 = numOfRowInBlk * numOfSeriesBlk, numOfRows_2 = numOfRowInBlk;
    addr_t readAddr_1 = smallTable.start, readAddr_2 = bigTable.start;
    addr_t curAddr = 0, resAddr = resTable.start;
    
    block_t seriesBlk[numOfSeriesBlk], singleBlk, resBlk;
    for (int i = 0; i < numOfSeriesBlk; ++i)
        seriesBlk[i].loadFromDisk(readAddr_1++);
    resBlk.writeInit(resAddr);
    row_t t1[numOfRows_1], t2[numOfRows_2];

    int readRows_1, readRows_2;
    while(1) {
        // 每次从小表中读入6块
        readRows_1 = read_N_Rows_From_M_Block(seriesBlk, t1, numOfRows_1, numOfSeriesBlk);
        // printRows(t, readRows, val);
        singleBlk.loadFromDisk(readAddr_2);
        while(1) {
            // 每次从大表中读入1块进行比较
            readRows_2 = read_N_Rows_From_1_Block(singleBlk, t2, numOfRows_2);
            for (int i = 0; i < readRows_1; ++i) {
                for (int j = 0; j < readRows_2; ++j) {
                    if (t1[i] == t2[j]) {
                        curAddr = resBlk.writeRow(t1[i]);
                        resTable.size += 1;
                    }
                }
            }
            if (readRows_2 < numOfRows_2)
                break;
        }
        if (readRows_1 < numOfRows_1) {
            addr_t endAddr = resBlk.writeLastBlock();
            if (endAddr != END_OF_FILE)
                curAddr = endAddr;
            resTable.end = curAddr;
            break;
        }
    }
    // 处理空结果表
    if (resTable.size == 0)
        resTable.start = resTable.end = 0;
}


/**
 * @brief 表的差操作
 * 采用嵌套循环的方法检索符合条件的记录
 * 
 * @param diffedTable 被差表的相关信息
 * @param diffTable 差表的相关信息
 * @param resTable 差操作结果存放区域的信息表
 */
void tablesDiff(table_t diffedTable, table_t diffTable, table_t &resTable) {
    int numOfSeriesBlock = 6;
    int numOfRows_1 = numOfRowInBlk * numOfSeriesBlock, numOfRows_2 = numOfRowInBlk;
    addr_t diffedTableAddr = diffedTable.start, diffTableAddr = diffTable.start;
    addr_t curAddr = 0;
    
    block_t seriesBlk[numOfSeriesBlock], singleBlk, resBlk;
    resBlk.writeInit(resTable.start);
    for (int i = 0; i < numOfSeriesBlock; ++i)
        seriesBlk[i].loadFromDisk(diffedTableAddr++);
    row_t tDiffed[numOfRows_1], tDiff[numOfRows_2];

    int readRows_1, readRows_2;
    while(1) {
        // 每次从大表中读入6块
        readRows_1 = read_N_Rows_From_M_Block(seriesBlk, tDiffed, numOfRows_1, numOfSeriesBlock);
        // insertSort<row_t>(tDiffed, readRows_1);
        // printRows(tDiffed, readRows_1);
        for (int i = 0; i < readRows_1; ++i) {
            bool isAppeared = false;
            singleBlk.loadFromDisk(diffTableAddr);
            while(1) {
                // 每次从小表中读入1块进行比较
                readRows_2 = read_N_Rows_From_1_Block(singleBlk, tDiff, numOfRows_2);
                for (int j = 0; j < readRows_2; ++j) {
                    if (tDiff[j] == tDiffed[i]) {
                        isAppeared = true;
                        break;
                    }
                }
                if (readRows_2 < numOfRows_2 || isAppeared) {
                    // 被差表读完了或者在被差表中遇到了一样的记录，就停止遍历
                    if (isAppeared)
                        singleBlk.freeBlock();
                    break;
                }
            }
            // 如果在差表中没有发现该被差表中的记录，则说明该记录是差操作的结果
            if (!isAppeared) {
                curAddr = resBlk.writeRow(tDiffed[i]);
                resTable.size += 1;
            }
        }
        if (readRows_1 < numOfRows_1) {
            // 大表读完了
            addr_t endAddr = resBlk.writeLastBlock();
            if (endAddr != END_OF_FILE)
                curAddr = endAddr;
            resTable.end = curAddr;
            break;
        }
    }
    // 处理空结果表
    if (resTable.size == 0)
        resTable.start = resTable.end = 0;
}


/**
 * @brief 表的并操作
 * 
 * @param table1 第一个表的相关信息
 * @param table2 第二个表的相关信息
 * @param resTable 并操作结果存放区域的信息表
 */
void tablesUnion(table_t table1, table_t table2, table_t &resTable) {
    table_t bigTable, smallTable;
    table_t diffTable(6000);
    if (table1.size > table2.size) {
        bigTable = table1;
        smallTable = table2;
    } else {
        bigTable = table2;
        smallTable = table1;  
    }
    tablesDiff(smallTable, bigTable, diffTable);
    row_t r, emptyRow;
    block_t blk, resBlk;
    resBlk.writeInit(resTable.start);
    addr_t tableAddr[2] = {diffTable.start, bigTable.start};

    for (int i = 0; i < 2; ++i) {
        blk.loadFromDisk(tableAddr[i]);
        while(1) {
            r = blk.getNewRow();
            if (r == emptyRow) {
                blk.freeBlock();
                break;
            }
            resBlk.writeRow(r);
            resTable.size += 1;
        }
    }
    DropFiles(diffTable.start);
    addr_t endAddr = resBlk.writeLastBlock();
    resTable.end = endAddr;
}


/**************************** main ****************************/
// int main() {
//     bufferInit();
//     clear_Buff_IO_Count();
//     table_t res(setOperationResultStart);
//     tablesUnion(table_R, table_S, res);
//     // table_t res = tablesIntersect(table_R, table_S, res);
//     // table_t res = tablesDiff(table_R, table_S, res);
//     // table_t res = tablesDiff(table_S, table_R, res);
//     showResult(res.start);
//     int numOfUsedBlocks = ceil(1.0 * res.size / (numOfRowInBlk));
//     printf("\n注：结果写入起始磁盘块：4000-%d\n", res.start + numOfUsedBlocks - 1);
//     printf("本次共发生%ld次I/O\n\n", buff.numIO);

//     system("pause");
//     return OK;
// }