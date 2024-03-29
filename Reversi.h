//
//  Reversi.h
//  ReversiClient
//
//  Created by ganjun on 2018/3/6.
//  Copyright © 2018年 ganjun. All rights reserved.
//

#ifndef Reversi_h
#define Reversi_h
#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <cstring>
#include "ClientSocket.h"
#include "Define.h"

using namespace std;


//需要一个数据结构来承载这个算法
struct GRP
{
	GRP *before;
	int before_num;//作为是上一个指针的第几个的标志
	int x = -10000;
	int y = 10000;//作为α和β进行剪枝
	int board[8][8] = { 0 };
	int color;//作为下面的棋子颜色的标记
	vector<vector<int> > choice;
	vector<int> power;
	vector<GRP*> next;//这三个应该是等长的
};
//终了


class Reversi {
private:
	ClientSocket client_socket;
	int ownColor;
	int oppositeColor;

	//这里要添加一个二维数组来存贮变量棋盘
	int chessboard[8][8] = { 0 };
	int nowColor = 0;//作为目前应该是谁进行落子的标记
	LARGE_INTEGER TIME;

	int now_row;
	int now_col;//作为对方落子点的标记可以直接调用

	string Data_File_Head = ".\\CHESS DATA\\";
	string chess_data_filename;
	FILE *file;
	//function 
	void handleMessage(int row, int col, int color);

	// according to chessman position (row , col) , generate one step message in order to send to server
	void generateOneStepMessage(int row, int col);

	pair<int, int> step();

	void saveChessBoard();

	void initChessBoard();
public:
	Reversi();
	~Reversi();

	void authorize(const char *id, const char *pass);

	void gameStart();

	void gameOver();

	void roundStart(int round);

	void oneRound();

	void roundOver(int round);

	int observe();

	void PrintChessBoard(int x[][8]) const;

	int Flap(int board[][8], int row, int col, int color, bool mod);//mode代表是否进行原二维数组的修改，若要修改，则为true，反之不修改

	void Judge(const int board[][8]) const;

	void Build_Next_Point(GRP *now);

	GRP* Build_Tree(const int board[][8], int row, int col, int level, vector<GRP*> &tail);

	void Compute(vector<GRP*> tail, int level);
};

#endif /* Reversi_h */
