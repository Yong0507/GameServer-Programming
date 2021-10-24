#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <windows.h>
#include <iostream>
#include <string>
#include <random>
#include <unordered_map>
#include <chrono>
#include <string>
#include "protocol.h"
using namespace std;
using namespace chrono;


sf::TcpSocket g_socket;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH / 2 + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH / 2 + 10;
constexpr auto BUF_SIZE = 200;


// 추후 확장용.
int NPC_ID_START = MAX_USER;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

sf::RectangleShape rectangle_EXPbox;
sf::RectangleShape rectangle_EXP;

////////////////

bool login_ok = false;
bool loginOK = false;
 

// attack Flag
bool IsAttack = false;
bool IsMonster = false;

// for obstacle
default_random_engine dre{ 2016184004 };   // 시드값 주자 장애물 정보를 클라, 서버 둘다 동일하게 가지고 유지
uniform_int_distribution <> uid{ 0,5 };

sf::Text m_worldText[3];
high_resolution_clock::time_point m_worldtime_out[3];

sf::Text m_responText;
high_resolution_clock::time_point m_time_out_responText;

char g_Map[WORLD_WIDTH][WORLD_HEIGHT];

class OBJECT {
private:
    bool m_showing;
    sf::Sprite m_sprite;
    sf::Sprite m_sprite_attack;
    sf::Sprite m_sprite_damage;
    
    sf::RectangleShape Hp_UI1;
    sf::RectangleShape Hp_UI2;
    sf::ConvexShape    Level_UI;
    sf::CircleShape    exp_UI;

    char m_mess[MAX_STR_LEN];
    high_resolution_clock::time_point m_time_out;
    sf::Text m_text;
    sf::Text m_name;

public:
    int m_x, m_y;
    char name[MAX_ID_LEN];
    short hp;
    short level;
    int   exp;
    int id;

    OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
        m_showing = false;
        m_sprite.setTexture(t);
        m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
        m_time_out = high_resolution_clock::now();
    }
    OBJECT() {
        m_showing = false;
        m_time_out = high_resolution_clock::now();
        
    }

    void setMoveMotion(sf::Texture& t,int x,int y, int x2,int y2)
    {
        m_sprite.setTexture(t);
        m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
    }

    void setAttackMotion(sf::Texture& t, int x, int y, int x2, int y2)
    {
        m_sprite_attack.setTexture(t);
        m_sprite_attack.setTextureRect(sf::IntRect(x, y, x2, y2));
    }

    void setDamageMotion(sf::Texture& t, int x, int y, int x2, int y2)
    {
        m_sprite_damage.setTexture(t);
        m_sprite_damage.setTextureRect(sf::IntRect(x, y, x2, y2));
    }

    void show()
    {
        m_showing = true;
    }
    void hide()
    {
        m_showing = false;
    }

    void a_move(int x, int y) {
        m_sprite.setPosition((float)x, (float)y);
    }

    void a_draw() {
        g_window->draw(m_sprite);
    }

    void move(int x, int y) {
        m_x = x;
        m_y = y;
    }
    void draw() {
        if (false == m_showing) return;
        float rx = (m_x - g_left_x) * 65.0f + 8;
        float ry = (m_y - g_top_y) * 65.0f + 8;
        m_sprite.setPosition(rx, ry);
        g_window->draw(m_sprite);
        m_name.setPosition(rx - 10, ry - 10);
        g_window->draw(m_name);
        if (high_resolution_clock::now() < m_time_out) {
            m_text.setPosition(rx - 10, ry + 15);
            g_window->draw(m_text);
        }

        if (IsAttack == true) 
        {
            int attack_cnt = 0;

            m_sprite_attack.setPosition(rx + TILE_WIDTH, ry);
            g_window->draw(m_sprite_attack);

            m_sprite_attack.setPosition(rx - TILE_WIDTH, ry);
            g_window->draw(m_sprite_attack);

            m_sprite_attack.setPosition(rx, ry + TILE_WIDTH);
            g_window->draw(m_sprite_attack);

            m_sprite_attack.setPosition(rx, ry - TILE_WIDTH);
            g_window->draw(m_sprite_attack);
        }


        Hp_UI1.setPosition(sf::Vector2f(rx, ry - 50));
        Hp_UI1.setOutlineThickness(3.0f);
        Hp_UI1.setOutlineColor(sf::Color::White);
        Hp_UI1.setFillColor(sf::Color(255, 0, 0, 100));
        Hp_UI1.setSize(sf::Vector2f(100, 30));
        g_window->draw(Hp_UI1);

        Hp_UI2.setPosition(sf::Vector2f(rx, ry - 50));
        Hp_UI2.setFillColor(sf::Color(0, 255, 0, 100));
        Hp_UI2.setSize(sf::Vector2f(hp, 30));
        g_window->draw(Hp_UI2);
    }

    void set_name(char str[]) {
        m_name.setFont(g_font);
        m_name.setString(str);
        m_name.setFillColor(sf::Color(255, 255, 0));
        m_name.setStyle(sf::Text::Bold);
    }

    void set_hp(char str[]) {
        m_name.setFont(g_font);
        m_name.setString(str);
        m_name.setFillColor(sf::Color(255, 255, 0));
        m_name.setStyle(sf::Text::Bold);    
    }

    void set_level(char str[]) {
        m_name.setFont(g_font);
        m_name.setString(str);
        m_name.setFillColor(sf::Color(255, 255, 0));
        m_name.setStyle(sf::Text::Bold);
    }

    void set_exp(char str[]) {
        m_name.setFont(g_font);
        m_name.setString(str);
        m_name.setFillColor(sf::Color(255, 255, 0));
        m_name.setStyle(sf::Text::Bold);
    }


    void add_chat(char chat[]) {
        m_text.setFont(g_font);
        m_text.setString(chat);
        m_time_out = high_resolution_clock::now() + 1s;
    }
};

OBJECT avatar;   
unordered_map <int, OBJECT> npcs;

OBJECT white_tile;
OBJECT black_tile;
OBJECT map_tile;
OBJECT obstacle_tile;
OBJECT Player;
OBJECT Attack;
//OBJECT Damage;

sf::Texture* board;
sf::Texture* npc;
sf::Texture* tile1;
sf::Texture* tile2;
sf::Texture* myPlayer;
sf::Texture* attacking;
sf::Texture* damaging;

void client_initialize()
{
    board = new sf::Texture;
    npc = new sf::Texture;
    tile1 = new sf::Texture;
    tile2 = new sf::Texture;
    myPlayer = new sf::Texture;
    attacking = new sf::Texture;
    damaging = new sf::Texture;

    if (false == g_font.loadFromFile("ConsolaMalgun.ttf")) {
        cout << "Font Loading Error!\n";
        while (true);
    }

    board->loadFromFile("chessmap.bmp");
    npc->loadFromFile("monster2.png");
    tile1->loadFromFile("maptile.png");
    tile2->loadFromFile("obstacle.png");
    myPlayer->loadFromFile("player.png");
    attacking->loadFromFile("fire.png");

    white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
    black_tile = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
    map_tile = OBJECT{ *tile1,0,0,TILE_WIDTH,TILE_WIDTH };
    obstacle_tile = OBJECT{ *tile2,0,0,TILE_WIDTH,TILE_WIDTH };

    
    for (int i = 0; i < WORLD_WIDTH; ++i) {
        for (int j = 0; j < WORLD_HEIGHT; ++j) {
            if (uid(dre) == 0)
                g_Map[i][j] = eBLOCKED;
            else
                g_Map[i][j] = eBLANK;
        }
    }
    
    //avatar = OBJECT{ *myPlayer, 0, 10, TILE_WIDTH, TILE_WIDTH };

    avatar.setMoveMotion(*myPlayer, 0, 0, 65, 65);
    avatar.setAttackMotion(*attacking, 0, 0, 65, 65);
    //avatar.setDamageMotion(*damaging, 0, 0, 65, 65);
    
    avatar.move(4, 4);
}


void client_finish()
{
    delete board;
    delete npc;
    delete tile1;
    delete tile2;
    delete myPlayer;
    delete attacking;
}

sf::Text text;
sf::Text text1;


void ProcessPacket(char* ptr)
{
    static bool first_time = true;
    switch (ptr[1])
    {
    case SC_PACKET_LOGIN_OK:
    {
        sc_packet_login_ok* my_packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
        g_myid = my_packet->id;
               
        avatar.move(my_packet->x, my_packet->y);
       
        g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
        g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
        avatar.id = my_packet->id;
        avatar.hp = my_packet->hp;
        avatar.exp = my_packet->exp;
        avatar.level = my_packet->level;

        char buf[100];
        text.setFont(g_font);
        sprintf_s(buf, "ID : %dP  &&  EXP : %d/%d  &&  LEVEL : %d", avatar.id, avatar.exp, (int)(100 * pow(2, (avatar.level - 1))), avatar.level);

        text.setString(buf);
        text.setPosition(20, 0);
        text.setCharacterSize(40);
        sf::Color color(0, 0, 0);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Blue);
        text.setStyle(sf::Text::Underlined);


        avatar.show();

        loginOK = true;
    }
    break;

    case SC_PACKET_ENTER:
    {
        sc_packet_enter* my_packet = reinterpret_cast<sc_packet_enter*>(ptr);
        int id = my_packet->id;

        if (id == g_myid) {
            avatar.move(my_packet->x, my_packet->y);
            //g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
            //g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
            avatar.show();
        }
        else {
            if (id < NPC_ID_START)
                //npcs[id] = OBJECT{ *pieces, 64, 0, 64, 64 };
            {
                npcs[id] = OBJECT{ *myPlayer , 0,0,TILE_WIDTH,TILE_WIDTH };
                npcs[id].hp = 100;
            }
            else
                npcs[id] = OBJECT{ *npc, 0, 0, TILE_WIDTH, TILE_WIDTH };
            strcpy_s(npcs[id].name, my_packet->name);
            npcs[id].set_name(my_packet->name);
            npcs[id].move(my_packet->x, my_packet->y);
            npcs[id].show();
            //npcs[id].hp = 1;
        }
    }
    break;
    case SC_PACKET_MOVE:
    {
        sc_packet_move* my_packet = reinterpret_cast<sc_packet_move*>(ptr);
        int other_id = my_packet->id;
        if (other_id == g_myid) {
            avatar.move(my_packet->x, my_packet->y);
            Attack.move(my_packet->x, my_packet->y);
            g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
            g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
            //cout << g_left_x << endl;
            //cout << g_top_y << endl;

        }
        else {
            if (0 != npcs.count(other_id))
                npcs[other_id].move(my_packet->x, my_packet->y);
        }


    }
    break;

    case SC_PACKET_LEAVE:
    {
        sc_packet_leave* my_packet = reinterpret_cast<sc_packet_leave*>(ptr);
        int other_id = my_packet->id;
        if (other_id == g_myid) {
            avatar.hide();
        }
        else {
            if (0 != npcs.count(other_id)) {
                npcs[other_id].hide();
                npcs[other_id].hp = 0;
                
            }
        }
    }
    break;

    case SC_PACKET_CHAT: 
    {
        sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);
        int other_id = my_packet->id;
        int mess_type = my_packet->mess_type;
     
            if (mess_type == 0)     // 경험치, 레벨, ID 등등의 플레이어 정보 메시지
            {
                if (0 != npcs.count(other_id)) 
                {
                    npcs[other_id].add_chat(my_packet->message);
                }

                text.setFont(g_font);
                char buf[100];

                sprintf_s(buf, my_packet->message);
                text.setString(buf);
                text.setPosition(20, 0);
                text.setCharacterSize(40);
                sf::Color color(0, 0, 0);
                text.setOutlineColor(sf::Color::Blue);
                text.setStyle(sf::Text::Underlined);

            
            }
            else if (mess_type == 1) 
            {
                
                npcs[other_id].add_chat(my_packet->message);

                text1.setFont(g_font);
                char buf[100];

                sprintf_s(buf, my_packet->message);
                text1.setString(buf);
                text1.setPosition(20, 1100);
                text1.setCharacterSize(40);
                sf::Color color(0, 0, 0);
                text1.setOutlineColor(sf::Color::Blue);
                text1.setStyle(sf::Text::Underlined);
            }                      
    }
    break;
    
    case SC_PACKET_LOGIN_FAIL: 
    {
        sc_packet_login_fail* my_packet = reinterpret_cast<sc_packet_login_fail*>(ptr);
        int id = my_packet->id;
        if (id != g_myid ) {
            //cout << "Login Failed!!\n";
            //cout << my_packet->message << endl;
        }
    }

    case SC_PACKET_STAT_CHANGE: {
        sc_packet_stat_chage* my_packet = reinterpret_cast<sc_packet_stat_chage*>(ptr);
        int id = my_packet->id;
        // int hp = my_packet->hp;
        if (id == g_myid) {
            //if (avatar.hp > my_packet->hp)
                //avatar.damageFlag = true;
            
            avatar.hp = my_packet->hp;
            avatar.level = my_packet->level;
            avatar.exp = my_packet->exp;



            //char str[20];
            //sprintf_s(str, "%d", my_packet->level);
            //avatar.set_level(str);

            //sprintf_s(str, "EXP ");
            //avatar.set_exp(str);

            //sprintf_s(str, "%d/100", my_packet->hp);
            //avatar.set_hp(str);

            avatar.show();
        }
        if(id > NPC_ID_START)
        {
            //IsMonster = true;
            npcs[id].exp = my_packet->exp;
            npcs[id].hp = my_packet->hp;
            npcs[id].level = my_packet->level;  
            // cout << npcs[id].hp << endl;
        }
        //else 
        //{
        //    npcs[id].exp = my_packet->exp;
        //    npcs[id].hp = my_packet->hp;
        //    npcs[id].level = my_packet->level;
        //    cout << npcs[id].exp << endl;
        //}
    }
    //default:
    //    printf("Unknown PACKET type [%d]\n", ptr[1]);

    }
}

void process_data(char* net_buf, size_t io_byte)
{
    char* ptr = net_buf;
    static size_t in_packet_size = 0;
    static size_t saved_packet_size = 0;
    static char packet_buffer[BUF_SIZE];

    while (0 != io_byte) {
        if (0 == in_packet_size) in_packet_size = ptr[0];
        if (io_byte + saved_packet_size >= in_packet_size) {
            memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
            ProcessPacket(packet_buffer);
            ptr += in_packet_size - saved_packet_size;
            io_byte -= in_packet_size - saved_packet_size;
            in_packet_size = 0;
            saved_packet_size = 0;
        }
        else {
            memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
            saved_packet_size += io_byte;
            io_byte = 0;
        }
    }
}

void client_main()
{
    char net_buf[BUF_SIZE];
    size_t   received;

    auto recv_result = g_socket.receive(net_buf, BUF_SIZE, received);
    if (recv_result == sf::Socket::Error)
    {
        wcout << L"Recv 에러!";
        while (true);
    }

    if (recv_result == sf::Socket::Disconnected)
    {
        wcout << L"서버 접속 종료.";
        g_window->close();
    }

    if (recv_result != sf::Socket::NotReady)
        if (received > 0) process_data(net_buf, received);


    if (loginOK == true)
    {
        for (int i = 0; i < SCREEN_WIDTH; ++i) {
            int tile_x = i + g_left_x;
            if (tile_x >= WORLD_WIDTH) break;
            if (tile_x < 0) continue;

            for (int j = 0; j < SCREEN_HEIGHT; ++j)
            {
                int tile_y = j + g_top_y;
                if (tile_y >= WORLD_HEIGHT) break;
                if (tile_y < 0) continue;
                
                if(g_Map[tile_x][tile_y] == eBLOCKED)
                {
                    obstacle_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
                    obstacle_tile.a_draw();
                }
                else if (g_Map[tile_x][tile_y] == eBLANK)
                {   
                    map_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
                    map_tile.a_draw();
                }
            }
        }

        avatar.draw();
       
        for (auto& npc : npcs) {
            npc.second.draw();

            if (npc.second.hp <= 0)
                npc.second.hide();
            else
                npc.second.show();

        }

        //sf::Text text;
        //text.setFont(g_font);
        //char buf[100];

        //sprintf_s(buf, "HP %d/100 &&& EXP : %d/%d &&& LEVEL : %d", avatar.hp, avatar.exp, (int)(100 * pow(2, (avatar.level - 1))), avatar.level);
        //text.setString(buf);
        //text.setPosition(20, 900);
        //text.setCharacterSize(30);
        //text.setOutlineColor(sf::Color::Blue);
        //text.setStyle(sf::Text::Underlined);
        g_window->draw(text);
        g_window->draw(text1);

        //// exp
        //rectangle_EXPbox.setPosition(sf::Vector2f(50, WINDOW_HEIGHT + 100));
        //rectangle_EXPbox.setOutlineThickness(3.0f);
        //rectangle_EXPbox.setOutlineColor(sf::Color::White);
        //rectangle_EXPbox.setFillColor(sf::Color::Black);
        //rectangle_EXPbox.setSize(sf::Vector2f(WINDOW_WIDTH + 500, 25));
        //g_window->draw(rectangle_EXPbox);

        //if (avatar.level > 0)
        //{
        //    rectangle_EXP.setPosition(sf::Vector2f(50, WINDOW_HEIGHT + 100));
        //    rectangle_EXP.setFillColor(sf::Color::Blue);
        //    rectangle_EXP.setSize(sf::Vector2f((avatar.exp * (WINDOW_WIDTH + 500)) / (int)(100 * pow(2, (avatar.level - 1))), 25));
        //    g_window->draw(rectangle_EXP);
        //}

        //total exp level * level * 100 

        //avatar.m_exp.setPosition(10, WINDOW_HEIGHT + 530);
        //g_window->draw(avatar.m_exp);
    }
}

bool collision_check(int pos_x, int pos_y) 
{
    if (g_Map[pos_x + 6][pos_y + 7] == 0) return true;
    else if (g_Map[pos_x + 8][pos_y + 7] == 0)  return true;
    else if (g_Map[pos_x + 7][pos_y + 6] == 0) return true;
    else if (g_Map[pos_x + 7][pos_y + 8] == 0)  return true;
    else return false;
}

void send_packet(void* packet)
{
    char* p = reinterpret_cast<char*>(packet);
    size_t sent;
    sf::Socket::Status st = g_socket.send(p, p[0], sent);
    int a = 3;
}

void send_move_packet(unsigned char dir)
{
    cs_packet_move m_packet;
    m_packet.type = CS_MOVE;
    m_packet.size = sizeof(m_packet);
    m_packet.direction = dir;
    send_packet(&m_packet);
}

void send_attack_packet()
{
    cs_packet_attack m_packet;
    m_packet.type = CS_ATTACK;
    m_packet.size = sizeof(m_packet);
    send_packet(&m_packet);
}

void send_chat_packet(char* mess) 
{
    cs_packet_chat m_packet;
    m_packet.type = CS_CHAT;
    m_packet.size = sizeof(m_packet);
    strcpy_s(m_packet.message, mess);
    send_packet(&m_packet);
}

void send_logout_packet() 
{
    cs_packet_logout m_packet;
    m_packet.type = CS_LOGOUT;
    m_packet.size = sizeof(m_packet);
    send_packet(&m_packet);
}

void send_teleport_packet() 
{

    
}

int main()
{
    sf::Text text;
    sf::Font font;

    font.loadFromFile("ConsolaMalgun.ttf");

    text.setFillColor(sf::Color::White);
    text.setFont(font);
    string s;
    const char* c;

    ///////////////////

    wcout.imbue(locale("korean"));
    sf::Socket::Status status = g_socket.connect("127.0.0.1", SERVER_PORT);
    g_socket.setBlocking(false);

    if (status != sf::Socket::Done) {
        wcout << L"서버와 연결할 수 없습니다.\n";
        while (true);
    }

    client_initialize();

    // -- 로그인 ID 입력 -- //
    cs_packet_login m_packet;
    m_packet.type = CS_LOGIN;
    m_packet.size = sizeof(m_packet);
    int t_id = GetCurrentProcessId();
    char id[10];
    cout << "ID 입력 : ";
    cin >> id;
    strcpy_s(m_packet.name, id);
    send_packet(&m_packet);
    // -- 로그인 ID 입력 -- //


    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
    g_window = &window;

    sf::View view = g_window->getView();
    view.zoom(2.0f);
    view.move(SCREEN_WIDTH * TILE_WIDTH / 4, SCREEN_HEIGHT * TILE_WIDTH / 4);
    g_window->setView(view);


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                int p_type = -1;
                switch (event.key.code) {
                case sf::Keyboard::Left:
                    send_move_packet(MV_LEFT);
                    break;
                case sf::Keyboard::Right:
                    send_move_packet(MV_RIGHT);
                    break;
                case sf::Keyboard::Up:
                    send_move_packet(MV_UP);
                    break;
                case sf::Keyboard::Down:
                    send_move_packet(MV_DOWN);
                    break;
                case sf::Keyboard::A:
                    IsAttack = true;
                    send_attack_packet();
                    break;
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                }
            }

            if (event.type == sf::Event::KeyReleased) {
                switch (event.key.code) {
                case sf::Keyboard::A:
                    IsAttack = false;
                }
            }


        }
        window.clear();        
        client_main();              
        window.display();
    }
    client_finish();

    return 0;
}