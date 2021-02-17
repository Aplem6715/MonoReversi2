/**
 * @file bench.cpp
 * @author Daichi Sato
 * @brief 探索ベンチマーク機能の定義
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 */

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "bench.hpp"
#include "../board.h"
#include "../game.h"
#include "../bit_operation.h"

using namespace std;

void LoadGameRecords(const char *file, vector<vector<uint8>> &records)
{
    ifstream infile(file);
    string str;

    if (infile.fail())
    {
        fprintf(stderr, "Failed to open file.");
        return;
    }

    vector<uint8> moves;
    while (getline(infile, str))
    {
        moves.clear();
        for (size_t i = 0; i < str.length(); i += 2)
        {
            moves.push_back(PosIndexFromAscii(str.substr(i, 2).c_str()));
        }
        records.push_back(moves);
    }
}

void MakeBench(int nbGames, uint8 nbRandomTurn, string benchFile)
{
    Board board[1];
    uint8 pos;
    uint8 turn;
    char xAscii;
    int y;
    uint8 moves[60];

    ofstream outfile(benchFile, ios::app);

    for (int i = 0; i < nbGames; i++)
    {
        BoardReset(board);
        for (turn = 0; turn < nbRandomTurn; turn++)
        {
            pos = BoardGetRandomPosMoveable(board);
            if (pos == 0)
            {
                break;
            }
            BoardPutTT(board, pos);
            moves[turn] = pos;
        }

        if (pos == 0)
        {
            i--;
            continue;
        }

        for (turn = 0; turn < nbRandomTurn; turn++)
        {
            CalcPosAscii(moves[turn], &xAscii, &y);
            outfile << xAscii << y;
        }

        outfile << "\n";
    }
}

void Bench1Game(SearchTree &tree, vector<uint8> moves, int nbPut, ofstream &logfile)
{
    Board board[1];
    uint8 pos;
    char xAscii;
    int y;

    BoardReset(board);

    for (uint8 move : moves)
    {
        BoardPutTT(board, move);
    }

    BoardDraw(board);
    for (int i = 0; i < nbPut; i++)
    {
        ResetTree(&tree);
        if (BoardIsFinished(board))
        {
            break;
        }
        else if (BoardGetMobility(board) == 0)
        {
            BoardSkip(board);
        }
        pos = Search(&tree, BoardGetOwn(board), BoardGetOpp(board), 0);
        BoardPutTT(board, pos);
        BoardDraw(board);
        CalcPosAscii(pos, &xAscii, &y);

        {
            logfile << (int)tree.depth << ","
                    << tree.usedTime << ","
                    << tree.nodeCount << ","
                    << tree.nodeCount / tree.usedTime << ","
                    << tree.nbCut << ",";
        }
        if (tree.useHash)
        {
            logfile << tree.nwsTable->nbUsed << ","
                    << tree.nwsTable->nbHit << ","
                    << tree.nwsTable->nb2ndUsed << ","
                    << tree.nwsTable->nb2ndHit << ","
                    << tree.nwsTable->nbCollide << ",";

            logfile << tree.pvTable->nbUsed << ","
                    << tree.pvTable->nbHit << ","
                    << tree.pvTable->nb2ndUsed << ","
                    << tree.pvTable->nb2ndHit << ","
                    << tree.pvTable->nbCollide << ",";
        }
        else
        {
            logfile << ",,,";
        }
        {
            logfile << tree.score / (float)(STONE_VALUE) << ","
                    << xAscii << y << "\n";
        }
    }
}

void BenchSearching(vector<unsigned char> depths, unsigned char useHash, unsigned char useMPC, unsigned char nestMPC, unsigned char midPvsDepth, unsigned char endPvsDepth, string benchFile)
{
    SearchTree tree;
    vector<vector<uint8>> records;
    string logFileName;

    cout << "ベンチマーク: ログファイルのファイル名を入力してください\n";
    cin >> logFileName;

    ofstream logfile(BENCH_LOG_DIR + logFileName + ".csv");

    chrono::system_clock::time_point start, end;

    logfile.setf(ios::fixed, ios::floatfield);
    logfile.precision(2);
    logfile << "探索深度,思考時間,探索ノード数,探索速度,カット数,ハッシュ記録数,ハッシュヒット数,2ndハッシュ記録数,2ndハッシュヒット数,ハッシュ衝突数,pvハッシュ記録数,pvハッシュヒット数,pv2ndハッシュ記録数,pv2ndハッシュヒット数,pvハッシュ衝突数,推定CPUスコア,着手位置\n";
    LoadGameRecords(benchFile.c_str(), records);

    InitTree(&tree, 4, 4, midPvsDepth, endPvsDepth, useHash, useHash, useMPC, nestMPC);
    for (vector<uint8> moves : records)
    {
        for (uint8 move : moves)
        {
            char x;
            int y;
            CalcPosAscii(move, &x, &y);
            logfile << x << y;
        }
        logfile << "\n";
        for (unsigned char depth : depths)
        {
            ConfigTree(&tree, depth, depth);
            Bench1Game(tree, moves, 2, logfile);
        }
        logfile << "\n";
    }

    logfile.unsetf(ios::floatfield);
    logfile.close();
}