// ESPHackingTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <Windows.h>
#include <sstream>
#include <math.h>
#include <iostream>
#include "HackProcess.h"
#include "pch.h"

//引用标准库的命名空间
using namespace std;

//黑客程序配置器
CHackProcess fProcess;

//屏幕dpi缩放值
float ScreenDPIScale = 1.5f;

//定义输入按键的值
#define F6_Key 0x75
#define RIGHT_MOUSE 0x02

//存储数量的玩家并定期更新，以了解我们正在处理多少敌人
int NumOfPlayers = 32;

//CS起源中常用的静态基址
const DWORD dw_PlayerCountOffs = 0x5EF6BC;//Engine.dll
const DWORD Player_Base = 0x4C6708;
const DWORD dwClientId = 0x4c;
const DWORD dw_Health = 0x94;//client
const DWORD dw_mTeamOffset = 0x9C;//client
const DWORD dw_Pos = 0x260;//client
const DWORD dwAngle = 0x26c;
const DWORD EntityPlayer_Base = 0x4D3904; //entitylist
const DWORD EntityLoopDistance = 0x10;
const DWORD dw_Jump = 0x4F3B3C; //relative to client.dll
const DWORD dw_JumpOffset = 0x350;
const DWORD dw_crosshairOffs = 0x14F0;//Client.dll //

//需要设置的初始化变量
HDC HDC_Desktop;
//windef库定义的笔刷对象，用于绘图
HBRUSH EnemyBrush; //Brush to paint ESP etc
//windef库定义的文字对象，用于绘画文字
HFONT Font;
//目标窗口的矩形对象
RECT m_Rect;

//玩家矩阵的静态地址（指针地址）
const DWORD dw_vMatrix = 0x5B3BF0;

//闪烁检测的指针地址
//const DWORD dw_antiFlick = 0x5916B8; //2018弃用

//目标窗口
HWND TargetWnd;
//目标窗口代理
HWND Handle;

DWORD DwProcId;

//线的颜色
COLORREF SnapLineCOLOR;

//文本的颜色
COLORREF TextCOLOR;

//世界空间到屏幕矩阵的结构体
typedef struct
{
	float flMatrix[4][4];
}WorldToScreenMatrix_t;

//获取本体和敌人的距离(我的坐标，敌人坐标)
float Get3dDistance(float * myCoords, float * enemyCoords)
{
	//通过xyz轴相减，调用sqrt函数获得距离
	return sqrt(
		pow(double(enemyCoords[0] - myCoords[0]), 2.0) +
		pow(double(enemyCoords[1] - myCoords[1]), 2.0) +
		pow(double(enemyCoords[2] - myCoords[2]), 2.0)
	);
}


//设置绘画
void  SetupDrawing(HDC hDesktop,HWND handle)
{
	//目标桌面
	HDC_Desktop = hDesktop;
	//代理
	Handle = handle;
	//创建一个正方体笔刷，颜色为红色
	EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
	//设置线的颜色为蓝色
	SnapLineCOLOR = RGB(0, 0, 255);
	//设置文本的颜色为绿色
	TextCOLOR = RGB(0, 255, 0);
}


//玩家本体的结构体
struct MyPlayer_t
{
	//LocalPlayer 指针地址
	DWORD CLocalPlayer;
	//所在队伍
	int Team;
	//生命值
	int Health;
	//本体玩家坐标
	float Position[3];

	//本体玩家世界空间到屏幕矩阵的结构体
	WorldToScreenMatrix_t WorldToScreenMatrix;

	//闪烁检测
	//int flickerCheck; //2018弃用

	//读取本体玩家信息
	void ReadInformation()
	{
		// 根据DLL的指针地址，加上本地玩家的PlayerBase指针地址（偏移值），得出当前在内存的地址（动态地址），赋值到CLocalPlayer
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + Player_Base), &CLocalPlayer, sizeof(DWORD), 0);

		//根据CLocalPlayer的指针地址，加上dw_mTeamOffset的偏移值，得到所在团队的值
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CLocalPlayer + dw_mTeamOffset), &Team, sizeof(int), 0);

		//根据CLocalPlayer的指针地址，加上dw_Health的偏移值，得到生命值
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CLocalPlayer + dw_Health), &Health, sizeof(int), 0);

		//根据CLocalPlayer的指针地址，加上dw_Pos的偏移值，得到本体玩家的世界坐标
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CLocalPlayer + dw_Pos), &Position, sizeof(float[3]), 0);

		// 根据DLL的指针地址，加上玩家总数的PlayerBase指针地址（偏移值），得出当前在内存的地址（动态地址），赋值到玩家总数
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + dw_PlayerCountOffs), 
			&NumOfPlayers, sizeof(int), 0);

		// 根据DLL的指针地址，加上屏幕矩阵的dw_vMatrix指针地址（偏移值），得出当前在内存的地址（动态地址），赋值到闪烁检测
		//ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + dw_antiFlick), &flickerCheck, sizeof(DWORD), 0); //2018弃用

		//是否闪烁，0为无，1为有
		//if (flickerCheck == 0) //2018弃用
		{
			// 根据DLL的指针地址，加上屏幕矩阵的dw_vMatrix指针地址（偏移值），得出当前在内存的地址（动态地址），赋值到屏幕矩阵的结构体
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + dw_vMatrix), &WorldToScreenMatrix, sizeof(WorldToScreenMatrix), 0);
		}

		//-1A4 = ANTI FLICKER	
		//Engine.dll+0x5B3BF0

	}
}MyPlayer;



//保存玩家对象的结构体。所有玩家对象的集合
struct PlayerList_t
{
	//BaseEntity指针地址
	DWORD CBaseEntity;
	int Team;
	int Health;
	float Position[3];
	float AimbotAngle[3];
	char Name[39];

	void ReadInformation(int Player)
	{
		// 根据DLL的指针地址，加上实体玩家的EntityPlayer_Base指针地址（偏移值），得出当前在内存的地址（动态地址），赋值到CBaseEntity
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + EntityPlayer_Base + (Player * EntityLoopDistance)), &CBaseEntity, sizeof(DWORD), 0);

		//根据CBaseEntity的指针地址，加上dw_mTeamOffset的偏移值，得到所在团队的值
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CBaseEntity + dw_mTeamOffset), &Team, sizeof(int), 0);

		//根据CBaseEntity的指针地址，加上dw_Health的偏移值，得到生命值
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CBaseEntity + dw_Health), &Health, sizeof(int), 0);

		//根据CBaseEntity的指针地址，加上dw_Pos的偏移值，得到当前对象玩家的世界坐标
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(CBaseEntity + dw_Pos), &Position, sizeof(float[3]), 0);
	}
}PlayerList[32];

//通过世界坐标计算出目标
bool WorldToScreen(float * from, float * to)
{
	//玩家与目标在屏幕上的距离
	float w = 0.0f;
	//根据目标在世界空间的xyz轴，得出在该目标在矩阵中的x轴
	to[0] = MyPlayer.WorldToScreenMatrix.flMatrix[0][0] * from[0] + MyPlayer.WorldToScreenMatrix.flMatrix[0][1] * from[1] + MyPlayer.WorldToScreenMatrix.flMatrix[0][2] * from[2] + MyPlayer.WorldToScreenMatrix.flMatrix[0][3];
	//根据目标在世界空间的xyz轴，得出在该目标在矩阵中的y轴
	to[1] = MyPlayer.WorldToScreenMatrix.flMatrix[1][0] * from[0] + MyPlayer.WorldToScreenMatrix.flMatrix[1][1] * from[1] + MyPlayer.WorldToScreenMatrix.flMatrix[1][2] * from[2] + MyPlayer.WorldToScreenMatrix.flMatrix[1][3];

	//根据目标在世界空间的xyz轴，得出玩家与目标在屏幕上的距离
	w = MyPlayer.WorldToScreenMatrix.flMatrix[3][0] * from[0] + MyPlayer.WorldToScreenMatrix.flMatrix[3][1] * from[1] + MyPlayer.WorldToScreenMatrix.flMatrix[3][2] * from[2] + MyPlayer.WorldToScreenMatrix.flMatrix[3][3];

	//如果小于0.01返回false
	if (w < 0.01f)
		return false;
	//根据距离的缩放值
	float invw = 1.0f / w;
	//x轴乘以缩放值
	to[0] *= invw;
	//y轴乘以缩放值
	to[1] *= invw;

	//该程序窗口的宽度
	int width = (int)(m_Rect.right - m_Rect.left);
	//该程序窗口的高度
	int height = (int)(m_Rect.bottom - m_Rect.top);

	//得到中心点的x轴
	float x = width / 2;
	//得到中心点的y轴
	float y = height / 2;

	//计算出相对于程序窗口的x轴
	x += 0.5 * to[0] * width + 0.5;
	//计算出相对于程序窗口的y轴
	y -= 0.5 * to[1] * height + 0.5;

	//计算出绝对的x轴
	to[0] = x + m_Rect.left;
	//计算出绝对的y轴
	to[1] = y + m_Rect.top;

	//返回真
	return true;
}


//填充矩形
void DrawFilledRect(int x, int y, int w, int h)
{
	RECT rect = { x, y, x + w, y + h };
	FillRect(HDC_Desktop, &rect, EnemyBrush);
}

//绘画目标的边框
void DrawBorderBox(int x, int y, int w, int h, int thickness)
{
	//标准代码
	DrawFilledRect(x, y, w, thickness); //上边框
	DrawFilledRect(x, y, thickness, h); //左边框
	DrawFilledRect((x + w), y, thickness, h); //右边框
	DrawFilledRect(x, y + h, w + thickness, thickness); //下边框
}

//绘画直线
void DrawLine(float StartX, float StartY, float EndX, float EndY, COLORREF Pen)
{
	//设置绘线的样式
	HPEN hNPen = CreatePen(PS_SOLID, 2, Pen);
	//配置到画笔对象
	HPEN hOPen = (HPEN)SelectObject(HDC_Desktop, hNPen);
	//设置起始点
	MoveToEx(HDC_Desktop, StartX, StartY, NULL); //start
	//设置画笔终点
	LineTo(HDC_Desktop, EndX, EndY); //end
	//删除对象
	DeleteObject(SelectObject(HDC_Desktop, hOPen));
}

//绘画距离字符串
void DrawString(int x, int y, COLORREF color, const char* text)
{
	//设置文本对齐方式
	SetTextAlign(HDC_Desktop, TA_CENTER | TA_NOUPDATECP);
	//设置背景颜色
	SetBkColor(HDC_Desktop, RGB(0, 0, 0));
	//设置背景模式，透明
	SetBkMode(HDC_Desktop, TRANSPARENT);
	//设置文字颜色
	SetTextColor(HDC_Desktop, color);

	
	//选择对象
	SelectObject(HDC_Desktop, Font);
	//输出文本
	TextOutA(HDC_Desktop, x, y, text, strlen(text));
	//删除对象
	DeleteObject(Font);
}

//绘制透视，根据x，y轴。 和世界空间的距离
void DrawESP(int x, int y, float distance) 
{ 
	//边框宽度
	int width = 18100 / distance * ScreenDPIScale;
	//边框高度
	int height = 36000 / distance * ScreenDPIScale;

	//调用绘制边框函数
	DrawBorderBox(x - (width / 2), y - height, width, height, 1);

	//调用绘线函数
	DrawLine(
		(m_Rect.right - m_Rect.left) / 2,//窗口中心x轴
		m_Rect.bottom - m_Rect.top,//窗口底部y轴
		x, y //目标的xy坐标
		, SnapLineCOLOR); //颜色
	
	//字符串流对象
	std::stringstream ss;
	//移位赋值
	ss << (int)distance;

	//初始化字符数组对象
	char * distanceInfo = new char[ss.str().size() + 1];
	//把字符串流拷贝到字符数组
	strcpy(distanceInfo, ss.str().c_str());

	//调用绘制文字函数
	DrawString(x, y, TextCOLOR, distanceInfo);

	//删除数组
	delete[] distanceInfo;
}

//执行透视
void ESP() 
{
	//通过窗口，获取矩形对象
	GetWindowRect(FindWindow(NULL, "Counter-Strike Source"), &m_Rect);

	//屏幕矩形根据dpi进行缩放
	m_Rect.left *= ScreenDPIScale;
	m_Rect.right *= ScreenDPIScale;
	m_Rect.top *= ScreenDPIScale;
	m_Rect.bottom *= ScreenDPIScale;

	//遍历所有玩家对象
	for (int i = 1; i < NumOfPlayers; i++) 
	{
		//调用读取信息函数
		PlayerList[i].ReadInformation(i);

		//如果该玩家生命值小于2，跳到下一次循环
		if (PlayerList[i].Health < 2)
			continue;

		//如果该玩家是队友，跳到下一次循环
		if (PlayerList[i].Team == MyPlayer.Team)
			continue;

		//敌人在屏幕的xy轴
		float EnemyXY[3];

		//判断是否应该绘制ESP
		if (WorldToScreen(PlayerList[i].Position, EnemyXY))
		{
			//调用绘制透视函数
			DrawESP(EnemyXY[0] - m_Rect.left, EnemyXY[1] - m_Rect.top, Get3dDistance(MyPlayer.Position, PlayerList[i].Position));
		}
	}

}

//Main函数入口
int main()
{
	system("mode con cols=30 lines=20");//改变宽高
	//控制台输出
	cout << "欢迎打开CS起源外挂！！！";
	//启动黑客程序配置器
	fProcess.RunProcess();

	//ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);

	//为目标窗体赋值
	TargetWnd = FindWindow(0, "Counter-Strike Source");
	//获得目标桌面
	HDC HDC_Desktop = GetDC(TargetWnd);
	//设置绘画
	SetupDrawing(HDC_Desktop, TargetWnd);

	//设置无限循环
	for (;;)
	{
		//读取本体玩家信息
		MyPlayer.ReadInformation();

		//执行透视
		ESP();
	}

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
