#pragma once
#include "GameConstants.h"
#include "../External/json.hpp"
#include "../External/utils.h"
#include <vector>
#include <cstdlib>
using json = nlohmann::json;

// �غϱ��λ 0-��ͨ��1-���ı�����2-���������4-�����Ů��
// ��������������������
enum TurnFlags { TURN_NORMAL = 0, TURN_RACE = 1, TURN_PREFER_RACE = 2, TURN_RED = 4 };

// �������ɱ������� [start, end]
struct FreeRaceData
{
	int startTurn;	// ��ʼ�غ�
	int endTurn;	// �����غϣ�������
	int count;		// ��Ҫ�Ĵ���

	// ����friend from_json, to_json��json��ʹ��
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(FreeRaceData, startTurn, endTurn, count)
};

//����Ĳ���
struct UmaData
{
	int star;//����
	int gameId;
	int races[TOTAL_TURN];// �غϱ�� enum TurnFlags
	int fiveStatusBonus[5];//���Լӳ�
	int fiveStatusInitial[5];//��ʼ����
	std::vector<FreeRaceData> freeRaces;	// ���ɱ������䣨�����
	std::string name;

	// ����inline
	friend void to_json(json& j, const UmaData& me)
	{
		j["gameId"] = me.gameId;
		j["name"] = string_To_UTF8(me.name);
		j["star"] = me.star;
		j["fiveStatusBonus"] = arrayToJson(me.fiveStatusBonus, 5);
		j["fiveStatusInitial"] = arrayToJson(me.fiveStatusInitial, 5);
		j["freeRaces"] = me.freeRaces;
		// dump races
		std::vector<int> races, preferRaces, preferReds;
		for (int i = 0; i < TOTAL_TURN; ++i) {
			if (me.races[i] & TURN_RACE)
				races.push_back(i);
			if (me.races[i] & TURN_PREFER_RACE)
				preferRaces.push_back(i);
			if (me.races[i] & TURN_RED)
				preferReds.push_back(i);
		}
		j["races"] = races;
		j["preferRaces"] = preferRaces;
		j["preferReds"] = preferReds;
	}

	  friend void from_json(const json& j, UmaData& me)
	  {
		  std::string st;
		  j.at("gameId").get_to(me.gameId);
		  j.at("name").get_to(st);
		  me.name = UTF8_To_string(st);
		  j.at("star").get_to(me.star);
		  jsonToArray(j.at("fiveStatusBonus"), me.fiveStatusBonus, 5);
		  jsonToArray(j.at("fiveStatusInitial"), me.fiveStatusInitial, 5);
		  j.at("freeRaces").get_to(me.freeRaces);

		  // ��Races��preferRaces��preferRedsѹ����RaceFlags��
		  memset(me.races, 0, sizeof(me.races));
		  if (j["races"][0].is_boolean())
		  {
			  // �ϱ�����ʽ
			  for (int i = 0; i < j["races"].size(); ++i)
				  if ((bool)(j["races"][i]))
					  me.races[i] |= TURN_RACE;
		  }
		  else // is int
		  {
				//me.races[TOTAL_TURN - 1] = true;//Grand Master�����һս��

				for (auto turn : j["races"])
				{
					static_assert(TOTAL_TURN < 1000);
					if(turn<TOTAL_TURN)
						me.races[turn] |= TURN_RACE;
					else if (turn > 1000)//�����°룬����1061�����һ��6���ϰ룬2112����ڶ���11���°�
					{
						int year = turn / 1000 - 1;
						int month = (turn % 1000) / 10 - 1;
						int halfmonth = turn % 10 - 1;
						int realTurn = year * 24 + month * 2 + halfmonth;
						me.races[realTurn] |= TURN_RACE;

					}
				}
		  }
		  for (auto turn : j["preferRaces"])
			  me.races[turn] |= TURN_PREFER_RACE;
		  for (auto turn : j["preferReds"])
			  me.races[turn] |= TURN_RED;
	  }
};

