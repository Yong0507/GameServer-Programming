//#pragma once
//#include "protocol.h"
//#include "AStar.h"
//
//extern MAP g_Map[WORLD_HEIGHT][WORLD_WIDTH];
//
//
//bool FComp(const _NODE* lhs, const _NODE* rhs) {
//	return lhs->F < rhs->F;
//};
//
//
//CAStar::CAStar()
//{
//
//}
//
//
//CAStar::~CAStar()
//{
//}
//
//void CAStar::setG(_NODE* _node)
//{
//	if (_node->pParent != nullptr)
//		_node->G = _node->pParent->G + 1;
//	else
//		_node->G = 0;
//}
//
//void CAStar::setG_dia(_NODE* _node)
//{
//	if (_node->pParent != nullptr)
//		_node->G = _node->pParent->G + 1.5;
//	else
//		_node->G = 0;
//}
//
//bool CAStar::compareG(int** mapData, _NODE* _node, bool isDia, int dir)
//{
//	std::list<_NODE*>::iterator iter;
//
//	int x = _node->ix;
//	int y = _node->iy;
//
//	switch (dir)
//	{
//	case 1:
//		x = x - 1;
//		y = y - 1;
//		break;
//	case 2:
//		y = y - 1;
//		break;
//	case 3:
//		x = x + 1;
//		y = y - 1;
//		break;
//	case 4:
//		x = x - 1;
//		break;
//	case 5:
//		x = x + 1;
//		break;
//	case 6:
//		x = x - 1;
//		y = y + 1;
//		break;
//	case 7:
//		y = y + 1;
//		break;
//	case 8:
//		x = x + 1;
//		y = y + 1;
//		break;
//	}
//
//	for (iter = openList.begin(); iter != openList.end(); ++iter)
//	{
//		if ((*iter)->iy == y && (*iter)->ix == x)
//		{
//			if (isDia)
//			{
//				if ((*iter)->G >= _node->G + 1.5)
//				{
//					(*iter)->pParent = _node;
//					(*iter)->G = _node->G + 1.5;
//					(*iter)->F = (*iter)->G + (*iter)->H;
//					mapData[(*iter)->iy][(*iter)->ix] = eOPEN;
//				}
//
//				return true;
//			}
//			else
//			{
//				if ((*iter)->G >= _node->G + 1)
//				{
//					(*iter)->pParent = _node;
//					(*iter)->G = _node->G + 1;
//					(*iter)->F = (*iter)->G + (*iter)->H;
//					mapData[(*iter)->iy][(*iter)->ix] = eOPEN;
//				}
//				return true;
//			}
//		}
//	}
//
//	return false;
//}
//
//void CAStar::setH(_NODE* _node)
//{
//	_node->H = abs(_node->ix - iendX) + abs(_node->iy - iendY);
//}
//
//void CAStar::setF(_NODE* _node)
//{
//	_node->F = _node->G + _node->H;
//}
//
//void CAStar::setEndPos(int x, int y)
//{
//	iendX = x;
//	iendY = y;
//}
//
//void CAStar::setStartPos(int x, int y)
//{
//	istartX = x;
//	istartY = y;
//}
//
//bool CAStar::returnPos(int* x, int* y)
//{
//	if (endNode == nullptr)
//		return false;
//
//	_NODE* temp = endNode;
//
//	if (temp->pParent == nullptr)
//	{
//		return false;
//	}
//
//	//만약 플레이어 위치 == NPC 위치 
//		//이동X 공격O -> return
//	if (temp->pParent->ix == istartX && temp->pParent->iy == istartY)
//	{
//		return true;
//	}
//
//	while (true)
//	{
//		if (temp->pParent == nullptr)
//		{
//			return false;
//		}
//
//
//		//만약 플레이어 위치 != NPC 위치  다르다면
//		*x = temp->ix;
//		*y = temp->iy;
//
//		temp = temp->pParent;
//	}
//}
//
//bool CAStar::searchLoad(int** mapData, int _startX, int _startY, int _endX, int _endY)
//{
//	std::list<_NODE*>::iterator iter;
//	while (openList.empty() == false)
//	{
//		iter = openList.begin();
//		delete[](*iter);
//		openList.erase(iter);
//	}
//	while (closeList.empty() == false)
//	{
//		iter = closeList.begin();
//		delete[](*iter);
//		closeList.erase(iter);
//	}
//
//	for (int i = 0; i < WORLD_HEIGHT; ++i)
//		for (int j = 0; j < WORLD_WIDTH; ++j)
//			mapData[i][j] = eBLANK;
//
//
//	//openlist에 시작 노드 추가
//	_NODE* newNode = nullptr;
//
//	mapData[_startY][_startX] = eSTART;
//	mapData[_endY][_endX] = eEND;
//
//	setStartPos(_startX, _startY);
//	setEndPos(_endX, _endY);
//
//	newNode = new _NODE(_startX, _startY, nullptr);
//
//	setG(newNode);
//	setH(newNode);
//	setF(newNode);
//	openList.push_back(newNode);
//
//	int count = 0;
//
//	//단계별 출력때문에 timer에서 매번 호출
//	while (true)
//	{
//		if (openList.size() == 0)
//			return false;
//
//		openList.sort(FComp);
//		_NODE* popNode = openList.front();
//		openList.pop_front();
//
//		//길찾기 끝
//		if (popNode->ix == iendX && popNode->iy == iendY)
//		{
//			endNode = popNode;
//			mapData[iendY][iendX] = eEND;
//			mapData[istartY][istartX] = eSTART;
//
//			//단계별 출력때문에 마지막 한번 더 호출 그냥 없어도 됨
//			//InvalidateRect(hWnd, NULL, false);
//			return true;
//		}
//
//		//100칸 이상 멀어지면 안따라감
//		if (count >= 100)
//		{
//			endNode = nullptr;
//			return false;
//		}
//		count++;
//
//		closeList.push_back(popNode);
//		int x = popNode->ix;
//		int y = popNode->iy;
//		mapData[y][x] = eCLOSE;
//
//
//		//■□□
//		//□□□
//		//□□□
//		if (x - 1 >= 0 && y - 1 >= 0) //배열 다음줄로 밀리는경우
//		{
//			if (g_Map[y - 1][x - 1].type != eBLOCKED)
//			{
//				if (mapData[y - 1][x - 1] == eOPEN || mapData[y - 1][x - 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, true, 1); //new노드랑, list 싹찾아서 비교
//				}
//				else if ((g_Map[y - 1][x - 1].type == eBLANK) || (g_Map[y - 1][x - 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x - 1, y - 1, popNode);
//					setG_dia(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y - 1][x - 1] = eOPEN;
//				}
//			}
//		}
//		//□■□
//		//□□□
//		//□□□
//		if (y - 1 >= 0)
//		{
//			if (g_Map[y - 1][x].type != eBLOCKED)
//			{
//				if (mapData[y - 1][x] == eOPEN || mapData[y - 1][x] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, false, 2); //new노드랑, list 싹찾아서 비교
//				}
//				else if ((g_Map[y - 1][x].type == eBLANK) || (g_Map[y - 1][x].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x, y - 1, popNode);
//					setG(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y - 1][x] = eOPEN;
//				}
//			}
//		}
//		//□□■
//		//□□□
//		//□□□
//		if (x + 1 < WORLD_WIDTH && y - 1 >= 0)
//		{
//			if (g_Map[y - 1][x + 1].type != eBLOCKED)
//			{
//				if (mapData[y - 1][x + 1] == eOPEN || mapData[y - 1][x + 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, true, 3); //new노드랑, list 싹찾아서 비교
//				}
//				else if ((g_Map[y - 1][x + 1].type == eBLANK) || (g_Map[y - 1][x + 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x + 1, y - 1, popNode);
//					setG_dia(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y - 1][x + 1] = eOPEN;
//				}
//			}
//		}
//		//□□□
//		//■□□
//		//□□□
//		if (x - 1 >= 0)
//		{
//			if (g_Map[y][x - 1].type != eBLOCKED)
//			{
//				if (mapData[y][x - 1] == eOPEN || mapData[y][x - 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, false, 4); //new노드랑, list 싹찾아서 비교
//				}
//				else if ((g_Map[y][x - 1].type == eBLANK) || (g_Map[y][x - 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x - 1, y, popNode);
//					setG(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y][x - 1] = eOPEN;
//				}
//			}
//		}
//		//□□□
//		//□□■
//		//□□□
//		if (x + 1 < WORLD_WIDTH)
//		{
//			if (g_Map[y][x + 1].type != eBLOCKED)
//			{
//				if (mapData[y][x + 1] == eOPEN || mapData[y][x + 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, false, 5); //new노드랑, list 싹찾아서 비교
//
//				}
//				else if ((g_Map[y][x + 1].type == eBLANK) || (g_Map[y][x + 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x + 1, y, popNode);
//					setG(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y][x + 1] = eOPEN;
//				}
//			}
//		}
//		//□□□
//		//□□□
//		//■□□
//		if (x - 1 >= 0 && y + 1 < WORLD_HEIGHT)
//		{
//			if (g_Map[y + 1][x - 1].type != eBLOCKED)
//			{
//				if (mapData[y + 1][x - 1] == eOPEN || mapData[y + 1][x - 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, true, 6); //new노드랑, list 싹찾아서 비교
//				}
//				else if ((g_Map[y + 1][x - 1].type == eBLANK) || (g_Map[y + 1][x - 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x - 1, y + 1, popNode);
//					setG_dia(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y + 1][x - 1] = eOPEN;
//				}
//			}
//		}
//		//□□□
//		//□□□
//		//□■□
//		if (y + 1 < WORLD_HEIGHT)
//		{
//			if (g_Map[y + 1][x].type != eBLOCKED)
//			{
//				if (mapData[y + 1][x] == eOPEN || mapData[y + 1][x] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, false, 7); //new노드랑, list 싹찾아서 비교
//
//				}
//				else if ((g_Map[y + 1][x].type == eBLANK) || (g_Map[y + 1][x].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x, y + 1, popNode);
//					setG(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y + 1][x] = eOPEN;
//				}
//			}
//		}
//		//□□□
//		//□□□
//		//□□■
//		if (x + 1 < WORLD_WIDTH && y + 1 < WORLD_HEIGHT)
//		{
//			if (g_Map[y + 1][x + 1].type != eBLOCKED)
//			{
//				if (mapData[y + 1][x + 1] == eOPEN || mapData[y + 1][x + 1] == eCLOSE)
//				{
//					//노드는 만들지 않는데, 여기 해당하는 노드의 F값을 비교해서 더 빠른길이면 새로 넣어주기
//					compareG(mapData, popNode, true, 8); //new노드랑, list 싹찾아서 비교
//
//				}
//				else if ((g_Map[y + 1][x + 1].type == eBLANK) || (g_Map[y + 1][x + 1].type == eEND))
//				{
//					_NODE* newNode = new _NODE(x + 1, y + 1, popNode);
//					setG_dia(newNode);
//					setH(newNode);
//					setF(newNode);
//					openList.push_back(newNode);
//					mapData[y + 1][x + 1] = eOPEN;
//				}
//			}
//		}
//
//
//
//	}
//	mapData[istartY][istartX] = eSTART;
//	//InvalidateRect(hWnd, NULL, false);
//}
//
////void CAStar::pathDraw(HWND hWnd)
////{
////	if (endNode == nullptr)
////		return;
////
////	HDC hdc = GetDC(hWnd);
////
////
////	NODE* temp = endNode;
////	while (true)
////	{
////		HPEN Pen, oPen;
////		Pen = CreatePen(PS_SOLID	, 3, RGB(255, 0, 0));
////		oPen = (HPEN)SelectObject(hdc, Pen);
////		MoveToEx(hdc, temp->pParent->ix* 20 + 10, temp->pParent->iy * 20+10,NULL);
////		LineTo(hdc, temp->pParent->pParent->ix* 20 + 10, temp->pParent->pParent->iy* 20 + 10);
////		SelectObject(hdc, oPen);
////		DeleteObject(Pen);
////
////		temp = temp->pParent;
////
////		if (temp->pParent->pParent == nullptr)
////			return;
////	}
////
////	InvalidateRect(hWnd, NULL, false);
////	ReleaseDC(hWnd, hdc);
////}
//
//void CAStar::setEndNodeNULL()
//{
//	endNode = nullptr;
//}
//
//bool CAStar::isEndNode()
//{
//	if (endNode == nullptr)
//		return false;
//	else
//		return true;
//}
