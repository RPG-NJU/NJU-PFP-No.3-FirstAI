
#include "Reversi.h"

#define random(x) (rand()%x)
#define ROWS 8
#define COLS 8
#define ROUNDS 10

#define BOARD_TEST 1//1的时候可以在客户端显示棋盘的具体情况
#define SAVE 1//1的时候进行棋盘的储存

//下面需要给出自己的静态权重
#define POWER_CORNER 4
#define POWER_NEAR_CORNER -4
#define POWER_TWO_NEAR_CORNER -2
#define POWER_TWO_TO_CORNER 2
//终了

//需要标记所有的静态权重点
const int corner[4][2] = { {0,0},{0,7},{7,0},{7,7} };//这是角落的四个点
const int near_corner[12][2] = { {0,1},{0,6},{1,0},{1,1},{1,6},{1,7},{6,0},{6,1},{6,6},{6,7},{7,1},{7,6} };//这是包围角落的四个角，最不该下的点
const int two_to_corner[12][2] = { {0,2},{0,5},{2,0},{2,2},{2,5},{2,7},{5,0},{5,2},{5,5},{5,7},{7,2},{7,5} };//这里是角落辐射出去的隔一个格子的点，四角外次重要的点
const int two_near_corner[8][2] = { {1,2},{1,5},{2,1},{2,6},{5,1},{5,6},{6,2},{6,5} };
//以上的权重的建立方式有可能被下面的版本1.1的所取代


Reversi::Reversi()
{
	client_socket = ClientSocket();
	oppositeColor = ownColor = -1;
	QueryPerformanceFrequency(&TIME);
	now_row = -1;
	now_col = -1;//作为没有禁手的标志

	//可能需要对于存储的文件进行名字的初始化，最好有可辨识度
#if SAVE == 1
	string chess_data_filename_build;
	time_t t;
	time(&t);
	struct tm *p;
	p = localtime(&t);
	char mid[100];
	//cout << p->tm_year << endl;
	//chess_data_filename_build = Data_File_Head;
	itoa(p->tm_year + 1900, mid, 10);
	chess_data_filename_build = mid;
	chess_data_filename_build += "年";
	itoa(p->tm_mon + 1, mid, 10);
	chess_data_filename_build += mid;
	chess_data_filename_build += "月";
	//cout << chess_data_filename_build << endl;
	itoa(p->tm_mday, mid, 10);
	chess_data_filename_build += mid;
	chess_data_filename_build += "日";
	itoa(p->tm_hour, mid, 10);
	chess_data_filename_build += mid;
	chess_data_filename_build += "时";
	itoa(p->tm_min, mid, 10);
	chess_data_filename_build += mid;
	chess_data_filename_build += "分";
	//cout << p->tm_sec << endl;
	itoa(p->tm_sec, mid, 10);
	chess_data_filename_build += mid;
	chess_data_filename_build += "秒";
	chess_data_filename_build += ".txt";
	//这个时候的文件名已经确认
	//strcpy_s(chess_data_filename, 100, chess_data_filename_build.c_str());//将文件名传递
	chess_data_filename = chess_data_filename_build;
	cout << "--------------------------->棋盘相关文件名为" << chess_data_filename << endl;

	//进行文件的打开
	if (fopen_s(&file, (Data_File_Head + chess_data_filename).c_str(), "w+"))
		//if (fopen_s(&file, "why.txt", "w+"))
	{
		cout << chess_data_filename << endl;
		cout << "--------------------------->棋盘保存文件打开失败" << endl;
		//cout << fopen_s(&file, chess_data_filename, "w+") << endl;
		exit(-1);//后期可以改成其他的容错率较高的结束方法
	}
	else
	{
		cout << "--------------------------->棋盘保存文件打开成功" << endl;
	}
	//文件打开终了
#endif
}

Reversi::~Reversi()
{
#if SAVE == 1
	fclose(file);//在程序结束的时候关闭文件夹
#endif
}

/*
 send id and password to server by socket
 rtn != 0 represents socket transfer error
 */
void Reversi::authorize(const char *id, const char *pass)
{
	client_socket.connectServer();
	std::cout << "Authorize " << id << std::endl;
	char msgBuf[BUFSIZE];
	memset(msgBuf, 0, BUFSIZE);
	msgBuf[0] = 'A';
	memcpy(&msgBuf[1], id, 9);
	memcpy(&msgBuf[10], pass, 6);
	int rtn = client_socket.sendMsg(msgBuf);
	if (rtn != 0) printf("Authorized Failed!\n");
}

/* user input id and password that should match id and password in file in ReversiServer "players-X.txt"
 Where X should be the maximum number of the numbers.
*/
void Reversi::gameStart()
{
	char id[12] = { 0 }, passwd[10] = { 0 };
	//char id[12] = "111111110", passwd[10] = "123456";
	printf("ID: %s\n", id);
	scanf("%s", id);
	printf("PASSWD: %s\n", passwd);
	scanf("%s", passwd);

	authorize(id, passwd);

	printf("Game Start!\n");

	for (int round = 0; round < ROUNDS; round++) {
		roundStart(round);
		oneRound();
		roundOver(round);
	}
	gameOver();
	client_socket.close();
}

void Reversi::gameOver()
{
	printf("Game Over!\n");
}

/* send a message, lazi in position (row, col)
 receive 2 message:
 first message is the answer to your step.
 second message is the step of opponent player.
*/
void Reversi::roundStart(int round)
{
	printf("Round %d Ready Start!\n", round);

	// first time receive msg from server
	int rtn = client_socket.recvMsg();
	if (rtn != 0) return;
	if (strlen(client_socket.getRecvMsg()) < 2)
		printf("Authorize Failed!\n");
	else
		printf("Round start received msg %s\n", client_socket.getRecvMsg());
	switch (client_socket.getRecvMsg()[1]) {
		// this client : black chessman
	case 'B':
		ownColor = 0;
		oppositeColor = 1;
		rtn = client_socket.sendMsg("BB");
		printf("Send BB -> rtn: %d\n", rtn);
		if (rtn != 0) return;
		break;
	case 'W':
		ownColor = 1;
		oppositeColor = 0;
		rtn = client_socket.sendMsg("BW");
		printf("Send BW -> rtn: %d\n", rtn);
		if (rtn != 0) return;
		break;
	default:
		printf("Authorized Failed!\n");
		break;
	}

	//chess board init
	initChessBoard();
}

void Reversi::oneRound()
{
	int STEP = 1;
	//cout << "?";
	//this->saveChessBoard();
	switch (ownColor) {
	case 0:
		while (STEP < 10000) {

			pair<int, int> chess = step();                        // take action, send message

			// lazi only excute after server's message confirm  in observe function
			generateOneStepMessage(chess.first, chess.second);


			if (observe() >= 1) break;     // receive RET Code

			if (observe() >= 1) break;    // see white move
			STEP++;
			// saveChessBoard();
		}
		printf("One Round End\n");
		break;
	case 1:
		while (STEP < 10000) {

			if (observe() >= 1) break;    // see black move

			pair<int, int> chess = step();                        // take action, send message
			// lazi only excute after server's message confirm  in observe function
			generateOneStepMessage(chess.first, chess.second);


			if (observe() >= 1) break;     // receive RET Code
			// saveChessBoard();
			STEP++;
		}
		printf("One Round End\n");
		break;

	default:
		break;
	}
}

void Reversi::roundOver(int round)
{
	printf("Round %d Over!\n", round);
	// reset initializer

	ownColor = oppositeColor = -1;
}

int Reversi::observe()
{
#if SAVE == 1
	this->saveChessBoard();//每一步就进行一次存储棋盘
#endif

	int rtn = 0;
	int recvrtn = client_socket.recvMsg();
	if (recvrtn != 0) return 1;
	printf("receive msg %s\n", client_socket.getRecvMsg());
	switch (client_socket.getRecvMsg()[0]) {
	case 'R':
	{
		switch (client_socket.getRecvMsg()[1]) {
		case 'Y':   // valid step
			switch (client_socket.getRecvMsg()[2]) {
			case 'P':   // update chessboard
			{
				int desRow = (client_socket.getRecvMsg()[3] - '0') * 10 + client_socket.getRecvMsg()[4] - '0';
				int desCol = (client_socket.getRecvMsg()[5] - '0') * 10 + client_socket.getRecvMsg()[6] - '0';
				int color = (client_socket.getRecvMsg()[7] - '0');
				//use handleMessage to handle the position (desRow , desCol)
				handleMessage(desRow, desCol, color);

				printf("a valid step of : (%d %d)\n", desRow, desCol);
				break;
			}
			case 'N':   // R0N: enemy wrong step
			{
				int desRow = -1, desCol = -1;
				int color = (client_socket.getRecvMsg()[3] - '0');
				handleMessage(desRow, desCol, color);

				//
				printf("a true judgement of no step\n");
				break;
			}
			}

			break;
		case 'W':
			// invalid step
			switch (client_socket.getRecvMsg()[2]) {
			case 'P': {
				int desRow = (client_socket.getRecvMsg()[3] - '0') * 10 + client_socket.getRecvMsg()[4] - '0';
				int desCol = (client_socket.getRecvMsg()[5] - '0') * 10 + client_socket.getRecvMsg()[6] - '0';

				int color = (client_socket.getRecvMsg()[7] - '0');
				printf("Invalid step , server random a true step of : (%d %d)\n", desRow, desCol);
				//use handleMessage to handle the position (desRow , desCol)
				handleMessage(desRow, desCol, color);
				break;
			}
			case 'N': {
				int desRow = -1, desCol = -1;
				int color = (client_socket.getRecvMsg()[3] - '0');
				handleMessage(desRow, desCol, color);
				printf("a wrong judgement of no step\n");
				break;
			}
			default:
				break;
			}
			break;

		default:

			printf("Error : Other error!\n");
			rtn = -5;
			break;
		}
		break;
	}
	case 'E':
	{
		switch (client_socket.getRecvMsg()[1]) {
		case '0':
			// game over
			rtn = 2;
			break;
		case '1':
			// round over
			rtn = 1;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return rtn;
}

void Reversi::generateOneStepMessage(int row, int col)
{
	char msg[BUFSIZE];
	memset(msg, 0, sizeof(msg));


	//put row and col in the message
	msg[0] = 'S';
	msg[1] = 'P';
	msg[2] = '0' + row / 10;
	msg[3] = '0' + row % 10;
	msg[4] = '0' + col / 10;
	msg[5] = '0' + col % 10;
	msg[6] = '\0';
	if (row < 0 || col < 0) {
		row = -1;
		col = -1;
	}
	if (row == -1 && col == -1) {
		msg[2] = '-';
		msg[3] = '1';
		msg[4] = '-';
		msg[5] = '1';
	}
	//print
	printf("generate one step at possition (%2d,%2d) : %s\n", row, col, msg);


	client_socket.sendMsg(msg);
}

/*-------------------------last three function--------------------------------
 * step : find a good position to lazi your chess.
 * saveChessBoard : save the chess board now.
 * handleMessage: handle the message from server.
 */



void Reversi::saveChessBoard()
{
	cout << "--------------------------->saveChessBoard调用" << endl;
	//存储在CHESS DATA文件夹下
	//需要决定有辨识度的文件名进行储存，目前把这个步骤放在类构造中
	//拟将文件的打开放在构造函数中
	//下面进行文件的写入
	if (ownColor == 0)//己方执子黑色
		fprintf_s(file, "%s", "己方执子●，对方执子○\n");
	else
		fprintf_s(file, "%s", "己方执子○，对方执子●\n");
	//增加了一条来进行执子的表示
	fprintf_s(file, "%s", "  0 1 2 3 4 5 6 7\n");
	for (int i = 0; i < 8; i++)
	{
		fprintf_s(file, "%d", i);
		for (int j = 0; j < 8; j++)
		{
			if (chessboard[i][j] == 0)
				fprintf_s(file, "%s", "  ");//两个空格才等大
			else if (chessboard[i][j] == 1)
				fprintf_s(file, "%s", "●");
			else if (chessboard[i][j] == 2)
				fprintf_s(file, "%s", "○");
			else
				fprintf_s(file, "%s", "X");
		}
		fprintf_s(file, "%s", "\n");
	}
	fprintf_s(file, "%s", "\n");
	fprintf_s(file, "%s", "\n");
	return;
}

void Reversi::handleMessage(int row, int col, int color)
{
	now_row = row;
	now_col = col;
	if (color == ownColor)
		if (ownColor == 0)
			cout << "--------------------------->己方落●子" << "(" << row << "," << col << ")" << endl;
		else
			cout << "--------------------------->己方落○子" << "(" << row << "," << col << ")" << endl;
	else if (color == oppositeColor)
		if (oppositeColor == 0)
			cout << "--------------------------->对方落●子" << "(" << row << "," << col << ")" << endl;
		else
			cout << "--------------------------->对方落○子" << "(" << row << "," << col << ")" << endl;
	//上面的信息在客户端作为必要的显示

	LARGE_INTEGER begin, end;
	QueryPerformanceCounter(&begin);
	this->Flap(chessboard, row, col, color, true);//先进行翻转的操作
	QueryPerformanceCounter(&end);
	cout << "--------------------------->计算棋盘翻转耗时" << (double)(end.QuadPart - begin.QuadPart) / (double)TIME.QuadPart << "s" << " 当今棋盘为：" << endl;

	chessboard[row][col] = color + 1;//因为1是黑色，0是空白，2是白色
#if BOARD_TEST == 1
	this->PrintChessBoard(chessboard);
	this->Judge(chessboard);
#endif
	return;
}

//init your chess board here
void Reversi::initChessBoard()
{
	cout << "---------------->新一局游戏开始" << endl;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
			chessboard[i][j] = 0;
	}
	chessboard[3][3] = 1, chessboard[4][4] = 1;
	chessboard[3][4] = 2, chessboard[4][3] = 2;

	if (ownColor == 0)
		cout << "---------------->己方执黑子" << endl;
	else if (ownColor == 1)
		cout << "---------------->己方执白子" << endl;
	else
		cout << "---------------->执子相关错误" << endl;
	return;
}
void Reversi::PrintChessBoard(int x[][8]) const
{
	cout << "  0 1 2 3 4 5 6 7 " << endl;
	for (int i = 0; i < 8; i++)
	{
		cout << i;
		for (int j = 0; j < 8; j++)
		{
			if (x[i][j] == 0)
				cout << "  ";//两个空格才等大
			else if (x[i][j] == 1)
				cout << "●";
			else if (x[i][j] == 2)
				cout << "○";
			else
				cout << "X";
		}
		cout << endl;
	}
	return;
}

int Reversi::Flap(int board[][8], int row, int col, int color, bool mode)//这里的传址方式的确可以修改原来的数组
{
	if (row < 0 || col < 0)
	{
		cout << "--------------------------->无子可下没有翻转" << endl;
		return 0;
	}

	int flap_num = 0;//作为翻子个数的记录
	//以新的点为中心，对周围进行八方向的搜索遍历，直到本方棋子或者边界
	//(row,col)是这个落子的地点

	board[row][col] = 10;//先暂时假定落子点有，但是不标记为任意一个有效颜色，只是作为非空进行记录
	int i = row, j = col;//从中心开始延展
	bool flag = false;
	//向上
	for (; i >= 0; i--)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i < row; i++)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向下
	for (; i < 8; i++)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i > row; i--)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向左
	for (; j >= 0; j--)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; j < col; j++)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向右
	for (; j < 8; j++)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; j > col; j--)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向左上
	for (; i >= 0 && j >= 0; i--, j--)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i < row && j < col; i++, j++)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向右上
	for (; i >= 0 && j < 8; i--, j++)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i < row && j > col; i++, j--)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向左下
	for (; i < 8 && j >= 0; i++, j--)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i > row && j < col; i--, j++)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	i = row, j = col;
	flag = false;
	//向右下
	for (; i < 8 && j < 8; i++, j++)
	{
		if (board[i][j] == color + 1)
		{
			flag = true;
			break;
		}
		else if (board[i][j] == 0)
			break;
	}
	if (flag)
	{
		for (; i > row && j > col; i--, j--)
		{
			if (board[i][j] != color + 1)
			{
				if (mode)
					board[i][j] = color + 1;
				flap_num++;
			}
		}
	}

	board[row][col] = 0;//归还之前的假设存在
	return flap_num;
}