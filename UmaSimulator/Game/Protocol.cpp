#include <iostream>
#include <sstream>
#include <cassert>
#include "../External/json.hpp"
#include "Game.h"
using namespace std;
using json = nlohmann::json;

// �Ƿ�ѵ��ƿ��������ƴ������ᵼ��һ����Ԥ��ƫ�
// ΪTrueʱ�������ID�����λ��Ϊ���ƣ�����5xxx����4xxx��
static bool maskUmaId = true;

int hack_umaId(int umaId)
{
    return umaId % 1000000;
}

int hack_scId(int scId)
{
    return scId % 100000 + 400000;
}

bool Game::loadGameFromJson(std::string jsonStr)
{
  try
  {
    json j = json::parse(jsonStr);
    //cout << jsonStr << endl;
    umaId = j["umaId"];
    if (maskUmaId)
        umaId = hack_umaId(umaId);
    if (!GameDatabase::AllUmaGameIdToSimulatorId.count(umaId))
      throw "δ֪����";
    umaId = GameDatabase::AllUmaGameIdToSimulatorId.at(umaId);

    turn = j["turn"];
    if (turn >= TOTAL_TURN && turn < 0)
      throw "�غ�������ȷ";

    vital = j["vital"];
    maxVital = j["maxVital"];
    isQieZhe = j["isQieZhe"];
    isAiJiao = j["isAiJiao"];
    failureRateBias = j["failureRateBias"];
    for (int i = 0; i < 5; i++)
      fiveStatus[i] = j["fiveStatus"][i];
    for (int i = 0; i < 5; i++)
      fiveStatusLimit[i] = j["fiveStatusLimit"][i];

    skillPt = j["skillPt"];
    motivation = j["motivation"];
    for (int i = 0; i < 6; i++)
    {
      int c = j["cardId"][i];
      if (maskUmaId)
          c = hack_scId(c);
      if (!GameDatabase::AllSupportCardGameIdToSimulatorId.count(c))
        throw "δ֪֧Ԯ��";
      cardId[i] = GameDatabase::AllSupportCardGameIdToSimulatorId.at(c);
    }

    for (int i = 0; i < 8; i++)
      cardJiBan[i] = j["cardJiBan"][i];
    

    for (int i = 0; i < 5; i++)
      trainLevelCount[i] = j["trainLevelCount"][i];
    
    for (int i = 0; i < 5; i++)
      zhongMaBlueCount[i] = j["zhongMaBlueCount"][i];

    for (int i = 0; i < 6; i++)
      zhongMaExtraBonus[i] = j["zhongMaExtraBonus"][i];
    

    // std::cout << "Value load finished\n";


    isRacing = j["isRacing"];
    venusLevelYellow = j["venusLevelYellow"];
    venusLevelRed = j["venusLevelRed"];
    venusLevelBlue = j["venusLevelBlue"];

    for (int i = 0; i < 8; i++)
      venusSpiritsBottom[i] = j["venusSpiritsBottom"][i];

    for (int i = 0; i < 6; i++)
      venusSpiritsUpper[i] = j["venusSpiritsUpper"][i];

    venusAvailableWisdom = j["venusAvailableWisdom"];
    venusIsWisdomActive = j["venusIsWisdomActive"];
    venusCardFirstClick = j["venusCardFirstClick"];
    venusCardUnlockOutgoing = j["venusCardUnlockOutgoing"];
    venusCardIsQingRe = j["venusCardIsQingRe"];
    venusCardQingReContinuousTurns = j["venusCardQingReContinuousTurns"];

    for (int i = 0; i < 5; i++)
      venusCardOutgoingUsed[i] = j["venusCardOutgoingUsed"][i];

    // std::cout << "VenusCard load finished\n";

    stageInTurn = j["stageInTurn"];
    for (int i = 0; i < 5; i++)
      for (int k = 0; k < 8; k++)
      {
        cardDistribution[i][k] = j["cardDistribution"][i][k];
      }

    for (int i = 0; i < 6; i++)
      cardHint[i] = j["cardHint"][i];

    for (int i = 0; i < 8; i++)
      spiritDistribution[i] = j["spiritDistribution"][i];

    // std::cout << "Others load finished\n";

    // 5�������˻��Ŷ�
    if ( GameDatabase::AllSupportCards[cardId[0]].cardType != 5)//1��λ�������ţ���������λ�ã������Ż���1��λ
    {
      int s = -1;//����ԭλ��
      for (int i = 1; i < 6; i++)
      {
        if (GameDatabase::AllSupportCards[cardId[i]].cardType == 5)
        {
          s = i;
          break;
        }
      }
      if (s == -1)
        throw "û������";

      std::swap(cardId[s], cardId[0]);
      std::swap(cardJiBan[s], cardJiBan[0]);

      for (int i = 0; i < 5; i++)
        std::swap(cardDistribution[i][s], cardDistribution[i][0]);

      std::swap(cardHint[s], cardHint[0]);
    }

    // std::cout << "Swap load finished\n";

    initRandomGenerators();
    calculateVenusSpiritsBonus();
    calculateTrainingValue();

  }
  catch (string e)
  {
    cout << "��ȡ��Ϸ��Ϣjson������" << e << endl;
    return false;
  }
  catch (std::exception &e)
  {
    cout << "��ȡ��Ϸ��Ϣjson������δ֪����" << e.what() << endl;
    return false;
  }

  return true;
}
