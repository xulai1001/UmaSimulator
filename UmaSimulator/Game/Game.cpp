#include <iostream>
#include <cassert>
#include "Game.h"
using namespace std;

const std::string Action::trainingName[8] =
{
  "��",
  "��",
  "��",
  "��",
  "��",
  "��Ϣ",
  "���",
  "����"
};

static bool randBool(mt19937_64& rand, double p)
{
  return rand() % 65536 < p * 65536;
}

//������Game���˳��һ��
void Game::newGame(mt19937_64& rand, GameSettings settings, int newUmaId, int umaStars, int newCards[6], int newZhongMaBlueCount[5], int newZhongMaExtraBonus[6])
{
  gameSettings = settings;

  umaId = newUmaId;
  isLinkUma = GameConstants::isLinkChara(umaId);
  if (!GameDatabase::AllUmas.count(umaId))
  {
    throw "ERROR Unknown character. Updating database is required.";
  }
  for (int i = 0; i < TOTAL_TURN; i++)
    isRacingTurn[i] = GameDatabase::AllUmas[umaId].races[i] == TURN_RACE;
  assert(isRacingTurn[11] == true);//������
  //isRacingTurn[TOTAL_TURN - 5] = true;//ura1
  //isRacingTurn[TOTAL_TURN - 3] = true;//ura2
  //isRacingTurn[TOTAL_TURN - 1] = true;//ura3

  for (int i = 0; i < 5; i++)
    fiveStatusBonus[i] = GameDatabase::AllUmas[umaId].fiveStatusBonus[i];

  turn = 0;
  stage = ST_distribute;
  vital = 100;
  maxVital = 100;
  motivation = 3;

  for (int i = 0; i < 5; i++)
    fiveStatus[i] = GameDatabase::AllUmas[umaId].fiveStatusInitial[i] - 10 * (5 - umaStars); //�������ʼֵ
  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] = GameConstants::BasicFiveStatusLimit[i]; //ԭʼ��������

  skillPt = 120;
  skillScore = umaStars >= 3 ? 170 * (umaStars - 2) : 120 * (umaStars);//���м���
  hintSkillLvCount = 0;

  for (int i = 0; i < 5; i++)
  {
    trainLevelCount[i] = 0;
  }

  failureRateBias = 0;
  isQieZhe = false;
  isAiJiao = false;
  isPositiveThinking = false;
  isRefreshMind = false;

  haveCatchedDoll = false;

  for (int i = 0; i < 5; i++)
    zhongMaBlueCount[i] = newZhongMaBlueCount[i];
  for (int i = 0; i < 6; i++)
    zhongMaExtraBonus[i] = newZhongMaExtraBonus[i];

  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] += int(zhongMaBlueCount[i] * 5.34 * 2); //��������--�������ֵ
  for (int i = 0; i < 5; i++)
    addStatus(i, zhongMaBlueCount[i] * 7); //����

  stage = ST_distribute;
  decidingEvent = DecidingEvent_none;
  isRacing = false;

  friendship_noncard_yayoi = 0;
  friendship_noncard_reporter = 0;

  for (int i = 0; i < MAX_INFO_PERSON_NUM; i++)
  {
    persons[i] = Person();
  }

  saihou = 0;
  friend_type = 0;
  friend_personId = PS_none;
  friend_stage = 0;
  for (int i = 0; i < 5; i++)
    friend_outgoingUsed[i] = false;
  friend_vitalBonus = 1.0;
  friend_statusBonus = 1.0;
  friend_qingre = false;
  friend_qingreTurn = 0;
  for (int i = 0; i < 6; i++)
  {
    int cardId = newCards[i];
    persons[i].setCard(cardId);
    saihou += persons[i].cardParam.saiHou;

    if (persons[i].personType == PersonType_scenarioCard)
    {
      friend_personId = i;
      bool isSSR = cardId > 300000;
      if (isSSR)
        friend_type = 1;
      else
        friend_type = 2;
      int friendLevel = cardId % 10;
      assert(friendLevel >= 0 && friendLevel <= 4);
      if (friend_type ==1)
      {
        friend_vitalBonus = GameConstants::FriendVitalBonusSSR[friendLevel];
        friend_statusBonus = GameConstants::FriendStatusBonusSSR[friendLevel];
      }
      else
      {
        friend_vitalBonus = GameConstants::FriendVitalBonusR[friendLevel];
        friend_statusBonus = GameConstants::FriendStatusBonusR[friendLevel];
      }
      friend_vitalBonus += 1e-10;
      friend_statusBonus += 1e-10;//�Ӹ�С����������Ϊ�����������
    }
  }


  for (int i = 0; i < 6; i++)//֧Ԯ����ʼ�ӳ�
  {
    for (int j = 0; j < 5; j++)
      addStatus(j, persons[i].cardParam.initialBonus[j]);
    skillPt += persons[i].cardParam.initialBonus[5];
  }


  lg_mainColor = -1;
  for (int i = 0; i < 3; i++)
    lg_gauge[i] = 0;
  for (int i = 0; i < 10; i++)
    lg_buffs[i] = ScenarioBuffInfo();
  for (int i = 0; i < 57; i++)
    lg_haveBuff[i] = false;

  lg_pickedBuffsNum = 0;
  for (int i = 0; i < 9; i++)
    lg_pickedBuffs[i] = -1;

  lg_blue_active = false;
  lg_blue_remainCount = 0;
  lg_blue_currentStepCount = 0;
  lg_blue_canExtendCount = 0;
  lg_green_todo = 0;

  for (int i = 0; i < 16; i++)
    lg_red_friendsGauge[i] = 0;
  for (int i = 0; i < 16; i++)
    lg_red_friendsLv[i] = 0;

  lg_buffCondition = ScenarioBuffCondition();

  calculateScenarioBonus();
  randomizeTurn(rand); //������俨�飬������������
  
}

void Game::calculateScenarioBonus()
{
  lg_bonus.clear();
  for (int i = 0; i < 10; i++)
    addScenarioBuffBonus(i);
}

void Game::randomizeTurn(std::mt19937_64& rand)
{
  if (stage != ST_distribute)
    throw "������俨��Ӧ����ST_distribute";
  randomDistributeHeads(rand);
  randomInviteHeads(rand, lg_bonus.extraHead);

  //�Ƿ���hint�����ڳ�����ͷ����Ҫ����
  if (!isRacing)
  {
    for (int i = 0; i < 6; i++)
      persons[i].isHint = false;
    float hintProbBonus = lg_bonus.hintProb;

    for (int t = 0; t < 5; t++)
    {
      for (int h = 0; h < 5; h++)
      {
        int pid = personDistribution[t][h];
        if (pid < 0)break;
        if (pid >= 6)continue;

        if (persons[pid].personType == PersonType_card)
        {
          double hintProb = 0.075 * (1 + 0.01 * persons[pid].cardParam.hintProbIncrease) * (1 + 0.01 * hintProbBonus);//��֪���Ǽӻ��ǳˣ��Ȱ�����
          persons[pid].isHint = randBool(rand, hintProb);

        }
      }
    }

    //�����ۡ������ٳ���һ��hint
    int sureHintHead = lg_bonus.alwaysHint ? 1 : 0;
    int triedTimes = 0;
    while (sureHintHead > 0)
    {
      triedTimes++;
      if (triedTimes > 1000)
        throw "sureHintHead����1000��ʧ��";
      int idx = rand() % 6;
      if (persons[idx].personType == PersonType_card)
      {
        persons[idx].isHint = true;
        break;
      }
    }
  }

  //���������ɫ
  for (int i = 0; i < 8; i++)
  {
    lg_trainingColor[i] = rand() % 3;
  }

  calculateTrainingValue();
  stage = ST_train;
}

void Game::undoRandomize()
{
  if (stage == ST_train)
    stage = ST_distribute;
  else if (stage == ST_chooseBuff)
    stage = ST_pickBuff;
}

void Game::randomDistributeHeads(std::mt19937_64& rand)
{
  for (int i = 0; i < 5; i++)
    for (int j = 0; j < 5; j++)
      personDistribution[i][j] = -1;

  //�����غ�
  if (isRacing)
  {
    return;//�������÷��俨��
  }



  int headN[5] = { 0,0,0,0,0 };
  bool friendN[5] = { 0,0,0,0,0 };
  //��һ�������³�/����
  for (int p = 6; p < 6 + 2; p++)
  {
    if (isXiahesu())
      continue;
    if (p == PS_noncardReporter && turn < 12)//����
    {
      continue;
    }
    std::vector<int> probs = { 100,100,100,100,100,200 }; //���������Ǹ�
    auto distribution = std::discrete_distribution<>(probs.begin(), probs.end());
    while (true)
    {
      int atTrain = distribution(rand);
      if (atTrain < 5 && (friendN[atTrain] > 0 || headN[atTrain] >= 5))
        continue; //ѵ���������ˣ����·���
      else
      {
        if (atTrain < 5)
        {
          personDistribution[atTrain][headN[atTrain]] = p;
          headN[atTrain] += 1;
          friendN[atTrain] += 1;
        }
        break;
      }
    }
  }

  vector<int> normalCards;
  for (int card = 0; card < 6; card++)
  {
    if (persons[card].personType == PersonType_card)
      normalCards.push_back(card);
  }
  std::shuffle(normalCards.begin(), normalCards.end(), rand);//��֤���п��ĵ�λ�ǹ�ƽ��


  if (lg_mainColor == L_red)
  {
    //�ڶ�����������
    for (int t = 0; t < normalCards.size(); t++)
    {
      int p = normalCards[t];
      if (lg_red_friendsGauge[p] != 20)continue;
      auto& ps = persons[p];
      int cardType = ps.cardParam.cardType;
      if (headN[cardType] < 5)//���ѵ�����п�λ���ʹ������ѵ��
      {
        personDistribution[cardType][headN[cardType]] = p;
        headN[cardType] += 1;
      }
      else //���������������6���ٿ�����Щ����Ļᵽ����ѵ��
      {
        std::vector<int> probs = { 100,100,100,100,100,50 - int(lg_bonus.disappearRateReduce) }; //���������Ǹ�
        auto distribution = std::discrete_distribution<>(probs.begin(), probs.end());
        while (true)
        {
          int atTrain = distribution(rand);
          if (atTrain < 5 && headN[atTrain] >= 5)
            continue; //ѵ�����ˣ����·���
          else
          {
            if (atTrain < 5)
            {
              personDistribution[atTrain][headN[atTrain]] = p;
              headN[atTrain] += 1;
            }
            break;
          }
        }
      }

    }

    //��������������npc
    for (int p = PS_npc0; p <= PS_npc4; p++)
    {
      if (lg_red_friendsGauge[p] != 20)continue;
      int cardType = p - PS_npc0;
      if (headN[cardType] < 5)//���ѵ�����п�λ���ʹ������ѵ��
      {
        personDistribution[cardType][headN[cardType]] = p;
        headN[cardType] += 1;
      }
      else //���������������6���ٿ�����Щ����Ļᵽ����ѵ��
      {
        std::vector<int> probs = { 100,100,100,100,100,50 - int(lg_bonus.disappearRateReduce) }; //���������Ǹ�
        auto distribution = std::discrete_distribution<>(probs.begin(), probs.end());
        while (true)
        {
          int atTrain = distribution(rand);
          if (atTrain < 5 && headN[atTrain] >= 5)
            continue; //ѵ�����ˣ����·���
          else
          {
            if (atTrain < 5)
            {
              personDistribution[atTrain][headN[atTrain]] = p;
              headN[atTrain] += 1;
            }
            break;
          }
        }
      }

    }
  }

  //���Ĳ�����ͨ��
  for (int t = 0; t < normalCards.size(); t++)
  {
    int p = normalCards[t];
    if (lg_red_friendsGauge[p] == 20)continue;//�Ѿ�����
    auto& ps = persons[p];
    int cardType = ps.cardParam.cardType;
    int deYiLv = ps.cardParam.deYiLv + lg_bonus.deyilv;
    int absentRate = 50 - lg_bonus.disappearRateReduce;
    std::vector<int> probs = { 100,100,100,100,100,absentRate }; //���������Ǹ�
    probs[cardType] += deYiLv;
    auto distribution = std::discrete_distribution<>(probs.begin(), probs.end()); 
    while (true)
    {
      int atTrain = distribution(rand);
      if (atTrain < 5 && headN[atTrain] >= 5)
        continue; //ѵ�����ˣ����·���
      else
      {
        if (atTrain < 5)
        {
          personDistribution[atTrain][headN[atTrain]] = p;
          headN[atTrain] += 1;
        }
        break;
      }
    }
  }

  //���岽�����npc

  if (lg_mainColor == L_red)
  {
    for (int p = PS_npc0; p <= PS_npc4; p++)
    {
      if (lg_red_friendsGauge[p] == 20)continue;//�Ѿ�����
      int cardType = p - PS_npc0;
      int deYiLv = lg_bonus.deyilv;
      int absentRate = 50 - lg_bonus.disappearRateReduce;
      std::vector<int> probs = { 100,100,100,100,100,absentRate }; //���������Ǹ�
      probs[cardType] += deYiLv;
      auto distribution = std::discrete_distribution<>(probs.begin(), probs.end()); 
      while (true)
      {
        int atTrain = distribution(rand);
        if (atTrain < 5 && headN[atTrain] >= 5)
          continue; //ѵ�����ˣ����·���
        else
        {
          if (atTrain < 5)
          {
            personDistribution[atTrain][headN[atTrain]] = p;
            headN[atTrain] += 1;
          }
          break;
        }
      }
    }
  }

  //���������ſ����˿�
  for (int p = 0; p < 6; p++)
  {
    auto& ps = persons[p];
    if (ps.personType == PersonType_card)continue;
    int cardType = ps.cardParam.cardType;
    
    int absentRate = 100 - lg_bonus.disappearRateReduce;
    std::vector<int> probs = { 100,100,100,100,100,absentRate }; //���������Ǹ�
    auto distribution = std::discrete_distribution<>(probs.begin(), probs.end()); while (true)
    {
      int atTrain = distribution(rand);
      if (atTrain < 5 && (headN[atTrain] >= 5 || friendN[atTrain] > 0))
        continue; //ѵ�����ˣ����·���
      else
      {
        if (atTrain < 5)
        {
          personDistribution[atTrain][headN[atTrain]] = p;
          headN[atTrain] += 1;
        }
        break;
      }
    }
  }

}

void Game::randomInviteHeads(std::mt19937_64& rand, int num)
{
  if (num == 0)return;

  vector<int> normalCards;
  for (int card = 0; card < 6; card++)
  {
    if (persons[card].personType == PersonType_card)
      normalCards.push_back(card);
  }

  int normalCardsToInvite = num;
  int friendCardsToInvite = 0;
  if (normalCardsToInvite > normalCards.size())
  {
    if (num > 6)num = 6;
    friendCardsToInvite = num - normalCards.size();
    normalCardsToInvite = normalCards.size();
  }

  std::shuffle(normalCards.begin(), normalCards.end(), rand);//ҡǰnum����
  for (int i = 0; i < normalCardsToInvite; i++)
    inviteOneHead(rand, normalCards[i]);
  
  if (friendCardsToInvite > 0) //Ҫҡ���˿���
  {
    vector<int> friendCards;
    for (int card = 0; card < 6; card++)
    {
      if (persons[card].personType != PersonType_card)
        friendCards.push_back(card);
    }
    std::shuffle(friendCards.begin(), friendCards.end(), rand);//ҡǰnum����
    for (int i = 0; i < friendCardsToInvite; i++)
      inviteOneHead(rand, friendCards[i]);
  }


}

void Game::inviteOneHead(std::mt19937_64& rand, int idx)
{
  bool isNormalCard = persons[idx].personType == PersonType_card;
  int triedTimes = 0;
  while (true)
  {
    triedTimes++;
    if (triedTimes > 1000)
    {
      //�ǳ����˵������ҡ6��ͷ����Ҫҡ�ſ�������2��ѵ�����ˣ�2��ѵ�����³����ߣ�1��ѵ�����ſ��Լ�
      //Լ50���һ��
      return;
      //throw "inviteOneHead����1000��ʧ��";
    }
    int atTrain = rand() % 5;
    //����Ƿ�Ϲ�
    bool isOK = true;
    for (int i = 0; i < 5; i++)
    {
      int t = personDistribution[atTrain][i];
      if (t < 0)break;
      if (i == 4)//����
      {
        isOK = false;
        break;
      }
      if (t == idx || //�ظ�
        (!isNormalCard &&  //����/�ſ���������������ſ�/���³�������һ��
          (t == PS_noncardYayoi || t == PS_noncardReporter || ((t < 6 && t >= 0) && persons[t].personType != PersonType_card))))
      {
        isOK = false;
        break;
      }
    }

    if (isOK)
    {
      bool suc = false;
      for (int i = 0; i < 5; i++)
      {
        int t = personDistribution[atTrain][i];
        if (t < 0)
        {
          personDistribution[atTrain][i] = idx;
          suc = true;
          break;
        }
      }
      if (!suc)
        throw "inviteOneHead������ͷʧ��";


      break;
    }
    else //���·���
      continue;
  }
}

//��Ҫ��ǰ����calculateScenarioBonus()
void Game::calculateTrainingValue()
{
  for (int i = 0; i < 5; i++)
    calculateTrainingValueSingle(i);
}
void Game::addTrainingLevelCount(int trainIdx, int n)
{
  trainLevelCount[trainIdx] += n;
  if (trainLevelCount[trainIdx] > 16)trainLevelCount[trainIdx] = 16;
}
int Game::calculateRealStatusGain(int value, int gain) const//����1200����Ϊ2�ı�����ʵ����������ֵ
{
  int newValue = value + gain;
  if (newValue <= 1200)return gain;
  if (gain == 1)return 2;
  return (newValue / 2) * 2 - value;
}
void Game::addStatus(int idx, int value)
{
  assert(idx >= 0 && idx < 5);
  int t = fiveStatus[idx] + value;
  
  if (t > fiveStatusLimit[idx])
    t = fiveStatusLimit[idx];
  if (t < 1)
    t = 1;
  if (t > 1200)
    t = (t / 2) * 2;
  fiveStatus[idx] = t;
}
void Game::addVital(int value)
{
  vital += value;
  if (vital > maxVital)
    vital = maxVital;
  if (vital < 0)
    vital = 0;
}
void Game::addVitalMax(int value)
{
  maxVital += value;
  if (maxVital > 120)
    maxVital = 120;
}
void Game::addMotivation(int value)
{
  if (value < 0)
  {
    if (isPositiveThinking)
      isPositiveThinking = false;
    else
    {
      motivation += value;
      if (motivation < 1)
        motivation = 1;
    }
  }
  else
  {
    motivation += value;
    if (motivation > 5)
      motivation = 5;
    //TODO ����
  }
}
void Game::addJiBan(int idx, int value, int type) //type0�ǵ����type1���¼���type2��û���κ��ӳ�
{
  if(idx==PS_noncardYayoi)
    friendship_noncard_yayoi += value;
  else if (idx == PS_noncardReporter)
    friendship_noncard_reporter += value;
  else
  {
    int gain = value;
    if (type == 0)
    {
      gain += lg_bonus.jibanAdd1;
      gain += lg_bonus.jibanAdd2;
      if (isAiJiao)
        gain += 2;
    }
    else if(type == 1)
    {
      gain += lg_bonus.jibanAdd1;
      if (isAiJiao)
        gain += 2;
    }
    if (idx < 6)
    {
      auto& p = persons[idx];
      persons[idx].friendship += gain;
      if (p.friendship > 100)p.friendship = 100;
    }
    if (lg_mainColor == L_red)
    {
      lg_red_friendsGauge[idx] += gain;
      if (lg_red_friendsGauge[idx] > 20)lg_red_friendsGauge[idx] = 20;
    }
    //else
    //  throw "ERROR: Game::addJiBan Unknown person id";
  }
}
void Game::addAllStatus(int value)
{
  for (int i = 0; i < 5; i++)addStatus(i, value);
}
int Game::calculateFailureRate(int trainType, double failRateMultiply) const
{
  //������ϵ�ѵ��ʧ���ʣ����κ��� A*(x0-x)^2+B*(x0-x)
  //���Ӧ����2%����
  static const double A = 0.025;
  static const double B = 1.25;
  double x0 = 0.1 * GameConstants::FailRateBasic[trainType][getTrainingLevel(trainType)];
  
  double f = 0;
  if (vital < x0)
  {
    f = (100 - vital) * (x0 - vital) / 40.0;
  }
  if (f < 0)f = 0;
  if (f > 99)f = 99;//����ϰ���֣�ʧ�������99%
  f *= failRateMultiply;//֧Ԯ����ѵ��ʧ�����½�����
  int fr = ceil(f);
  fr += failureRateBias;
  if (fr < 0)fr = 0;
  if (fr > 100)fr = 100;
  return fr;
}
void Game::runRace(int basicFiveStatusBonus, int basicPtBonus)
{
  double raceMultiply = 1 + 0.01 * saihou;
  double dishMultiply = 1.0;
  //dish race bonus

  int fiveStatusBonus = int(dishMultiply * int(raceMultiply * basicFiveStatusBonus));
  int ptBonus = int(dishMultiply * int(raceMultiply * basicPtBonus));
  //cout << fiveStatusBonus << " " << ptBonus << endl;
  addAllStatus(fiveStatusBonus);
  skillPt += ptBonus;
}

void Game::addStatusFriend(int idx, int value)
{
  value = int(value * friend_statusBonus);
  if (idx == 5)skillPt += value;
  else addStatus(idx, value);
}

void Game::addVitalFriend(int value)
{
  value = int(value * friend_vitalBonus);
  addVital(value);
}

void Game::handleOutgoing(std::mt19937_64& rand)
{
  assert(stage == ST_train);
  if (isXiahesu())
  {
    stage = ST_pickBuff;
    addVital(40);
    addMotivation(1);
    if (failureRateBias > 0)failureRateBias = 0;//����ϰ����
    addLgGauge(lg_trainingColor[T_outgoing], 1);
  }
  else if (friend_type != 0 &&  //�������˿�
    friend_stage == FriendStage_afterUnlockOutgoing &&  //�ѽ������
    !friend_outgoingUsed[4]  //���û����
    )
  {
    //�ȴ�ѡ�����˳���
    stage = ST_decideEvent;
    decidingEvent = DecidingEvent_outing;
  }
  else //��ͨ����
  {
    stage = ST_pickBuff;
    runNormalOutgoing(rand);
  }
}

void Game::runNormalOutgoing(std::mt19937_64& rand)
{
  //���ò�����ˣ���50%��2���飬50%��1����10����
  if (rand() % 2)
    addMotivation(2);
  else
  {
    addMotivation(1);
    addVital(10);
  }

  //ץ����
  if (turn >= 24 && (!haveCatchedDoll) && (rand() % 3 == 0))
  {
    addVital(15);
    addMotivation(1);
    haveCatchedDoll = true;
  }
  addLgGauge(lg_trainingColor[T_outgoing], 1);
}

void Game::runFriendOutgoing(std::mt19937_64& rand, int idx, int subIdx = -1)
{
  assert(friend_type!=0 && friend_stage >= FriendStage_afterUnlockOutgoing && !friend_outgoingUsed[idx]);
  int pid = friend_personId;
  friend_outgoingUsed[idx] = true;
  if (idx == 0)
  {
    addVitalMax(4);
    addVitalFriend(45);
    addMotivation(1);
    addStatusFriend(1, 15);
    addStatusFriend(2, 10);
    addStatusFriend(3, 10);
    addStatusFriend(5, 20);
    addJiBan(pid, 5, 1);
    addLgGauge(0, 3);
    addLgGauge(1, 3);
    addLgGauge(2, 3);
  }
  else if (idx == 1)
  {
    addVitalFriend(35);
    addMotivation(1);
    addStatusFriend(0, 10);
    addStatusFriend(1, 10);
    addStatusFriend(2, 10);
    addStatusFriend(3, 10);
    addStatusFriend(4, 10);
    addStatusFriend(5, 25);
    addJiBan(pid, 5, 1);
    addLgGauge(0, 3);
    addLgGauge(1, 3);
    addLgGauge(2, 3);
  }
  else if (idx == 2)
  {
    addVitalFriend(45);
    addMotivation(1);
    addStatusFriend(0, 15);
    addStatusFriend(4, 10);
    addStatusFriend(5, 30);
    addJiBan(pid, 5, 1);
    addLgGauge(0, 3);
    addLgGauge(1, 3);
    addLgGauge(2, 3);
  }
  else if (idx == 3)
  {
    if (subIdx < 0 || subIdx >= 3)
      throw "���Ķγ�����Ҫָ����ɫ";

    addVitalFriend(45);
    addMotivation(1);
    addJiBan(pid, 5, 1);
    addLgGauge(subIdx, 8);
    if (subIdx == 0)
    {
      addStatusFriend(0, 5);
      addStatusFriend(1, 5);
      addStatusFriend(2, 5);
      addStatusFriend(3, 5);
      addStatusFriend(4, 5);
    }
    else if (subIdx == 1)
    {
      addStatusFriend(1, 15);
      addStatusFriend(2, 10);
    }
    else if (subIdx == 2)
    {
      addStatusFriend(0, 10);
      addStatusFriend(3, 15);
    }

  }
  else if (idx == 4)
  {
    addVitalFriend(50);
    addMotivation(1);
    addStatusFriend(0, 8);
    addStatusFriend(1, 8);
    addStatusFriend(2, 8);
    addStatusFriend(3, 8);
    addStatusFriend(4, 8);
    addStatusFriend(5, 30);
    addJiBan(pid, 5, 1);
    addLgGauge(0, 3);
    addLgGauge(1, 3);
    addLgGauge(2, 3);
    skillPt += 50;//����
    friend_qingre = true;
    friend_qingreTurn = 0;

  }
  else throw "δ֪�ĳ���";

}
void Game::runFriendClickEvent(std::mt19937_64& rand, int idx)
{
  addJiBan(friend_personId, 5, 1);
  addLgGauge(idx, 1);
  if (friend_qingre)
    skillPt += 6;
  if (!friend_qingre && friend_stage == FriendStage_afterUnlockOutgoing)
  {
    friend_qingre = true;
    friend_qingreTurn = 0;
  }

  if (idx == 0)
  {
    addStatusFriend(0, 5);
  }
  else if (idx == 1)
  {
    addVital(3);
    skillPt += 3;
  }
  else if (idx == 2)
  {
    addStatusFriend(4, 3);

    //�����͵��˼�1�
    int minJiBan = 10000;
    for (int i = 0; i < 6; i++)
    {
      if (persons[i].personType == PersonType_card)
      {
        if (persons[i].friendship < minJiBan)
        {
          minJiBan = persons[i].friendship;
        }
      }
    }

    //����ж�����ͬ����ͷ�������Ƿ۵Ǻ��ڣ��������һ�����
    std::vector<int> candidates;
    for (int i = 0; i < 6; ++i) {
      if (persons[i].personType == PersonType_card)
      {
        if (persons[i].friendship == minJiBan)
          candidates.push_back(i);
      }
    }

    if (candidates.size() > 0)
    {
      int minJiBanId = candidates[rand() % candidates.size()];
      addJiBan(minJiBanId, 1, 1);
      printEvents("���˵���¼�:" + persons[minJiBanId].getPersonName() + " �+1");
    }
    else
      throw "û�п��Լ�����ͷ��6�����ˣ�";
  }

}
void Game::handleFriendUnlock(std::mt19937_64& rand)
{
  assert(friend_stage == FriendStage_beforeUnlockOutgoing);
  addVitalFriend(35);
  addStatusFriend(5, 10);
  friend_stage = FriendStage_afterUnlockOutgoing;
  isPositiveThinking = true;
  addJiBan(friend_personId, 5, 1);
  for (int i = 0; i < 3; i++)
    addLgGauge(i, 3);
  friend_qingre = true;
  friend_qingreTurn = 0;


  printEvents("�������������");
  
}
void Game::handleFriendClickEvent(std::mt19937_64& rand, int atTrain)
{
  assert(friend_type!=0 && (friend_personId<6&& friend_personId>=0) && persons[friend_personId].personType==PersonType_scenarioCard);
  if (friend_stage == FriendStage_notClicked)
  {
    printEvents("��һ�ε�����");
    friend_stage = FriendStage_beforeUnlockOutgoing;
    
    for (int i = 0; i < 5; i++)
      addStatusFriend(i, 5);
    addJiBan(friend_personId, 10, 1);
    addMotivation(1);

    for (int i = 0; i < 3; i++)
      addLgGauge(i, 3);
  }
  else
  {
    if ((!friend_qingre) && rand() % 5 < 3)return;//������ʱ40%���ʳ��¼���60%���ʲ���

    stage = ST_decideEvent;
    decidingEvent=DecidingEvent_three;

  }

}
void Game::handleFriendFixedEvent()
{
  if (friend_type == 0)return;//û���˿�
  if (friend_stage < FriendStage_beforeUnlockOutgoing)return;//����û������û�¼�
  if (turn == 23)
  {
    addVitalMax(4);
    addMotivation(1);
    for (int i = 0; i < 5; i++)
      addStatusFriend(i, 4);
    addStatusFriend(5, 5);
    addJiBan(friend_personId, 5, 1);
    skillPt += 40;//�������ܣ������н�����������hint����Ч��
    for (int i = 0; i < 3; i++)
      addLgGauge(i, 2);
  }
  else if (turn == TOTAL_TURN - 1)
  {
    if (friend_outgoingUsed[4])
    {
      for (int i = 0; i < 6; i++)
        addStatusFriend(i, 12);
    }
    else
    {
      for (int i = 0; i < 6; i++)
        addStatusFriend(i, 8);
    }
  }
  else
  {
    assert(false && "�����غ�û�����˹̶��¼�");
  }
}
bool Game::applyTraining(std::mt19937_64& rand, int16_t train)
{
  assert(stage == ST_train);

  bool trainingSucceed = false;

  if (isRacing)
  {
    //�̶�����������checkEventAfterTrain()�ﴦ��
    stage = ST_pickBuff;
    assert(train == T_race);
    addLgGauge(lg_trainingColor[T_race], 1);
  }
  else
  {
    if (train == T_rest)//��Ϣ
    {
      stage = ST_pickBuff;
      if (isXiahesu())//����ֻ�����
      {
        return false;
      }
      else
      {
        int r = rand() % 100;
        if (r < 25)
          addVital(70);
        else if (r < 82)
          addVital(50);
        else
          addVital(30);
      }
      addLgGauge(lg_trainingColor[T_rest], 1);
    }
    else if (train == T_race)//����
    {
      stage = ST_pickBuff;
      if (turn <= 12 || turn >= 72)
      {
        printEvents("Cannot race now.");
        return false;
      }
      //addAllStatus(1);//������
      runRace(2, 30);//���ԵĽ���

      //����̶�15
      addVital(-15);
      if (rand() % 5 == 0)
        addMotivation(1);

      addLgGauge(lg_trainingColor[T_race], 1);
    }
    else if (train == T_outgoing)//���
    {
      handleOutgoing(rand);
    }
    else if (train <= 4 && train >= 0)//����ѵ��
    {
      if (rand() % 100 < failRate[train])//ѵ��ʧ��
      {
        trainingSucceed = false;
        applyNormalTraining(rand, train, trainingSucceed);
      }
      else
      {
        trainingSucceed = true;
        applyNormalTraining(rand, train, trainingSucceed);
      }

    }
    else
    {
      printEvents("δ֪��ѵ����Ŀ");
      return false;
    }
  }

  updateScenarioBuffAfterTrain(train, trainingSucceed);

  if (stage == ST_pickBuff)
    maybeSkipPickBuffStage();//����û�������¼����ſ���ѡһ����������������Ƿ�ý���ѡbuff���ڣ������ȴ����¼��ټ�顣

  return true;
}

void Game::applyNormalTraining(std::mt19937_64& rand, int16_t train, bool success)
{
  assert(stage == ST_train);
  stage = ST_pickBuff;//�п��ܱ�handleFriendClickEvent(rand, train)�޸�
  if (!success)
  {
    if (failRate[train] >= 20 && (rand() % 100 < failRate[train]))//ѵ����ʧ�ܣ�������Ϲ�µ�
    {
      printEvents("ѵ����ʧ�ܣ�");
      addStatus(train, -10);
      if (fiveStatus[train] > 1200)
        addStatus(train, -10);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
      //�����2��10�������ĳ�ȫ����-4���������
      for (int i = 0; i < 5; i++)
      {
        addStatus(i, -4);
        if (fiveStatus[i] > 1200)
          addStatus(i, -4);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
      }
      addMotivation(-3);
      addVital(10);
    }
    else//Сʧ��
    {
      printEvents("ѵ��Сʧ�ܣ�");
      addStatus(train, -5);
      if (fiveStatus[train] > 1200)
        addStatus(train, -5);//��Ϸ��1200���Ͽ����Բ��۰룬�ڴ�ģ�������Ӧ1200���Ϸ���
      addMotivation(-1);
    }
    addLgGauge(lg_trainingColor[T_race], 1);
  }
  else
  {
    //�ȼ���ѵ��ֵ
    for (int i = 0; i < 5; i++)
      addStatus(i, trainValue[train][i]);
    skillPt += trainValue[train][5];
    addVital(trainVitalChange[train]);

    bool clickFriend = false;
    vector<int> hintCards;//���ļ����������̾����
    
    for (int i = 0; i < 5; i++)
    {
      int p = personDistribution[train][i];
      if (p < 0)break;//û��

      if (p == friend_personId && friend_type != 0)//���˿�
      {
        assert(persons[p].personType == PersonType_scenarioCard);
        addJiBan(p, 4, 0);
        clickFriend = true;
      }
      else if (p < 6)//��ͨ��
      {
        addJiBan(p, 7, 0);
        if (persons[p].isHint)
          hintCards.push_back(p);

        //���������ͷ���
        if (lg_mainColor == L_red && isCardShining(p, train) && lg_red_friendsGauge[p] == 20)
        {
          lg_red_friendsGauge[p] = 0;
          if (lg_red_friendsLv[p] < 9)
            lg_red_friendsLv[p] += 1;
        }
      }
      else if (p >= PS_npc0 && p <= PS_npc4)//npc
      {
        addJiBan(p, 7, 0);
        //���������ͷ���
        if (lg_mainColor == L_red && isCardShining(p, train) && lg_red_friendsGauge[p] == 20)
        {
          lg_red_friendsGauge[p] = 0;
          if (lg_red_friendsLv[p] < 9)
            lg_red_friendsLv[p] += 1;
        }
      }
      else if (p == PS_noncardYayoi)//�ǿ����³�
      {
        int jiban = friendship_noncard_yayoi;
        int g = jiban < 40 ? 2 : jiban < 60 ? 3 : jiban < 80 ? 4 : 5;
        skillPt += g;
        addJiBan(PS_noncardYayoi, 7, 0);
      }
      else if (p == PS_noncardReporter)//����
      {
        int jiban = friendship_noncard_reporter;
        int g = jiban < 40 ? 2 : jiban < 60 ? 3 : jiban < 80 ? 4 : 5;
        addStatus(train, g);
        addJiBan(PS_noncardReporter, 7, 0);
      }
      else
      {
        //��������/�ſ��ݲ�֧��
        assert(false);
      }
    }



    if (hintCards.size() > 0)
    {
      int hintCard = hintCards[rand() % hintCards.size()];//���һ�ſ���hint

      addJiBan(hintCard, 5, 1);

      int hintNum = lg_bonus.moreHint + 1;
      for (int i = 0; i < hintNum; i++)
        addHintWithoutJiban(rand, hintCard);

    }


    //ѵ���ȼ�����
    if (!isXiahesu())
      addTrainingLevelCount(train, 1);

    int gaugeGain = trainShiningNum[train] > 0 ? 3 : 1;
    addLgGauge(lg_trainingColor[train], gaugeGain);


    if (clickFriend)
      handleFriendClickEvent(rand, train);

  }
}
void Game::addHintWithoutJiban(std::mt19937_64& rand, int idx)
{
  int hintLevel = persons[idx].cardParam.hintLevel;
  int cardType = persons[idx].cardParam.cardType;
  assert(cardType < 5 && cardType >= 0);
  double skillProb = 0.9 * (1 - exp(-exp(2 - hintSkillLvCount / gameSettings.hintProbTimeConstant)));//�ж������Ǹ����ܶ���������
  if (hintLevel == 0)skillProb = 0;//�����������֣�ֻ������

  if (randBool(rand, skillProb))
  {
    hintSkillLvCount += hintLevel;

    skillPt += int(hintLevel * gameSettings.hintPtRate);
  }
  else 
  {
    if (cardType == 0)
    {
      addStatus(0, 6);
      addStatus(2, 2);
    }
    else if (cardType == 1)
    {
      addStatus(1, 6);
      addStatus(3, 2);
    }
    else if (cardType == 2)
    {
      addStatus(2, 6);
      addStatus(1, 2);
    }
    else if (cardType == 3)
    {
      addStatus(3, 6);
      addStatus(0, 1);
      addStatus(2, 1);
    }
    else if (cardType == 4)
    {
      addStatus(4, 6);
      skillPt += 5;
    }
    else
      throw "�����Ŷӿ�����hint";
  }
}
void Game::jicheng(std::mt19937_64& rand)
{
  for (int i = 0; i < 5; i++)
    addStatus(i, zhongMaBlueCount[i] * 6); //�����ӵ���ֵ

  double factor = double(rand() % 65536) / 65536 * 2;//�籾�������0~2��
  for (int i = 0; i < 5; i++)
    addStatus(i, int(factor * zhongMaExtraBonus[i])); //�籾����
  skillPt += int((0.5 + 0.5 * factor) * zhongMaExtraBonus[5]);//���߰��㼼�ܵĵ�Чpt

  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] += zhongMaBlueCount[i] * 2; //��������--�������ֵ��18�����μ̳й��Ӵ�Լ36���ޣ�ÿ��ÿ��������+1���ޣ�1200�۰��ٳ�2

  for (int i = 0; i < 5; i++)
    fiveStatusLimit[i] += rand() % 8; //��������--�����μ̳��������

}
void Game::updateScenarioBuffAfterTrain(int16_t trainIdx, bool trainSucceed)
{
  lg_buffCondition.clear();
  if (trainIdx == T_outgoing || trainIdx == T_rest)
  {
    lg_buffCondition.isRest = true;
  }
  else if (trainIdx >= 0 && trainIdx < 5)
  {
    lg_buffCondition.isTraining = true;
    lg_buffCondition.trainingSucceed = trainSucceed;
    if (trainSucceed)
    {
      lg_buffCondition.trainingHead = trainHeadNum[trainIdx];
      lg_buffCondition.isYouqing = trainShiningNum[trainIdx];
    }
  }
  for (int i = 0; i < 10; i++)
  {
    updateScenarioBuffCondition(i);
  }
}


void Game::maybeSkipPickBuffStage()
{
  assert(stage == ST_pickBuff);
  if (turn % 6 == 5 && turn <= 65)
  {
    //randomPickBuff(rand);
    //stage = ST_chooseBuff;
  }
  else
    stage = ST_event;
}

void Game::decideEvent(std::mt19937_64& rand, int16_t idx)
{
  if (decidingEvent == DecidingEvent_three)
  {
    decideEvent_three(rand, idx);
  }
  else if (decidingEvent == DecidingEvent_outing)
  {
    decideEvent_outing(rand, idx);
  }
  else throw "unknown decidingEvent";



  stage = ST_pickBuff;
  maybeSkipPickBuffStage();
}

//idx 01234567��������ͨ������������123���������4������ѡ��������5
void Game::decideEvent_outing(std::mt19937_64& rand, int16_t idx)
{
  if (!friend_type == 1 || friend_outgoingUsed[4] || isXiahesu())
  {
    throw "decideEvent_outing�������������";
  }
  if (idx == 0)
    runNormalOutgoing(rand);
  else if (idx == 1)
    runFriendOutgoing(rand, 0);
  else if (idx == 2)
    runFriendOutgoing(rand, 1);
  else if (idx == 3)
    runFriendOutgoing(rand, 2);
  else if (idx == 4)
    runFriendOutgoing(rand, 3, 0);
  else if (idx == 5)
    runFriendOutgoing(rand, 3, 1);
  else if (idx == 6)
    runFriendOutgoing(rand, 3, 2);
  else if (idx == 7)
    runFriendOutgoing(rand, 4);
}

void Game::decideEvent_three(std::mt19937_64& rand, int16_t idx)
{
  if (friend_stage != FriendStage_beforeUnlockOutgoing && friend_stage != FriendStage_afterUnlockOutgoing)
    throw "��һ�ε�����ᴥ����ѡһ�¼�";
  runFriendClickEvent(rand, idx);
}


bool Game::isLegal(Action action) const
{
  if (isRacing)
  {
    //if (isUraRace)
    //{
      if (action.stage==ST_train && action.idx == T_race)
        return true;
      else
        return false;
    //}
    //else
    //{
      //assert(false && "����ura����ľ籾��������checkEventAfterTrain()�ﴦ������applyTraining");
      //return false;//���о籾��������checkEventAfterTrain()�ﴦ���൱�ڱ����غ�ֱ���������������������
    //}
  }

  if (action.idx == T_rest)
  {
    if (isXiahesu())
    {
      return false;//���ĺ��޵ġ����&��Ϣ����Ϊ���
    }
    return true;
  }
  else if (action.idx == T_outgoing)
  {
    return true;
  }
  else if (action.idx == T_race)
  {
    return isRaceAvailable();
  }
  else if (action.idx >= 0 && action.idx <= 4)
  {
    return true;
  }
  else
  {
    assert(false && "δ֪��ѵ����Ŀ");
    return false;
  }
  return false;
}



float Game::getSkillScore() const
{
  float rate = isQieZhe ? gameSettings.ptScoreRate * 1.1 : gameSettings.ptScoreRate ;
  return rate * skillPt + skillScore;
}

static double scoringFactorOver1200(double x)//����ʤ������ɫʮ�֣�׷��
{
  if (x <= 1150)return 0;
  return tanh((x - 1150) / 100.0) * sqrt(x - 1150);
}

static double realRacingStatus(double x)
{
  if (x < 1200)return x;
  return 1200 + (x - 1200) / 4;
}

static double smoothUpperBound(double x)
{
  return (x - sqrt(x * x + 1)) / 2;
}

int Game::finalScore_mile() const
{
  double weights[5] = { 400,300,70,70,120 };
  double weights1200[5] = { 0,0,20,10,0 };


  double staminaTarget = 900;
  double staminaBonus = 5 * 100 * (smoothUpperBound((realRacingStatus(fiveStatus[1]) - staminaTarget) / 100.0) - smoothUpperBound((0 - staminaTarget) / 100.0));

  double total = 0;
  total += staminaBonus;
  for (int i = 0; i < 5; i++)
  {
    double realStat = realRacingStatus(min(fiveStatus[i], fiveStatusLimit[i]));
    total += weights[i] * sqrt(realStat);
    total += weights1200[i] * scoringFactorOver1200(realStat);
  }

  total += getSkillScore();
  if (total < 0)total = 0;
  //return uaf_haveLose ? 10000 : 20000;
  return (int)total;
}

int Game::finalScore_sum() const
{
  double weights[5] = { 5,3,3,3,3 };
  double total = 0;
  for (int i = 0; i < 5; i++)
  {
    double realStat = min(fiveStatus[i], fiveStatusLimit[i]);
    if (realStat > 1200)realStat = 1200 + (realStat - 1200) / 2;
    total += weights[i] * realStat;
  }

  total += getSkillScore();
  if (total < 0)total = 0;
  return (int)total;
}

int Game::finalScore_rank() const
{
  int total = 0;
  for (int i = 0; i < 5; i++)
    total += GameConstants::FiveStatusFinalScore[min(fiveStatus[i], fiveStatusLimit[i])];

  total += int(getSkillScore());
  //return uaf_haveLose ? 10000 : 20000;
  return total;
}

int Game::finalScore() const
{
  if (gameSettings.scoringMode == SM_normal)
  {
    return finalScore_rank();
  }
  else if (gameSettings.scoringMode == SM_race)
  {
    return finalScore_sum();
  }
  else if (gameSettings.scoringMode == SM_mile)
  {
    return finalScore_mile();
  }
  else
  {
    throw "�������㷨��δʵ��";
  }
  return 0;
}

bool Game::isEnd() const
{
  return turn >= TOTAL_TURN;
}

int Game::getTrainingLevel(int item) const
{
  if (isXiahesu())return 4;

  return trainLevelCount[item] / 4;
}

void Game::calculateTrainingValueSingle(int tra)
{
  int headNum = 0;//���ſ�����npc�����³����߲���
  int shiningNum = 0;//��������
  int linkNum = 0;//����link

  int basicValue[6] = { 0,0,0,0,0,0 };//ѵ���Ļ���ֵ��=ԭ����ֵ+֧Ԯ���ӳ�

  double totalXunlian = 0;//�²㣬ѵ��1+ѵ��2+...
  double totalXunlianUpper = lg_bonus.xunlian;//=�²�+�籾+npc
  double totalGanjing = 0;//�ɾ�1+�ɾ�2+...
  double totalGanjingUpper = lg_bonus.ganjing;//=�²�+�籾
  double totalYouqingMultiplier = 1.0;//(1+����1)*(1+����2)*...
  double totalYouqingUpper = lg_bonus.youqing;//�籾����ӳ�=�籾+npc
  int vitalCostBasic;//�������Ļ�������=ReLU(������������+link������������-�ǲ��������ļ���)
  double vitalCostMultiplier = 1.0;//(1-�������ļ�����1)*(1-�������ļ�����2)*...
  double failRateMultiplier = 1.0;//(1-ʧ�����½���1)*(1-ʧ�����½���2)*...




  int tlevel = getTrainingLevel(tra);


  for (int h = 0; h < 5; h++)
  {
    int pIdx = personDistribution[tra][h];
    if (pIdx < 0)break;
    if (pIdx == PS_noncardYayoi || pIdx == PS_noncardReporter)continue;//���ǿ�

    headNum += 1;

    if (isCardShining(pIdx, tra))
    {
      shiningNum += 1;
    }

    if (pIdx < 6)
    {
      const Person& p = persons[pIdx];
      if (p.cardParam.isLink)
      {
        linkNum += 1;
      }
    }
  }
  trainShiningNum[tra] = shiningNum;
  trainHeadNum[tra] = headNum;

  totalXunlianUpper += lg_bonus.xunlianPerHead * headNum;
  totalGanjingUpper += lg_bonus.ganjingPerHead * headNum;
  totalYouqingUpper += lg_bonus.youqingPerShiningHead * shiningNum;

  //����ֵ
  for (int i = 0; i < 6; i++)
    basicValue[i] = GameConstants::TrainingBasicValue[tra][tlevel][i];
  vitalCostBasic = -GameConstants::TrainingBasicValue[tra][tlevel][6];

  for (int h = 0; h < 5; h++)
  {
    int pid = personDistribution[tra][h];
    if (pid < 0)break;//û��
    if (pid == PS_noncardYayoi || pid == PS_noncardReporter)continue;//���ǿ�

    if (pid >= PS_npc0 && pid <= PS_npc4)
    {
      int lv = lg_red_friendsLv[pid]; 
      totalXunlianUpper += GameConstants::LG_redLvXunlianNPC[lv];
      if (isCardShining(pid, tra))//���npc��û��
      {
        totalYouqingUpper += GameConstants::LG_redLvYouqingNPC[lv];
      }
    }
    else if (pid < 6)
    {
      int lv = lg_red_friendsLv[pid];
      totalXunlianUpper += GameConstants::LG_redLvXunlianCard[lv];

      const Person& p = persons[pid];
      bool isThisCardShining = isCardShining(pid, tra);//���ſ���û��
      bool isThisTrainingShining = shiningNum > 0;//���ѵ����û��
      CardTrainingEffect eff = p.cardParam.getCardEffect(*this, isThisCardShining, tra, p.friendship, p.cardRecord, headNum, shiningNum);

      for (int i = 0; i < 6; i++)//����ֵbonus
      {
        if (basicValue[i] > 0)
          basicValue[i] += int(eff.bonus[i]);
      }
      if (isThisCardShining)//���ʣ�����ӳɺ��ǲʻظ�
      {
        totalYouqingMultiplier *= (1 + 0.01 * eff.youQing);
        if (tra == T_wiz)
          vitalCostBasic -= eff.vitalBonus;
      }
      totalXunlian += eff.xunLian;
      totalXunlianUpper += eff.xunLian;
      totalGanjing += eff.ganJing;
      totalGanjingUpper += eff.ganJing;
      vitalCostMultiplier *= (1 - 0.01 * eff.vitalCostDrop);
      failRateMultiplier *= (1 - 0.01 * eff.failRateDrop);
    }

  }

  //������ʧ����

  vitalCostMultiplier *= (1 - 0.01 * lg_bonus.vitalReduce);
  int vitalChangeInt = vitalCostBasic > 0 ? -int(vitalCostBasic * vitalCostMultiplier) : -vitalCostBasic;
  if (vitalChangeInt > maxVital - vital)vitalChangeInt = maxVital - vital;
  if (vitalChangeInt < -vital)vitalChangeInt = -vital;
  trainVitalChange[tra] = vitalChangeInt;
  failRate[tra] = calculateFailureRate(tra, failRateMultiplier);


  //��ͷ * ѵ�� * �ɾ� * ����    //֧Ԯ������
  double motivationFactor = lg_blue_active ? 0.55 : 0.1 * (motivation - 3);
  double totalYouqingUpperMultiplier = shiningNum > 0 ? (1 + 0.01 * totalYouqingUpper) : 1;
  double multiplierLower = (1 + 0.05 * headNum) * (1 + 0.01 * totalXunlian) * (1 + motivationFactor * (1 + 0.01 * totalGanjing)) * totalYouqingMultiplier;
  double multiplierUpper = (1 + 0.05 * headNum) * (1 + 0.01 * totalXunlianUpper) * (1 + motivationFactor * (1 + 0.01 * totalGanjingUpper)) * totalYouqingMultiplier * totalYouqingUpperMultiplier;
  //trainValueCardMultiplier[t] = cardMultiplier;

  //���Կ�ʼ����
  double trainValueTotalTmp[6];
  for (int i = 0; i < 6; i++)
  {
    double bvl = basicValue[i];
    double umaBonus = i < 5 ? 1 + 0.01 * fiveStatusBonus[i] : 1;
    trainValueLower[tra][i] = bvl * multiplierLower * umaBonus;
    trainValueTotalTmp[i] = bvl * multiplierUpper * umaBonus;
  }


  //�ϲ�=����-�²�

  for (int i = 0; i < 6; i++)
  {
    int lower = trainValueLower[tra][i];
    if (lower > 100) lower = 100;
    trainValueLower[tra][i] = lower;

    int total = int(trainValueTotalTmp[i]);
    int upper = total - lower;
    if (upper > 100)upper = 100;
    if (upper < 0)upper = 0;
    if (i < 5)
    {
      lower = calculateRealStatusGain(fiveStatus[i], lower);//consider the integer over 1200
      upper = calculateRealStatusGain(fiveStatus[i] + lower, upper);
    }
    total = upper + lower;
    trainValue[tra][i] = total;
  }


}

void Game::addScenarioBuffBonus(int idx)
{
  int id = lg_buffs[idx].buffId;
  if (id < 0)return;
  if (id == 0 * 19 + 0 || id == 1 * 19 + 0 || id == 2 * 19 + 0)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.deyilv += 30;
  }
  else if (id == 0 * 19 + 1 || id == 1 * 19 + 1 || id == 2 * 19 + 1)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.hintProb += 80;
  }
  else if (id == 0 * 19 + 2 || id == 1 * 19 + 2 || id == 2 * 19 + 2)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.jibanAdd1 += 2;
  }
  else if (id == 0 * 19 + 4 || id == 1 * 19 + 4 || id == 2 * 19 + 4)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.deyilv += 60;
  }
  else if (id == 0 * 19 + 6 || id == 1 * 19 + 6 || id == 2 * 19 + 6)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 5;
  }
  else if (id == 0 * 19 + 3)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.ganjing += 15;
  }
  else if (id == 0 * 19 + 5)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.ganjing += 30;
  }
  else if (id == 0 * 19 + 7)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.ganjing += 15;
    lg_bonus.ganjingPerHead += 8;
  }
  else if (id == 0 * 19 + 8)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
      lg_bonus.ganjing += 50;
  }
  else if (id == 0 * 19 + 9)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
      lg_bonus.hintProb += 200;
  }
  else if (id == 0 * 19 + 10)
  {
    lg_buffs[idx].isActive = false; //�غϺ󴥷�
  }
  else if (id == 0 * 19 + 11)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.youqing += 60;
  }
  else if (id == 0 * 19 + 12)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
      lg_bonus.alwaysHint;

  }
  else if (id == 0 * 19 + 13)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.xunlian += 60;
      lg_bonus.moreHint += 1;
    }
  }
  else if (id == 0 * 19 + 14)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.youqing += 15;
      if (lg_blue_active)
        lg_bonus.youqing += 20;
    }
  }
  else if (id == 0 * 19 + 15)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.ganjing += 200;
      lg_bonus.vitalReduce += 100;
    }
  }
  else if (id == 0 * 19 + 16)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.ganjing += 120;
    }
  }
  else if (id == 0 * 19 + 17)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.deyilv += 100;
    }
  }
  else if (id == 0 * 19 + 18)
  {
    lg_buffs[idx].isActive = motivation >= 5;
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.youqing += 25;
    }
  }
  else if (id == 1 * 19 + 3)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 3;
  }
  else if (id == 1 * 19 + 5)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 3;
    lg_bonus.ganjing += 15;
  }
  else if (id == 1 * 19 + 7)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 2;
    lg_bonus.xunlianPerHead += 2;
  }
  else if (id == 1 * 19 + 8)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.xunlian += 7;
  }
  else if (id == 1 * 19 + 9)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.moreHint += 1;
  }
  else if (id == 1 * 19 + 10)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.vitalReduce += 15;
  }
  else if (id == 1 * 19 + 11)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.youqing += 22;
  }
  else if (id == 1 * 19 + 12)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.xunlian += 25;
  }
  else if (id == 1 * 19 + 13)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 20;
  }
  else if (id == 1 * 19 + 14)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 15;
    if (lg_mainColor == L_green)
    {
      throw "��ɫδʵ��";
    }
  }
  else if (id == 1 * 19 + 15)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.extraHead += 3;
  }
  else if (id == 1 * 19 + 16)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.ganjing += 120;
  }
  else if (id == 1 * 19 + 17)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.hintProb += 250;
  }
  else if (id == 1 * 19 + 18)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.youqing += 25;
  }

  else if (id == 2 * 19 + 3)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.youqing += 5;
  }
  else if (id == 2 * 19 + 5)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.youqing += 3;
    lg_bonus.ganjing += 15;
  }
  else if (id == 2 * 19 + 7)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.youqing += 3;
    lg_bonus.youqingPerShiningHead += 3;
  }
  else if (id == 2 * 19 + 8)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.extraHead += 1;
  }
  else if (id == 2 * 19 + 9)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.hintLv += 1;
  }
  else if (id == 2 * 19 + 10)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.youqing += 10;
      lg_bonus.jibanAdd2 += 3;
    }
  }
  else if (id == 2 * 19 + 11)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.xunlian += 15;
      lg_bonus.extraHead += 3;
    }
  }
  else if (id == 2 * 19 + 12)
  {
    if (lg_buffs[idx].isActive)
      lg_bonus.extraHead += 1;
  }
  else if (id == 2 * 19 + 13)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.youqing += 22;
  }
  else if (id == 2 * 19 + 14)
  {
    lg_buffs[idx].isActive = true;
    lg_bonus.xunlian += 7;
    lg_bonus.xunlianPerHead += 7;
  }
  else if (id == 2 * 19 + 15)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.disappearRateReduce += 25;
    }
  }
  else if (id == 2 * 19 + 16)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.ganjing += 150;
    }
  }
  else if (id == 2 * 19 + 17)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.deyilv += 100;
    }
  }
  else if (id == 2 * 19 + 18)
  {
    if (lg_buffs[idx].isActive)
    {
      lg_bonus.xunlian += 20;
      lg_bonus.youqing += 20;
    }
  }
  else
  {
    throw "δ֪�ĵ�";
  }

}

void Game::updateScenarioBuffCondition(int idx)
{
  int id = lg_buffs[idx].buffId;
  if (id < 0)return;

  //�ɾ����õ��ȼ�ʱ�Ե������������ﴦ��

  //��Ϣ��
  if (
    id == 0 * 19 + 11 ||
    id == 0 * 19 + 13 ||
    id == 0 * 19 + 15||
    id == 2 * 19 + 11)
  {
    if(lg_buffCondition.isRest)
      lg_buffs[idx].isActive = true;
    if (lg_buffCondition.isTraining)
      lg_buffs[idx].isActive = false;
    //ע��������ı�״̬

    if (lg_buffCondition.isRest && id == 0 * 19 + 11)
      addMotivation(1);
  }
  //ѵ���ɹ��󣬲�������CD��
  else if (
    id == 1 * 19 + 8 ||
    id == 1 * 19 + 10 ||
    id == 1 * 19 + 12 ||
    id == 1 * 19 + 14 ||
    id == 1 * 19 + 16 ||
    id == 1 * 19 + 17 ||
    id == 1 * 19 + 18 
    )
  {
    if (lg_buffCondition.isTraining)
      lg_buffs[idx].isActive = false;
    if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed)
      lg_buffs[idx].isActive = true;
    //ע�������Ϣ������ı�״̬
  }
  //����ѵ���ɹ��󣬲�������CD��
  else if (
    id == 1 * 19 + 9 ||
    id == 2 * 19 + 16 ||
    id == 2 * 19 + 17 
    )
  {
    if (lg_buffCondition.isTraining)
      lg_buffs[idx].isActive = false;
    if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed && lg_buffCondition.isYouqing)
      lg_buffs[idx].isActive = true;
    //ע�������Ϣ������ı�״̬
  }
  //3~5ͷѵ���󣬲�������CD��
  else if (
    id == 2 * 19 + 9 ||
    id == 2 * 19 + 10 ||
    id == 2 * 19 + 12 ||
    id == 2 * 19 + 15 ||
    id == 2 * 19 + 18 
    )
  {
    int headRequire = (id == 2 * 19 + 18) ? 5 : 3;
    if (lg_buffCondition.isTraining)
      lg_buffs[idx].isActive = false;
    if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed && lg_buffCondition.trainingHead >= headRequire)
      lg_buffs[idx].isActive = true;
    //ע�������Ϣ������ı�״̬
  }
  //��CD�Ķ���������
  else if (id == 0 * 19 + 10)
  {
    if (lg_buffs[idx].coolTime > 0)
    {
      lg_buffs[idx].coolTime -= 1;
    }
    else
    {
      if (lg_buffs[idx].isActive)
      {
        if (lg_buffCondition.isTraining)
        {
          lg_buffs[idx].isActive = false;
          lg_buffs[idx].coolTime = 3;
        }
      }
      else
      {
        if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed && lg_buffCondition.isYouqing)
        {
          lg_buffs[idx].isActive = true;
          addMotivation(1);
        }
      }
    }
  }
  else if (id == 1 * 19 + 15)
  {
    if (lg_buffs[idx].coolTime > 0)
    {
      lg_buffs[idx].coolTime -= 1;
    }
    else
    {
      if (lg_buffs[idx].isActive)
      {
        if (lg_buffCondition.isTraining)
        {
          lg_buffs[idx].isActive = false;
          lg_buffs[idx].coolTime = 2;
        }
      }
      else
      {
        if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed)
          lg_buffs[idx].isActive = true;
      }
    }
  }
  else if (id == 2 * 19 + 8)
  {
    if (lg_buffs[idx].coolTime > 0)
    {
      lg_buffs[idx].coolTime -= 1;
    }
    else
    {
      if (lg_buffs[idx].isActive)
      {
        if (lg_buffCondition.isTraining)
        {
          lg_buffs[idx].isActive = false;
          lg_buffs[idx].coolTime = 2;
        }
      }
      else
      {
        if (lg_buffCondition.isTraining && lg_buffCondition.trainingSucceed && lg_buffCondition.isYouqing)
          lg_buffs[idx].isActive = true;
      }
    }
  }
  else if (id >= 0 && id < 57)
  {
    //����Ҫ����
  }
  else 
  {
    throw "δ֪�ĵ�";
  }
  
}

void Game::addLgGauge(int16_t color, int num)
{
  lg_gauge[color] += num;
  if (lg_gauge[color] > 8)lg_gauge[color] = 8;
}

void Game::setMainColorTurn36(std::mt19937_64& rand)
{
  int count[3] = { 0, 0, 0 };
  for (int i = 0; i < 6; ++i) {
    int color = lg_buffs[i].buffId / 19;
    ++count[color];
  }

  int max_count = *std::max_element(std::begin(count), std::end(count));
  std::vector<int> candidates;
  for (int c = 0; c < 3; ++c) {
    if (count[c] == max_count) {
      candidates.push_back(c);
    }
  }

  if (candidates.size() == 1) {
    lg_mainColor = candidates[0];
  }
  else {
    std::uniform_int_distribution<int> dist(0, candidates.size() - 1);
    int idx = dist(rand);
    lg_mainColor = candidates[idx];
  }

  if (gameSettings.color_priority != -1 && gameSettings.color_priority != lg_mainColor)
  {
    lg_mainColor = gameSettings.color_priority;
    skillScore -= 3000;//ûѡ����������ɫ����װ��3000�֣��Ұ���Ԥ������ɫ��������
  }
  if (lg_mainColor == L_red)
  {
    for (int i = 0; i < 6; i++)
    {
      if (persons[i].personType == PersonType_card)
      {
        lg_red_friendsLv[i] = 1;
        lg_red_friendsGauge[i] = 20;
      }
    }
    for (int i = PS_npc0; i <= PS_npc4; i++)
    {
      lg_red_friendsLv[i] = 1;
      lg_red_friendsGauge[i] = 20;
    }
  }
  else if (lg_mainColor == L_blue)
  {
    throw "todo";
  }
  else if (lg_mainColor == L_green)
  {
    throw "todo";
  }

}


void Game::randomPickBuff(std::mt19937_64& rand)
{
  if (stage != ST_pickBuff)
    throw "��ǰstage������randomPickBuff"; 
  stage = ST_chooseBuff;
  lg_pickedBuffsNum = 0;
  for (int i = 0; i < 9; i++)
    lg_pickedBuffs[i] = -1;
  int maxStar = turn <= 12 ? 1 : turn <= 24 ? 2 : 3;
  for (int color = 0; color < 3; color++)
  {
    int toPickNum = lg_gauge[color] == 8 ? 3 : lg_gauge[color] >= 4 ? 2 : lg_gauge[color] >= 2 ? 1 : 0;
    int toPickMaxStarNum = toPickNum == 3 ? (friend_type == 1 ? 2 : 1) : 0;//��8����2������ǵģ������ſ���1����
    int toPickRandomStarNum = toPickNum - toPickMaxStarNum;
    while (toPickMaxStarNum > 0)
    {
      int p = pickSingleBuff(rand, color, maxStar);
      if (p >= 0)
      {
        lg_pickedBuffs[lg_pickedBuffsNum] = p;
        lg_pickedBuffsNum += 1;
        toPickMaxStarNum -= 1;
      }
    }
    while (toPickRandomStarNum > 0)
    {
      int star = 1;
      if (maxStar == 2)
        star = rand() % 2 + 1;//50%һ�ǣ�50%����
      else if (maxStar == 3)
      {
        int s = rand() % 10;
        star = s < 3 ? 1 : s < 8 ? 2 : 3;//30%һ�ǣ�50%���ǣ�20%����
      }
      int p = pickSingleBuff(rand, color, star);
      if (p >= 0)
      {
        lg_pickedBuffs[lg_pickedBuffsNum] = p;
        lg_pickedBuffsNum += 1;
        toPickRandomStarNum -= 1;
      }
    }
  }
}

int Game::pickSingleBuff(std::mt19937_64& rand, int16_t color, int16_t star)
{
  int num = star == 1 ? 4 : star == 2 ? 6 : 9;
  int idx = star == 1 ? 0 : star == 2 ? 4 : 10;
  idx += color * 19;

  int maxTry = 20;
  while (maxTry > 0)
  {
    maxTry--;
    int t = idx + rand() % num;
    if (lg_haveBuff[t])continue;
    bool alreadyPicked = false;
    for (int i = 0; i < lg_pickedBuffsNum; i++)
    {
      if (lg_pickedBuffs[i] == t) {
        alreadyPicked = true;
        break;
      }
    }
    if (alreadyPicked)continue;
    return t;
  }
  return -1;//������ǳ����ˣ�С��������������Ӱ�첻�󣬻����³�ȡ
}

void Game::chooseBuff(int16_t idx)
{
  if (stage != ST_chooseBuff)
    throw "����ѡbuff�Ľ׶�";
  if (turn % 6 != 5 || turn > 65)
    throw "����ѡ�ĵõĻغ�";
  if (turn != 65)
  {
    if (idx < 0 || idx >= lg_pickedBuffsNum)
      throw "���Ϸ�ѡ��";
    int loc = turn / 6;
    lg_haveBuff[lg_pickedBuffs[idx]] = true;
    lg_buffs[loc].buffId = lg_pickedBuffs[idx];
    lg_buffs[loc].isActive = false;
    lg_buffs[loc].coolTime = 0;
  }
  else if (idx != 0) //����65�غϣ���11���ĵã���idx=0����ѡ��idx=10*(λ��+1)+�ڼ���ѡ��
  {
    int loc = (idx / 10) - 1;//���ڵڼ���λ�á���66���غ���Ҫ�滻һ��buff
    idx = idx % 10;
    if (loc < 0 || loc >= 10)
      throw "��66���غ���Ҫ�滻һ��buff��10*(λ��+1)+�ڼ���ѡ��";
    if (idx < 0 || idx >= lg_pickedBuffsNum)
      throw "���Ϸ�ѡ��";
    lg_haveBuff[lg_buffs[loc].buffId] = false;
    lg_haveBuff[lg_pickedBuffs[idx]] = true;
    lg_buffs[loc].buffId = lg_pickedBuffs[idx];
    lg_buffs[loc].isActive = false;
    lg_buffs[loc].coolTime = 0;
  }
  lg_pickedBuffsNum = 0;
  for (int i = 0; i < 9; i++)
    lg_pickedBuffs[i] = -1;
  for (int i = 0; i < 3; i++)
    lg_gauge[i] = 0;
  stage = ST_event;
}

void Game::checkEvent(std::mt19937_64& rand)
{
  assert(stage == ST_event);
  checkFixedEvents(rand);
  checkRandomEvents(rand);


  //�غ���+1
  turn++;
  stage = ST_distribute;
  
  if (turn >= TOTAL_TURN)
  {
    printEvents("���ɽ���!");
    printEvents("��ĵ÷��ǣ�" + to_string(finalScore()));
  }
  else {
    isRacing = isRacingTurn[turn];
    calculateScenarioBonus();
  }
  return;

}
void Game::checkFixedEvents(std::mt19937_64& rand)
{
  //������̶ֹ��¼�
  if (isRefreshMind)
  {
    addVital(5);
    if (rand() % 4 == 0) //����ÿ�غ���25%����buff��ʧ
      isRefreshMind = false;
  }
  if (isRacing)//���ı���
  {
    if (turn < 72)
    {
      runRace(3, 45);
      addJiBan(PS_noncardYayoi, 4, 1);
    }
    else if (turn == 73)//ura1
    {
      throw "this scenario does not have URA";
      runRace(10, 40);
    }
    else if (turn == 75)//ura1
    {
      throw "this scenario does not have URA";
      runRace(10, 60);
    }
    else if (turn == 77)//ura3
    {
      throw "this scenario does not have URA";
      runRace(10, 80);
    }

  }

  if (turn == 11)//������
  {
    assert(isRacing);
  }
  else if (turn == 23)//��һ�����
  {
    runRace(30, 125);
    skillScore += 170;//���м��ܵȼ�+1
    addVital(25);
    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 4);

    handleFriendFixedEvent();
    printEvents("��һ�����");
  }
  else if (turn == 29)//�ڶ���̳�
  {
    jicheng(rand);
    printEvents("�ڶ���̳�");
  }
  else if (turn == 35)
  {

    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 4);
    setMainColorTurn36(rand);
    printEvents("�ڶ�����޿�ʼ");
  }
  else if (turn == 42)
  {
    addAllStatus(5);
    skillPt += 50;
  }
  else if (turn == 47)//�ڶ������
  {
    runRace(45, 210);
    skillScore += 170;//���м��ܵȼ�+1
    addVital(35);
    for (int i = 0; i < 5; i++)
      addTrainingLevelCount(i, 4);
    printEvents("�ڶ������");
  }
  else if (turn == 48)//�齱
  {
  //  int rd = rand() % 100;
  //  if (rd < 16)//��Ȫ��һ�Ƚ�
  //  {
  //    addVital(30);
  //    addAllStatus(10);
  //    addMotivation(2);

  //    printEvents("�齱�����������Ȫ/һ�Ƚ�");
  //  }
  //  else if (rd < 16 + 27)//���Ƚ�
  //  {
  //    addVital(20);
  //    addAllStatus(5);
  //    addMotivation(1);
  //    printEvents("�齱��������˶��Ƚ�");
  //  }
  //  else if (rd < 16 + 27 + 46)//���Ƚ�
  //  {
  //    addVital(20);
  //    printEvents("�齱������������Ƚ�");
  //  }
  //  else//��ֽ
  //  {
  //    addMotivation(-1);
  //    printEvents("�齱��������˲�ֽ");
  //  }
  }
  else if (turn == 49)
  {
  }
  else if (turn == 53)//������̳�
  {
    jicheng(rand);
    printEvents("������̳�");

    //if (getYayoiJiBan() >= 60)
    //{
    //  skillScore += 170;//���м��ܵȼ�+1
    //  addMotivation(1);
    //}
    //else
    //{
    //  addVital(-5);
    //  skillPt += 25;
    //}
  }
  else if (turn == 59)
  {
    //addAllStatus(10);
    //skillPt += 100;
  
    printEvents("��������޿�ʼ");
  }
  else if (turn == 70)
  {
  }
  else if (turn == TOTAL_TURN - 1)//��Ϸ����
  {
    //�޴��棬���
    addAllStatus(25);
    skillPt += 125;


    runRace(55, 300);
    skillScore += 170;//���м��ܵȼ�+1

    //����
    if (friendship_noncard_reporter >= 80)
    {
      addAllStatus(5);
      skillPt += 20;
    }
    else if (friendship_noncard_reporter >= 60)
    {
      addAllStatus(3);
      skillPt += 10;
    }
    else if (friendship_noncard_reporter >= 40)
    {
      skillPt += 10;
    }
    else
    {
      skillPt += 5;
    }


    bool allWin = true;
    if (allWin)
    {
      addAllStatus(45);
      skillPt += 245;
    }
    else
    {
      addAllStatus(25);
      //there should be something, but not important
    }

    //���˿��¼�
    handleFriendFixedEvent();

    //addAllStatus(5);
    //skillPt += 20;

    printEvents("��������Ϸ����");
  }
}

void Game::checkRandomEvents(std::mt19937_64& rand)
{
  if (turn >= 72)
    return;//ura�ڼ䲻�ᷢ����������¼�

  //���˻᲻���������
  if (friend_type != 0)
  {
    Person& p = persons[friend_personId];
    assert(p.personType == PersonType_scenarioCard);
    if (friend_stage==FriendStage_beforeUnlockOutgoing)
    {
      double unlockOutgoingProb = p.friendship >= 60 ?
        GameConstants::FriendUnlockOutgoingProbEveryTurnHighFriendship :
        GameConstants::FriendUnlockOutgoingProbEveryTurnLowFriendship;
      //unlockOutgoingProb = 1.0;
      if (randBool(rand, unlockOutgoingProb))//����
      {
        handleFriendUnlock(rand);
      }
    }

    if (friend_qingre)//����״̬�������
    {
      if (friend_qingreTurn > 9)friend_qingreTurn = 9;
      double stopProb = GameConstants::FriendQingreStopProb[friend_qingreTurn];
      if (randBool(rand, stopProb))
      {
        friend_qingre = false;
        friend_qingreTurn = 0;
        printEvents("�ſ����Ƚ���");
      }
      else
        friend_qingreTurn += 1;
    }

  }

  //ģ���������¼�

  //֧Ԯ�������¼��������һ������5�
  if (randBool(rand, GameConstants::EventProb))
  {
    int card = rand() % 6;
    addJiBan(card, 5, 1);
    //addAllStatus(4);
    addStatus(rand() % 5, gameSettings.eventStrength);
    skillPt += gameSettings.eventStrength;
    printEvents("ģ��֧Ԯ������¼���" + persons[card].cardParam.cardName + " ���+5��pt���������+" + to_string(gameSettings.eventStrength));

    //֧Ԯ��һ����ǰ�����¼�������
    if (randBool(rand, 0.6 * pow((1.0 - turn * 1.0 / TOTAL_TURN),2)))
    {
      addMotivation(1);
      printEvents("ģ��֧Ԯ������¼�������+1");
    }
    if (randBool(rand, 0.5))
    {
      addVital(10);
      printEvents("ģ��֧Ԯ������¼�������+10");
    }
    else if (randBool(rand, 0.003))
    {
      addVital(-10);
      printEvents("ģ��֧Ԯ������¼�������-10");
    }
    if (randBool(rand, 0.003))
    {
      isPositiveThinking = true;
      printEvents("ģ��֧Ԯ������¼�����á�����˼����");
    }
  }

  //ģ����������¼�
  if (randBool(rand, 0.25))
  {
    addAllStatus(3);
    skillPt += 15;
    printEvents("ģ����������¼���ȫ����+3");
  }

  //������
  if (randBool(rand, 0.10))
  {
    addVital(5);
    printEvents("ģ������¼�������+5");
  }

  //������
  if (randBool(rand, 0.05))
  {
    addVital(10);
    printEvents("ģ������¼�������+10");
  }

  //��30�������Է��¼���
  if (randBool(rand, 0.02))
  {
    addVital(30);
    printEvents("ģ������¼�������+30");
  }

  //������
  if (randBool(rand, 0.02))
  {
    addMotivation(1);
    printEvents("ģ������¼�������+1");
  }

  //������
  if (turn >= 12 && randBool(rand, 0.05))
  {
    addMotivation(-1);
    printEvents("ģ������¼���\033[0m\033[33m����-1\033[0m\033[32m");
  }

}
std::vector<Action> Game::getAllLegalActions() const
{
  std::vector<Action> allActions;

  if (isEnd()) return allActions;

  if (stage == ST_distribute)
  {
    allActions.push_back(Action(ST_distribute)); 
  }
  else if (stage == ST_train)
  {
    if (isRacing)
    {
      allActions.push_back(Action(ST_train, T_race));
      return allActions;
    }
    for (int i = 0; i < 5; i++)
      allActions.push_back(Action(ST_train, i));
    if(!isXiahesu())
      allActions.push_back(Action(ST_train, T_rest));
    allActions.push_back(Action(ST_train, T_outgoing));
    if (isRaceAvailable())
      allActions.push_back(Action(ST_train, T_race));
  }
  else if (stage == ST_decideEvent)
  {
    if (decidingEvent == DecidingEvent_outing)
    {
      if(!(friend_stage == FriendStage_afterUnlockOutgoing && !friend_outgoingUsed[4]))
        throw "�޷����˳���";
      allActions.push_back(Action(ST_decideEvent, 0));//��ͨ����
      if (friend_outgoingUsed[0] && friend_outgoingUsed[1] && friend_outgoingUsed[2])
      {
        if (friend_outgoingUsed[3])
        {
          allActions.push_back(Action(ST_decideEvent, 7));//��5��
        }
        else
        {
          allActions.push_back(Action(ST_decideEvent, 4));//��4��
          allActions.push_back(Action(ST_decideEvent, 5));//��4��
          allActions.push_back(Action(ST_decideEvent, 6));//��4��
        }
      }
      else
      {
        if (!friend_outgoingUsed[0])
          allActions.push_back(Action(ST_decideEvent, 1));//��1��
        if (!friend_outgoingUsed[1])
          allActions.push_back(Action(ST_decideEvent, 2));//��2��
        if (!friend_outgoingUsed[2])
          allActions.push_back(Action(ST_decideEvent, 3));//��3��
      }

    }
    else if (decidingEvent == DecidingEvent_three)
    {
      allActions.push_back(Action(ST_decideEvent, 0));
      allActions.push_back(Action(ST_decideEvent, 1));
      allActions.push_back(Action(ST_decideEvent, 2));
    }
    else throw "δ֪decidingEvent";

  }
  else if (stage == ST_pickBuff)
  {
    allActions.push_back(Action(ST_pickBuff));

  }
  else if (stage == ST_chooseBuff)
  {
    if (lg_pickedBuffsNum <= 0)
      throw "û�г鵽buff��";
    if (turn == 65)
    {
      allActions.push_back(Action(ST_chooseBuff, 0));
      for (int place = 0; place < 10; place++)
        for (int i = 0; i < lg_pickedBuffsNum; i++)
          allActions.push_back(Action(ST_chooseBuff, 10 * (place + 1) + i));

    }
    else
    {
      for (int i = 0; i < lg_pickedBuffsNum; i++)
        allActions.push_back(Action(ST_chooseBuff, i));
    }
  }
  else if (stage == ST_event)
  {
    allActions.push_back(Action(ST_event));
  }
  else
    throw "δ֪stage";
  return allActions;
}
void Game::applyAction(std::mt19937_64& rand, Action action)
{
  if (isEnd()) return;
  if (action.stage == ST_action_randomize)
  {
    undoRandomize();
    return;
  }
  if (stage != action.stage)
    throw "Game::applyAction��stage��ƥ��";

  if (stage == ST_distribute)
  {
    randomizeTurn(rand);
  }
  else if (stage == ST_train)
  {
    bool suc = applyTraining(rand, action.idx);
    assert(suc && "Game::applyActionѡ���˲��Ϸ���ѵ��");

  }
  else if (stage == ST_decideEvent)
  {
    decideEvent(rand, action.idx);

  }
  else if (stage == ST_pickBuff)
  {
    randomPickBuff(rand);

  }
  else if (stage == ST_chooseBuff)
  {
    chooseBuff(action.idx);

  }
  else if (stage == ST_event)
  {
    checkEvent(rand);
  }
  else
    throw "δ֪stage";
}

void Game::continueUntilNextDecision(std::mt19937_64& rand)
{
  auto allActions = getAllLegalActions();
  if (allActions.size() == 0)
  {
    assert(isEnd());
    return;
  }
  else if (allActions.size() == 1)
  {
    applyAction(rand, allActions[0]);
    continueUntilNextDecision(rand);
  }
  else return;
}

void Game::applyActionUntilNextDecision(std::mt19937_64& rand, Action action)
{
  applyAction(rand, action);
  continueUntilNextDecision(rand);
}

bool Game::isCardShining(int personIdx, int trainIdx) const
{
  const Person& p = persons[personIdx];
  if (personIdx >= 0 && personIdx < 6)
  {
    if (p.personType == PersonType_card)
    {
      return p.friendship >= 80 && trainIdx == p.cardParam.cardType;
    }
    else if (p.personType == PersonType_scenarioCard)
    {
      return friend_qingre;
    }
    else if (p.personType == PersonType_groupCard)
    {
      throw "other friends or group cards are not supported";
    }
    return false;
  }
  else if (personIdx >= PS_npc0 && personIdx <= PS_npc4)//���npc
  {
    int tra = personIdx - PS_npc0;
    assert(lg_mainColor == L_red);
    int gauge = lg_red_friendsGauge[personIdx];
    return tra == trainIdx && gauge == 20;
  }
  return false;
}

ScenarioBonus::ScenarioBonus()
{
  clear();
}

void ScenarioBonus::clear()
{
  hintProb = 0.0f;
  hintLv = 0;
  moreHint = 0;
  alwaysHint = false;

  vitalReduce = 0.0f;
  jibanAdd1 = 0.0f;
  jibanAdd2 = 0.0f;
  deyilv = 0.0f;
  disappearRateReduce = 0.0f;

  xunlian = 0.0f;
  ganjing = 0.0f;
  youqing = 0.0f;

  extraHead = 0;

  xunlianPerHead = 0.0f;
  ganjingPerHead = 0.0f;
  youqingPerShiningHead = 0.0f;
}

GameSettings::GameSettings()
{
  playerPrint = false;
  ptScoreRate = GameConstants::ScorePtRateDefault;
  hintPtRate = GameConstants::HintLevelPtRateDefault;
  hintProbTimeConstant= GameConstants::HintProbTimeConstantDefault;
  eventStrength = GameConstants::EventStrengthDefault;
  scoringMode = SM_normal;
  color_priority = L_red;
}

ScenarioBuffInfo::ScenarioBuffInfo()
{
  buffId = -1;
  isActive = false;
  coolTime = 0;
}

int16_t ScenarioBuffInfo::getBuffColor() const
{
  if (buffId < 0)return -1;
  return buffId / 19;
}

int16_t ScenarioBuffInfo::getBuffStar() const
{
  return getBuffStarStatic(buffId);
}


Action::Action():stage(ST_none),idx(0)
{
}

Action::Action(int st) :stage(st), idx(0)
{
}

Action::Action(int st, int idx):stage(st), idx(idx)
{
}

std::string Action::toString() const
{
  return "Stage"+to_string(stage) + "_Idx" + to_string(idx);
}

static string getColoredColorName(int color)
{
  if (color == L_red)
    return "\033[1;31m��\033[0m";
  else if (color == L_green)
    return "\033[1;32m��\033[0m";
  else if (color == L_blue)
    return "\033[1;34m��\033[0m";
  else
    return "???";
}

std::string Action::toString(const Game& game) const
{
  if (idx < 0)
    return toString();
  if (stage == ST_train)
  {
    return trainingName[idx];
  }
  else if (stage == ST_decideEvent)
  {
    if (game.decidingEvent == DecidingEvent_outing)
    {
      if (idx == 0)
        return "��ͨ���";
      else if (idx == 1)
        return "�Ŷ����1";
      else if (idx == 2)
        return "�Ŷ����2";
      else if (idx == 3)
        return "�Ŷ����3";
      else if (idx == 4)
        return "�Ŷ����4ѡ��";
      else if (idx == 5)
        return "�Ŷ����4ѡ��";
      else if (idx == 6)
        return "�Ŷ����4ѡ��";
      else if (idx == 7)
        return "�Ŷ����5";
      else
        assert(false);
    }
    else if (game.decidingEvent == DecidingEvent_three)
    {
      if (idx < 3)
        return "ѡ" + getColoredColorName(idx);
      else
        assert(false);
    }
    else
      assert(false);
  }
  else if (stage == ST_chooseBuff)
  {
    if (game.turn == 65)
    {
      if (idx == 0)
        return "��ѡ";
    }

    int t = game.turn == 65 ? idx % 10 : idx;
    int c = game.lg_pickedBuffs[t] / 19;//��ɫ
    int ord = 0;
    for (int i = 0; i < t; i++)
    {
      if (game.lg_pickedBuffs[i] / 19 == c)
        ord += 1;
    }
    if (game.turn == 65)
    {
      return "ѡ" + getColoredColorName(c) + "ɫ�� \033[33m" + to_string(ord+1) + "\033[0m ���滻�� " + ScenarioBuffInfo::getScenarioBuffName(game.lg_buffs[idx / 10 - 1].buffId);
    }
    else
    {
      return "ѡ" + getColoredColorName(c) + "ɫ�� \033[33m" + to_string(ord + 1) + "\033[0m ��";// +ScenarioBuffInfo::getScenarioBuffName(game.lg_pickedBuffs[idx]);
      //return "ѡ" + ScenarioBuffInfo::getScenarioBuffName(game.lg_pickedBuffs[idx]);
    }
  }
  else
    return toString();
}

ScenarioBuffCondition::ScenarioBuffCondition()
{
  clear();
}

void ScenarioBuffCondition::clear()
{
  isRest = false;
  isTraining = false;
  isYouqing = false;
  trainingSucceed = false;
  trainingHead = 0;
}
