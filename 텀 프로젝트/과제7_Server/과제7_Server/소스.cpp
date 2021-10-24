#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <chrono>
#include <queue>
#include <sqlext.h>  
#include <random> 
#include <string>
#include <math.h>


#include <Windows.h>
#include <queue>
#include <vector>
#include <set>
#include <map>
#include <functional>



extern "C" {
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

#include "protocol.h"
using namespace std;
using namespace chrono;
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")

constexpr int MAX_BUFFER = 4096;

constexpr char OP_MODE_RECV = 0;
constexpr char OP_MODE_SEND = 1;
constexpr char OP_MODE_ACCEPT = 2;
constexpr char OP_RANDOM_MOVE = 3;
constexpr char OP_PLAYER_MOVE_NOTIFY = 4;
constexpr char OP_PLAYER_HEAL = 5;

constexpr int  KEY_SERVER = 1000000;

constexpr int NPC_TYPE_1 = 1;
constexpr int NPC_TYPE_2 = 2;
constexpr int NPC_TYPE_3 = 3;

void Load_Database();
void Update_Database(int id, int x, int y, int hp, int level, int exp);

default_random_engine dre{ 2016184004 };
uniform_int_distribution <> uid{ 0,5 };

int mapInfo[WORLD_WIDTH][WORLD_HEIGHT];

int revivemap[10][2] =
{
	{446,463},
	{415,415},
	{360,335},
	{305,266},
	{356,203},
	{479,413},
	{546,325},
	{540,259},
	{530,251},
	{540,188}
};


vector<POINT> AStar(int targetX, int targetY, int posx, int posy);
//MAP g_Map[WORLD_WIDTH][WORLD_HEIGHT];

using namespace std;

const char* lpszClass = "A Star";

struct Node
{
	POINT currentLocation;	// 현재 위치
	POINT prevLocation;		// 이전 위치
};

struct Huristic
{
	int movingDist;				// 현재까지 이동한 거리
	float predictionDist;	// 휴리스틱 가중치(노드와 도착지간의 직선거리)
};

// 이진트리에서 검색할때 사용하는 용도
bool operator < (const POINT& lhs, const POINT& rhs)
{
	if (lhs.x == rhs.x)
		return lhs.y < rhs.y;

	return lhs.x < rhs.x;
}

// 우선순위큐에서 heap구조를 만들 때 Node정보는 무시하게 하는용도로 연산자오버로딩
bool operator < (const Node& lhs, const Node& rhs)
{
	return false;
}

// 우선순위큐에서 가장 짧은경로가 될 노드임을 판단하기 위해서
// 현재까지 이동한거리 + 휴리스틱 가중치 값이 가장 작은것이 루트노드가 되게한다.
bool operator < (const Huristic& lhs, const Huristic& rhs)
{
	return (lhs.movingDist + lhs.predictionDist) < (rhs.movingDist + rhs.predictionDist);
}


POINT myPosition = { 0,0 };
POINT moving = { 0,0 };
//vector<POINT> vecPath;

int dx[] = { 0,-1,1,0 };
int dy[] = { -1,0,0,1 };

/////////////////////////////////////////////////////////////////////////




struct OVER_EX {
	WSAOVERLAPPED wsa_over;
	char	op_mode;
	WSABUF	wsa_buf;
	unsigned char iocp_buf[MAX_BUFFER];
	int		object_id;
};

struct client_info {
	mutex c_lock;
	char name[MAX_ID_LEN];
	short x, y;
	lua_State* L;
	mutex lua_l;
	mutex path_l;

	// 플레이어 정보들 //
	int DBKey;
	int hp;
	int exp;
	int level;

	// npc 관련 //
	int NPC_TYPE;

	client_info* target = nullptr;

	bool in_use;
	atomic_bool is_active;
	bool is_alive;
	//bool is_active;
	SOCKET	m_sock;
	OVER_EX	m_recv_over;
	unsigned char* m_packet_start;
	unsigned char* m_recv_start;

	mutex vl;
	unordered_set <int> view_list;

	int move_time;
};

atomic_int UserCount;

struct DATABASE {
	int user_id;
	char user_name[10];
	int user_exp;
	int user_level;
	int user_hp;
	int pos_x;
	int pos_y;
};

vector<DATABASE> vec_database;

mutex id_lock;
client_info g_clients[NUM_NPC];
HANDLE		h_iocp;

SOCKET g_lSocket;
OVER_EX g_accept_over;

struct event_type {
	int obj_id;
	system_clock::time_point wakeup_time;
	int event_id;
	int target_id;

	constexpr bool operator < (const event_type& _Left) const
	{
		return (wakeup_time > _Left.wakeup_time);
	}
};

priority_queue<event_type> timer_queue;
mutex timer_l;


void random_move_npc(int id, int i);
void send_chat_packet(int to_client, int id, char* mess, int mess_type);
void send_stat_change_packet(int to_client, int new_id);



void is_player_level_up(int user_id) 
{
	if (g_clients[user_id].exp >= (int)(100 * pow(2, (g_clients[user_id].level - 1))))
	{
		g_clients[user_id].exp = 0;
		g_clients[user_id].hp = 100;
		g_clients[user_id].level += 1;
	}
}

void is_npc_die(int user_id, int npc_id)
{

	if (g_clients[npc_id].hp < 0 && g_clients[npc_id].is_active == true)
	{
		if (g_clients[npc_id].is_alive == true) {
			g_clients[user_id].exp += (g_clients[npc_id].level * 10);   // 몬스터 레벨에 따라 획득하는 경험치가 달라짐
		}
		g_clients[npc_id].is_alive = false;
		g_clients[npc_id].is_active = false;
		//cout << "First" << g_clients[npc_id].is_active << endl;

		is_player_level_up(user_id);

		char mess[100];		
		sprintf_s(mess, "ID : %s  &&  EXP : %d  &&  LEVEL : %d", g_clients[user_id].name, g_clients[user_id].exp , g_clients[user_id].level);		
		
		for (int i = 0; i < UserCount; ++i) {
			if (i == user_id) {
				send_chat_packet(i, user_id, mess, 0);
			}
		}

		//g_clients[npc_id].hp = 0;

		send_stat_change_packet(user_id, npc_id);

		for (auto vl : g_clients[user_id].view_list)
			send_stat_change_packet(vl, npc_id);

		//cout << "Second" << g_clients[npc_id].is_active << endl;
	}
}

void add_timer(int obj_id, int ev_type, system_clock::time_point t)
{
	event_type ev{ obj_id, t, ev_type };
	timer_l.lock();
	timer_queue.push(ev);
	timer_l.unlock();
}

void time_worker()
{
	while (true) {
		while (true) {
			if (false == timer_queue.empty()) {
				event_type ev = timer_queue.top();
				if (ev.wakeup_time > system_clock::now()) break;
				timer_queue.pop();

				if (ev.event_id == OP_RANDOM_MOVE) {
					//random_move_npc(ev.obj_id);
					OVER_EX* ex_over = new OVER_EX;
					ex_over->op_mode = OP_RANDOM_MOVE;
					PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ex_over->wsa_over);
					//add_timer(ev.obj_id, OP_RANDOM_MOVE, system_clock::now() + 1s);
				}

				if (ev.event_id == OP_PLAYER_HEAL) {
					//random_move_npc(ev.obj_id);
					OVER_EX* ex_over = new OVER_EX;
					ex_over->op_mode = OP_PLAYER_HEAL;
					PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ex_over->wsa_over);
					//add_timer(ev.obj_id, OP_RANDOM_MOVE, system_clock::now() + 1s);
				}

			}
			else break;
		}
		this_thread::sleep_for(1ms);
	}
}

void wake_up_npc(int id)	// false 인애 동시에 true 안되게 막고 하나만 true로 바꾸게 한다.
							// 그리고 add_timer 하면서 랜덤으로 1초씩 움직이게 한다.
{
	bool b = false;
	if (true == g_clients[id].is_active.compare_exchange_strong(b, true) && true == g_clients[id].is_alive)
	{
		add_timer(id, OP_RANDOM_MOVE, system_clock::now() + 1s);
	}
}

void error_display(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러 " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}

bool is_npc(int p1)
{
	return p1 >= MAX_USER;
}

bool is_near(int p1, int p2)
{
	int dist = (g_clients[p1].x - g_clients[p2].x) * (g_clients[p1].x - g_clients[p2].x);
	dist += (g_clients[p1].y - g_clients[p2].y) * (g_clients[p1].y - g_clients[p2].y);

	return dist <= VIEW_LIMIT * VIEW_LIMIT;
}

void send_packet(int id, void* p)
{
	unsigned char* packet = reinterpret_cast<unsigned char*>(p);
	OVER_EX* send_over = new OVER_EX;
	memcpy(send_over->iocp_buf, packet, packet[0]);
	send_over->op_mode = OP_MODE_SEND;
	send_over->wsa_buf.buf = reinterpret_cast<CHAR*>(send_over->iocp_buf);
	send_over->wsa_buf.len = packet[0];
	ZeroMemory(&send_over->wsa_over, sizeof(send_over->wsa_over));
	g_clients[id].c_lock.lock();
	if (true == g_clients[id].in_use)
		WSASend(g_clients[id].m_sock, &send_over->wsa_buf, 1,
			NULL, 0, &send_over->wsa_over, NULL);
	g_clients[id].c_lock.unlock();
}

void send_chat_packet(int to_client, int id, char* mess, int mess_type)
{
	sc_packet_chat p;
	p.id = id;
	p.size = sizeof(p);
	p.type = SC_PACKET_CHAT;
	p.mess_type = mess_type;
	strcpy_s(p.message, mess);
	send_packet(to_client, &p);
}

void send_login_ok(int id)
{
	sc_packet_login_ok p;
	p.exp = g_clients[id].exp;
	p.hp = g_clients[id].hp;
	p.id = id;
	p.level = g_clients[id].level;
	p.size = sizeof(p);
	p.type = SC_PACKET_LOGIN_OK;
	p.x = g_clients[id].x;
	p.y = g_clients[id].y;

	send_packet(id, &p);
}

void send_login_fail(int id)
{
	sc_packet_login_fail p;
	p.size = sizeof(p);
	p.type = SC_PACKET_LOGIN_FAIL;

	char error[MAX_STR_LEN];
	sprintf_s(error, "Login Error - Invalid ID !!");
	strcpy_s(p.message, error);

	send_packet(id, &p);
}

void send_move_packet(int to_client, int id)
{
	sc_packet_move p;
	p.id = id;
	p.size = sizeof(p);
	p.type = SC_PACKET_MOVE;
	p.x = g_clients[id].x;
	p.y = g_clients[id].y;
	p.move_time = g_clients[id].move_time;
	send_packet(to_client, &p);
}

void send_enter_packet(int to_client, int new_id)
{
	sc_packet_enter p;
	p.id = new_id;
	p.size = sizeof(p);
	p.type = SC_PACKET_ENTER;
	p.x = g_clients[new_id].x;
	p.y = g_clients[new_id].y;
	g_clients[new_id].c_lock.lock();
	strcpy_s(p.name, g_clients[new_id].name);
	g_clients[new_id].c_lock.unlock();
	p.o_type = 0;
	send_packet(to_client, &p);
}

void send_leave_packet(int to_client, int new_id)
{
	sc_packet_leave p;
	p.id = new_id;
	p.size = sizeof(p);
	p.type = SC_PACKET_LEAVE;
	send_packet(to_client, &p);
}

void send_stat_change_packet(int to_client, int new_id) 
{
	// 몬스터 및 나의 상태변화를 알려주자

	sc_packet_stat_chage p;
	p.id = new_id;
	p.size = sizeof(p);
	p.type = SC_PACKET_STAT_CHANGE;
	p.hp = g_clients[new_id].hp;
	p.exp = g_clients[new_id].exp;
	p.level = g_clients[new_id].level;

	send_packet(to_client, &p);
}

void process_move(int id, char dir)
{
	short y = g_clients[id].y;
	short x = g_clients[id].x;

	//cout << mapInfo[x][y] << endl;
	//cout << x << endl;
	//cout << y << endl;

	switch (dir) {
	case MV_UP:
		if (mapInfo[x][y - 1] == NO_OBSTACLE_MAP)
		{
			if (y > 0) y--; break;
		}
		break;
	case MV_DOWN:
		if (mapInfo[x][y + 1] == NO_OBSTACLE_MAP)
		{
			if (y < (WORLD_HEIGHT - 1)) y++; break;
		}
		break;
	case MV_LEFT:
		if (mapInfo[x - 1][y] == NO_OBSTACLE_MAP)
		{
			if (x > 0) x--; break;
		}
		break;
	case MV_RIGHT:
		if (mapInfo[x + 1][y] == NO_OBSTACLE_MAP)
		{
			if (x < WORLD_WIDTH - 1) x++; break;
		}
		break;
	//case MV_UP: if (y > 0) y--; break;
	//case MV_DOWN: if (y < (WORLD_HEIGHT - 1)) y++; break;
	//case MV_LEFT: if (x > 0) x--; break;
	//case MV_RIGHT: if (x < (WORLD_WIDTH - 1)) x++; break;
	default: cout << "Unknown Direction in CS_MOVE packet.\n";
		while (true);
	}

	unordered_set <int> old_viewlist = g_clients[id].view_list;

	g_clients[id].x = x;
	g_clients[id].y = y;


	send_move_packet(id, id);

	unordered_set <int> new_viewlist;
	for (int i = 0; i < MAX_USER; ++i) {
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);
	}

	for (int i = MAX_USER; i < NUM_NPC; ++i) {
		if (true == is_near(id, i)) {
			new_viewlist.insert(i);
			
			wake_up_npc(i);
			send_stat_change_packet(id, i);
		}		
	}

	// 시야에 들어온 객체 처리
	for (int ob : new_viewlist) {
		if (0 == old_viewlist.count(ob)) {
			g_clients[id].view_list.insert(ob);
			send_enter_packet(id, ob);

			if (false == is_npc(ob)) {
				if (0 == g_clients[ob].view_list.count(id)) {
					g_clients[ob].view_list.insert(id);
					send_enter_packet(ob, id);
				}
				else {
					send_move_packet(ob, id);
				}
			}
		}
		else {  // 이전에도 시야에 있었고, 이동후에도 시야에 있는 객체
			if (false == is_npc(ob)) {
				if (0 != g_clients[ob].view_list.count(id)) {
					send_move_packet(ob, id);
				}
				else
				{
					g_clients[ob].view_list.insert(id);
					send_enter_packet(ob, id);
				}
			}
		}
	}
	for (int ob : old_viewlist) {
		if (0 == new_viewlist.count(ob)) {
			g_clients[id].view_list.erase(ob);
			send_leave_packet(id, ob);
			if (false == is_npc(ob)) {
				if (0 != g_clients[ob].view_list.count(id)) {
					g_clients[ob].view_list.erase(id);
					send_leave_packet(ob, id);
				}
			}
		}
	}

	if (false == is_npc(id)) {
		for (auto& npc : new_viewlist) {
			if (false == is_npc(npc)) continue;
			OVER_EX* ex_over = new OVER_EX;
			ex_over->object_id = id;
			ex_over->op_mode = OP_PLAYER_MOVE_NOTIFY;
			PostQueuedCompletionStatus(h_iocp, 1, npc, &ex_over->wsa_over);
		}
	}

}

void process_attack(int id) 
{
	g_clients[id].c_lock.lock();
	auto vl = g_clients[id].view_list;
	g_clients[id].c_lock.unlock();

	char mess[100];

	for (auto npc : vl) {
		// 현재 플레이어 시야에 들어와있는 npc 대상으로 공격 판정을 하자
		// cout << npc << endl;
		// cout << id << endl;

		if ((g_clients[npc].x == g_clients[id].x && g_clients[npc].y == g_clients[id].y - 1) ||
			(g_clients[npc].x == g_clients[id].x && g_clients[npc].y == g_clients[id].y + 1) ||
			(g_clients[npc].x == g_clients[id].x - 1 && g_clients[npc].y == g_clients[id].y) ||
			(g_clients[npc].x == g_clients[id].x + 1 && g_clients[npc].y == g_clients[id].y))
		{			
			
			g_clients[npc].hp -= 2 * g_clients[id].level;  // 레벨이 높을수록 몬스터 HP 금방 깍이도록


			is_npc_die(id, npc);

			char mess[100];
			sprintf_s(mess, "%s -> attack -> %s (-%d).", g_clients[id].name, g_clients[npc].name, g_clients[id].level * 2);

			for (int i = 0; i < UserCount; ++i)
			{
				send_chat_packet(i, id, mess,1);
				send_stat_change_packet(i, npc);
			}
		}
	}

}

void process_packet(int id)
{
	char p_type = g_clients[id].m_packet_start[1];
	switch (p_type) {
	case CS_LOGIN: {
		cs_packet_login* p = reinterpret_cast<cs_packet_login*>(g_clients[id].m_packet_start);
		g_clients[id].c_lock.lock();
		strcpy_s(g_clients[id].name, p->name);
		g_clients[id].c_lock.unlock();

		for (auto& db : vec_database) {
			if (strcmp(db.user_name, p->name) == 0) 
			{
				g_clients[id].x = db.pos_x;
				g_clients[id].y = db.pos_y;
				g_clients[id].hp = db.user_hp;
				g_clients[id].exp = db.user_exp;
				g_clients[id].level = db.user_level;
				g_clients[id].DBKey = db.user_id;
				send_login_ok(id);
				UserCount++;

				if (g_clients[id].hp < 100)
					add_timer(id, OP_PLAYER_HEAL, system_clock::now() + 5s);

			}
			else if (strcmp(db.user_name, p->name) != 0)
			{
				send_login_fail(id);
			}
		}
		
		//send_login_ok(id);

		for (int i = 0; i < MAX_USER; ++i)
			if (true == g_clients[i].in_use)
				if (id != i) {
					if (false == is_near(i, id)) continue;
					if (0 == g_clients[i].view_list.count(id)) {
						g_clients[i].view_list.insert(id);
						send_enter_packet(i, id);
					}
					if (0 == g_clients[id].view_list.count(i)) {
						g_clients[id].view_list.insert(i);
						send_enter_packet(id, i);
					}
				}
		for (int i = MAX_USER; i < NUM_NPC; ++i) {
			if (false == is_near(id, i)) continue;
			g_clients[id].view_list.insert(i);
			send_enter_packet(id, i);
			wake_up_npc(i);
			send_stat_change_packet(id, i);
		}		
		break;
	}
	case CS_MOVE: {
		cs_packet_move* p = reinterpret_cast<cs_packet_move*>(g_clients[id].m_packet_start);
		g_clients[id].move_time = p->move_time;
		process_move(id, p->direction);
		break;
	}
	case CS_ATTACK: {
		cs_packet_attack* p = reinterpret_cast<cs_packet_attack*>(g_clients[id].m_packet_start);
		process_attack(id);
		break;
	}	
	case CS_CHAT: {
		cs_packet_chat* p = reinterpret_cast<cs_packet_chat*>(g_clients[id].m_packet_start);
		break;
	}
	case CS_LOGOUT: {
		cs_packet_logout* p = reinterpret_cast<cs_packet_logout*>(g_clients[id].m_packet_start);
		break;
	}
	case CS_TELEORT: {
		break;
	}
	
	

	default: cout << "Unknown Packet type [" << p_type << "] from Client [" << id << "]\n";
		while (true);
	}
}

constexpr int MIN_BUFF_SIZE = 1024;

void process_recv(int id, DWORD iosize)
{
	unsigned char p_size = g_clients[id].m_packet_start[0];
	unsigned char* next_recv_ptr = g_clients[id].m_recv_start + iosize;
	while (p_size <= next_recv_ptr - g_clients[id].m_packet_start) {
		process_packet(id);
		g_clients[id].m_packet_start += p_size;
		if (g_clients[id].m_packet_start < next_recv_ptr)
			p_size = g_clients[id].m_packet_start[0];
		else break;
	}

	long long left_data = next_recv_ptr - g_clients[id].m_packet_start;

	if ((MAX_BUFFER - (next_recv_ptr - g_clients[id].m_recv_over.iocp_buf))
		< MIN_BUFF_SIZE) {
		memcpy(g_clients[id].m_recv_over.iocp_buf,
			g_clients[id].m_packet_start, left_data);
		g_clients[id].m_packet_start = g_clients[id].m_recv_over.iocp_buf;
		next_recv_ptr = g_clients[id].m_packet_start + left_data;
	}
	DWORD recv_flag = 0;
	g_clients[id].m_recv_start = next_recv_ptr;
	g_clients[id].m_recv_over.wsa_buf.buf = reinterpret_cast<CHAR*>(next_recv_ptr);
	g_clients[id].m_recv_over.wsa_buf.len = MAX_BUFFER -
		static_cast<int>(next_recv_ptr - g_clients[id].m_recv_over.iocp_buf);

	g_clients[id].c_lock.lock();
	if (true == g_clients[id].in_use) {
		WSARecv(g_clients[id].m_sock, &g_clients[id].m_recv_over.wsa_buf,
			1, NULL, &recv_flag, &g_clients[id].m_recv_over.wsa_over, NULL);
	}
	g_clients[id].c_lock.unlock();
}

void add_new_client(SOCKET ns)
{
	int i;
	id_lock.lock();
	for (i = 0; i < MAX_USER; ++i)
		if (false == g_clients[i].in_use) break;
	id_lock.unlock();
	if (MAX_USER == i) {
		cout << "Max user limit exceeded.\n";
		closesocket(ns);
	}
	else {
		// cout << "New Client [" << i << "] Accepted" << endl;
		g_clients[i].c_lock.lock();
		g_clients[i].in_use = true;
		g_clients[i].m_sock = ns;
		g_clients[i].name[0] = 0;
		g_clients[i].c_lock.unlock();

		g_clients[i].m_packet_start = g_clients[i].m_recv_over.iocp_buf;
		g_clients[i].m_recv_over.op_mode = OP_MODE_RECV;
		g_clients[i].m_recv_over.wsa_buf.buf
			= reinterpret_cast<CHAR*>(g_clients[i].m_recv_over.iocp_buf);
		g_clients[i].m_recv_over.wsa_buf.len = sizeof(g_clients[i].m_recv_over.iocp_buf);
		ZeroMemory(&g_clients[i].m_recv_over.wsa_over, sizeof(g_clients[i].m_recv_over.wsa_over));
		g_clients[i].m_recv_start = g_clients[i].m_recv_over.iocp_buf;

		//g_clients[i].x = rand() % WORLD_WIDTH;
		//g_clients[i].y = rand() % WORLD_HEIGHT;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(ns), h_iocp, i, 0);
		DWORD flags = 0;
		int ret;
		g_clients[i].c_lock.lock();
		if (true == g_clients[i].in_use) {
			ret = WSARecv(g_clients[i].m_sock, &g_clients[i].m_recv_over.wsa_buf, 1, NULL,
				&flags, &g_clients[i].m_recv_over.wsa_over, NULL);
		}
		g_clients[i].c_lock.unlock();
		if (SOCKET_ERROR == ret) {
			int err_no = WSAGetLastError();
			if (ERROR_IO_PENDING != err_no)
				error_display("WSARecv : ", err_no);
		}
	}
	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<ULONG> (cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));
	AcceptEx(g_lSocket, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);
}

void disconnect_client(int id)
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (true == g_clients[i].in_use)
			if (i != id) {
				if (0 != g_clients[i].view_list.count(id)) {
					g_clients[i].view_list.erase(id);
					send_leave_packet(i, id);
				}
			}
	}
	g_clients[id].c_lock.lock();
	g_clients[id].in_use = false;
	g_clients[id].view_list.clear();
	closesocket(g_clients[id].m_sock);
	g_clients[id].m_sock = 0;
	g_clients[id].c_lock.unlock();
}

void worker_thread()
{
	// 반복
	//   - 이 쓰레드를 IOCP thread pool에 등록  => GQCS
	//   - iocp가 처리를 맞긴 I/O완료 데이터를 꺼내기 => GQCS
	//   - 꺼낸 I/O완료 데이터를 처리
	while (true) {
		DWORD io_size;
		int key;
		ULONG_PTR iocp_key;
		WSAOVERLAPPED* lpover;
		int ret = GetQueuedCompletionStatus(h_iocp, &io_size, &iocp_key, &lpover, INFINITE);
		key = static_cast<int>(iocp_key);
		// cout << "Completion Detected" << endl;
		if (FALSE == ret) {
			error_display("GQCS Error : ", WSAGetLastError());
		}

		OVER_EX* over_ex = reinterpret_cast<OVER_EX*>(lpover);
		switch (over_ex->op_mode) {
		case OP_MODE_ACCEPT:
			add_new_client(static_cast<SOCKET>(over_ex->wsa_buf.len));
			break;
		case OP_MODE_RECV:
			if (0 == io_size) {
				for (auto& db : vec_database) {
					if (db.user_id == g_clients[key].DBKey) {

						short x = g_clients[key].x;
						short y = g_clients[key].y;
						short hp = g_clients[key].hp;
						short exp = g_clients[key].exp;
						short level = g_clients[key].level;

						//Update_Database(db.user_id, x, y);
						Update_Database(db.user_id, x, y, hp, level, exp);

						db.pos_x = x;
						db.pos_y = y;
						db.user_hp = hp;
						db.user_level = level;
						db.user_exp = exp;
						
					}
				}
				disconnect_client(key);
			}
			else {
				process_recv(key, io_size);
			}
			break;
		case OP_MODE_SEND:
			delete over_ex;
			break;
		case OP_RANDOM_MOVE: 
		{
			if (true == g_clients[key].is_alive)
			{	
				for (int i = 0; i < MAX_USER; ++i) {
					if (g_clients[i].in_use == false) continue;
					random_move_npc(key, i);
				}
				add_timer(key, OP_RANDOM_MOVE, system_clock::now() + 1s);
			}
			delete over_ex;
			//bool is_alive = false;
			//for (int i = MAX_USER; i < NUM_NPC; ++i) {
			//	if (true == is_near(key, i))
			//		if (true == g_clients[i].is_active) {
			//			is_alive = true;
			//			break;
			//		}
			//}

			//if (true == is_alive) {
			//	add_timer(key, OP_RANDOM_MOVE, system_clock::now() + 1s);
			//	cout << "first " << g_clients[key].is_active << endl;
			//}
			//else {
			//	g_clients[key].is_active = false;
			//	cout << "second " << g_clients[key].is_active << endl;
			//}
			//delete over_ex;
		}
			break;
		case OP_PLAYER_MOVE_NOTIFY: 
		{

			g_clients[key].lua_l.lock();
			lua_State* L = g_clients[key].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, over_ex->object_id);//누가 움직였는지 받기 위해 exover에 union을 이용한다.
			int err = lua_pcall(L, 1, 0, 0);
			g_clients[key].lua_l.unlock();
			delete over_ex;
		}
		case OP_PLAYER_HEAL: 
		{
			g_clients[key].hp += 10;
			if (g_clients[key].hp >= 100)
				g_clients[key].hp = 100;
			else if (g_clients[key].hp < 100)
				add_timer(key, OP_PLAYER_HEAL, system_clock::now() + 5s);
					
		}
			break;
		}
	}
}


int API_SendMessageBYE(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* message = (char*)lua_tostring(L, -1);

	send_chat_packet(user_id, my_id, message, 0);

	lua_pop(L, 3);
	return 0; //리턴 값 개수 0개
}

int API_get_x(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = g_clients[obj_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = g_clients[obj_id].y;
	lua_pushnumber(L, y);
	return 1;
}



void initialize_NPC()
{
	//cout << "Initializing NPCs\n";
	for (int i = MAX_USER; i < NUM_NPC; ++i)
	{
		int temp = rand() % 3 + 1;
		g_clients[i].NPC_TYPE = temp;

		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		
		int map_x = int(g_clients[i].x);
		int map_y = int(g_clients[i].y);

		if (mapInfo[map_x][map_y] == OBSTACLE_MAP)
		{

			while (true) {
				if (mapInfo[map_x][map_y] == OBSTACLE_MAP) {
					g_clients[i].x++;
					map_x = (int)g_clients[i].x;
				}
				else
					break;

			}
		}

		g_clients[i].is_alive = true;
		g_clients[i].is_active = false;
		g_clients[i].hp = 100;
		g_clients[i].level = 1;
		
		//g_clients[i].exp = 0;
		//add_timer(i, OP_RANDOM_MOVE, system_clock::now() + 5s);
		char npc_name[50];
		sprintf_s(npc_name, "N%d", i);
		strcpy_s(g_clients[i].name, npc_name);



		lua_State* L = g_clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "test1.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);

		//API 등록
		//lua_register(L, "API_SendMessageHELLO", API_SendMessageHELLO);
		lua_register(L, "API_SendMessageBYE", API_SendMessageBYE);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);

	}
	//cout << "NPC initialize finished.\n";
}

void npc_attack(int id) 
{
	

}

void random_move_npc(int id, int i)
{
	unordered_set <int> old_viewlist;
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) old_viewlist.insert(i);
	}

	int x = g_clients[id].x;
	int y = g_clients[id].y;
	
	// 플레이어 기준으로 근처에 10x10 영역에 있다면 몬스터가 쫓아온다
	if (abs(x - g_clients[i].x) < 10 && abs(y - g_clients[i].y) < 10) {

		moving.x = g_clients[i].x;
		moving.y = g_clients[i].y;
		vector<POINT> vecPath = AStar(moving.x, moving.y, x, y);		

		if (!vecPath.empty() && vecPath.size() > 2) {
			POINT temp;
			temp = vecPath[vecPath.size() - 2];
			x = temp.x;
			y = temp.y;
		}		
	}


	g_clients[id].x = x;
	g_clients[id].y = y;

	// 가까이 오면 공격해라 NPC!!!
	if (abs(x - g_clients[i].x) < 2 && abs(y - g_clients[i].y) < 2)
	{
		if ((g_clients[i].x == x && g_clients[i].y == y - 1) ||
			(g_clients[i].x == x && g_clients[i].y == y + 1) ||
			(g_clients[i].x == x - 1 && g_clients[i].y == y) ||
			(g_clients[i].x == x + 1 && g_clients[i].y == y))
		{

			// 플레이어 HP1씩 깍고 죽으면 경험치 반 깍고 HP 100으로 돌려놓는다.
			g_clients[i].hp -= 5;
			
			if (g_clients[i].hp < 0) {
				g_clients[i].hp = 100;
				g_clients[i].exp = g_clients[i].exp / 2;
				
				char mess[100];
				sprintf_s(mess, "ID : %s  &&  EXP : %d  &&  LEVEL : %d", g_clients[i].name, g_clients[i].exp, g_clients[i].level);
				{
					int temp = rand() % 10 + 1;
					
					g_clients[i].x = revivemap[temp][0];
					g_clients[i].y = revivemap[temp][1];
					
					send_move_packet(i, i);
					send_chat_packet(i, i, mess, 0);
				}
			}

			send_stat_change_packet(i, i);
		}
	}

	unordered_set <int> new_viewlist;

	for (int i = 0; i < MAX_USER; ++i) {
		if (id == i) continue;
		if (false == g_clients[i].in_use) continue;
		if (true == is_near(id, i)) new_viewlist.insert(i);		
	}

	for (auto pl : old_viewlist) {
		if (0 < new_viewlist.count(pl)) {
			if (0 < g_clients[pl].view_list.count(id))
				send_move_packet(pl, id);
			else {
				g_clients[pl].view_list.insert(id);
				send_enter_packet(pl, id);
			}
		}
		else
		{
			if (0 < g_clients[pl].view_list.count(id)) {
				g_clients[pl].view_list.erase(id);
				send_leave_packet(pl, id);
			}
		}
	}

	for (auto pl : new_viewlist) {
		if (0 == g_clients[pl].view_list.count(pl)) {
			if (0 == g_clients[pl].view_list.count(id)) {
				g_clients[pl].view_list.insert(id);
				send_enter_packet(pl, id);
			}
			else
				send_move_packet(pl, id);
		}
	}
}



void initialize_MAP()
{
	for (int i = 0; i < WORLD_WIDTH; ++i) {
		for (int j = 0; j < WORLD_HEIGHT; ++j) {
			if (uid(dre) == 0)
				mapInfo[i][j] = OBSTACLE_MAP;
			else
				mapInfo[i][j] = NO_OBSTACLE_MAP;
		}
	}
}

int main()
{
	std::wcout.imbue(std::locale("korean"));
	for (auto& cl : g_clients)
		cl.in_use = false;

	Load_Database();

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	g_lSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_lSocket), h_iocp, KEY_SERVER, 0);

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	::bind(g_lSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
	listen(g_lSocket, 5);

	SOCKET cSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_accept_over.op_mode = OP_MODE_ACCEPT;
	g_accept_over.wsa_buf.len = static_cast<int>(cSocket);
	ZeroMemory(&g_accept_over.wsa_over, sizeof(&g_accept_over.wsa_over));
	AcceptEx(g_lSocket, cSocket, g_accept_over.iocp_buf, 0, 32, 32, NULL, &g_accept_over.wsa_over);

	initialize_MAP();
	initialize_NPC();

	//thread ai_thread{ npc_ai_thread };
	thread timer_thread{ time_worker };
	vector <thread> worker_threads;
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(worker_thread);
	for (auto& th : worker_threads)
		th.join();
	//ai_thread.join();
	timer_thread.join();

	closesocket(g_lSocket);
	WSACleanup();
}

void sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode) {
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	} while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated.. 
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

void sql_show_error()
{
	printf("error\n");
}

void Load_Database()
{
	SQLHENV henv;		// 데이터베이스에 연결할때 사옹하는 핸들
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql명령어를 전달하는 핸들
	SQLRETURN retcode;  // sql명령어를 날릴때 성공유무를 리턴해줌
	
	SQLINTEGER user_id, user_exp, user_level, user_hp, pos_x, pos_y;
	SQLWCHAR user_name[10];	// 문자열
	SQLLEN cbUserID = 0, cbUserexp = 0, cbUserLevel = 0, cbUserHP = 0, cbPosX = 0, cbPosY = 0, cbUserName = 0;

	setlocale(LC_ALL, "korean"); // 오류코드 한글로 변환
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC로 연결

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5초간 연결 5초넘어가면 타임아웃

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS_Project_2016184004", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL명령어 전달할 한들
					
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"SELECT user_id, user_name, user_hwp, user_level, user_hp, pos_x, pos_y FROM user_table ORDER BY 2", SQL_NTS); // 모든 정보 다 가져오기

					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"EXEC select_player ", SQL_NTS); // 모든 정보 다 가져오기


					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &user_id, 4, &cbUserID);	// 아이디
						retcode = SQLBindCol(hstmt, 2, SQL_UNICODE_CHAR, user_name, 11, &cbUserName); // 이름 유니코드경우 SQL_UNICODE_CHAR 사용
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &user_exp, 4, &cbUserexp);	// 경험치
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &user_level, 4, &cbUserLevel);
						retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &user_hp, 4, &cbUserHP);
						retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &pos_x, 4, &cbPosX);
						retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &pos_y, 4, &cbPosY);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // hstmt 에서 데이터를 꺼내오는거
							if (retcode == SQL_ERROR)
								sql_show_error();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								//wprintf(L"%d: %d %lS %d %d %d %d %d \n", i + 1, user_id, user_name, user_exp, user_level, user_hp, pos_x, pos_y);

								// wchar char로 변환
								char* temp;
								int strSize = WideCharToMultiByte(CP_ACP, 0, user_name, -1, NULL, 0, NULL, NULL);
								temp = new char[11];
								WideCharToMultiByte(CP_ACP, 0, user_name, -1, temp, strSize, 0, 0);
								if (isdigit(temp[strlen(temp) - 1]) == 0) {
									//cout << "문자열공백제거\n";
									temp[strlen(temp) - 1] = 0; // 무슨이유에선진 모르겟지만 아이디마지막문자가 영문일 경우 맨뒤에 공백하나가 추가됨
								}
								DATABASE data;
								data.user_id = user_id;
								memcpy(data.user_name, temp, 10);
								data.user_level = user_level;
								data.user_exp = user_exp;
								data.user_hp = user_hp;
								data.pos_x = pos_x;
								data.pos_y = pos_y;

								//cout << "[" << data.user_name << "]  ";
								//cout << data.user_id << " " << data.user_name << " " << data.pos_x << " " << data.pos_y << " " << data.user_level << " " << data.user_hp << " " << data.user_exp << endl;
								vec_database.emplace_back(data);
							}
							else
								break;
						}
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // 핸들캔슬
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	//cout << "database access complete. \n";
}

void Update_Database(int id, int x, int y,int hp,int level, int exp)
{
	SQLHENV henv;		// 데이터베이스에 연결할때 사옹하는 핸들
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql명령어를 전달하는 핸들
	SQLRETURN retcode;  // sql명령어를 날릴때 성공유무를 리턴해줌
	SQLWCHAR query[1024];
	// wsprintf(query, L"UPDATE user_table SET pos_x = %d, pos_y = %d, user_hp = %d, user_level = %d, user_hwp = %d WHERE user_id = %d", x, y, hp, level, exp, id);
	wsprintf(query, L"EXEC update_player %d, %d, %d, %d, &d, %d", x, y, hp, level, exp, id);

	setlocale(LC_ALL, "korean"); // 오류코드 한글로 변환
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC로 연결

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5초간 연결 5초넘어가면 타임아웃

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GS_Project_2016184004", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL명령어 전달할 한들

					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)query, SQL_NTS); // 쿼리문
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90레벨 이상만 가져오기

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						//cout << "DataBase update success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // 핸들캔슬
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}
/////////////////////////////////////상준 건듬/////////////////////////////
vector<POINT> AStar(int targetX, int targetY, int posx,int posy)
{
	// resultPath: 최종적으로 이동할 경로를 저장하는곳
	vector<POINT> resultPath;

	// openQ: 가능성 있는 노드를 찾는다.(휴리스틱 가중치 기준으로 정렬돼있음)
	// closeQ: 탐색 경로에서 이전에 방문한적 있는 노드들을 저장해둠.
	priority_queue<pair<Huristic, Node>, vector<pair<Huristic, Node>>, greater<pair<Huristic, Node>>> openQ;
	map<POINT, POINT> closeQ; // key: 현재위치, value: 이전위치

	// 큐에 내 위치를 넣고 시작한다.
	pair<Huristic, Node> first;

	first.second.currentLocation = { posx,posy };
	first.second.prevLocation = { -1,-1 };
	first.first.movingDist = 0;
	first.first.predictionDist = -1;

	openQ.push(first);

	// AStarAlgorithmCount: BFS탐색과 비교하기 위해서 최종 경로를 찾을때까지 루프반복한 횟수를 측정한다.
	int AStarAlgorithmCount = 0;

	// 가능성있는 노드가 있을때동안 반복한다.

	while (!openQ.empty())
	{
		AStarAlgorithmCount++;

		Node currentNode = openQ.top().second;
		Huristic currentHuristic = openQ.top().first;
		openQ.pop();

		if (closeQ.find(currentNode.currentLocation) != closeQ.end())
			continue;

		closeQ.insert({ currentNode.currentLocation,
						currentNode.prevLocation });

		int x = currentNode.currentLocation.x;
		int y = currentNode.currentLocation.y;
		int nextY = x;
		int nextX = y;
		// 목적지에 도착했을때 반복문 중지

		if (x == targetX && y == targetY)
			break;

		for (int i = 0; i < 4; i++)
		{
			// 다음위치가 전체영역 안쪽일 때
			nextY = y + dy[i];
			nextX = x + dx[i];
			//if (mapInfo[nextY][nextX] != NO_OBSTACLE_MAP) return
			if (nextX >= 0 && nextX < 800 && nextY >= 0 && nextY < 800)
			{
				// 길이 아니거나, 이전에 방문한곳일 때는 해당 노드를 무시한다.
				if(mapInfo[nextX][nextY] != NO_OBSTACLE_MAP)
					//||closeQ.find({ nextX,nextY }) != closeQ.end())
					continue;

				Huristic huristic;
				Node nextNode;

				//cout << nextY << "," << nextX << endl;
				// 다음위치와 목표위치간의 직선거리를 구해서 휴리스틱가중치로 활용
				int deltaX = nextX - targetX;
				int deltaY = nextY - targetY;
				float directDist = sqrt(deltaX * deltaX + deltaY * deltaY);

				huristic.movingDist = currentHuristic.movingDist + 1;
				huristic.predictionDist = directDist;

				nextNode.currentLocation.x = nextX;
				nextNode.currentLocation.y = nextY;
				nextNode.prevLocation.x = x;
				nextNode.prevLocation.y = y;

				openQ.push({ huristic, nextNode });
			}
		}
		//if (x != nextX && y != nextY) {
		//	//cout << "dd:  " << nextY << "," << nextX << endl;
		//	break;
		//}

	}

	// 도착지까지 가는 경로가 존재할 때 결과벡터에 넣는다.
	if (closeQ.find({ moving.x, moving.y }) != closeQ.end())
	{
		POINT location = { moving.x, moving.y };
		while (location.x != -1)
		{
			resultPath.push_back(location);
			location = closeQ[location];
		}
	}

	return resultPath;
}