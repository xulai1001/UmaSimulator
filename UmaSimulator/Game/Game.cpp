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
  isRacingTurn[TOTAL_TURN - 5] = true;//ura1
  isRacingTurn[TOTAL_TURN - 3] = true;//ura2
  isRacingTurn[TOTAL_TURN - 1] = true;//ura3

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
  if (stage != ST_distribute && stage != ST_train)
    throw "������俨��Ӧ����ST_distribute��ST_trainʱ����";
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
      throw "inviteOneHead����1000��ʧ��";
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
void Game::addJiBan(int idx, int value, int type)
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
      if (isAiJiao)
        gain += 2;
    }
    else if(type == 1)
    {
      gain += lg_bonus.jibanAdd2;
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
  if (isXiahesu())
  {
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

void Game::runFriendOutgoing(std::mt19937_64& rand, int idx)
{
  assert(friend_type!=0 && friend_stage >= FriendStage_afterUnlockOutgoing && !friend_outgoingUsed[idx]);
  int pid = friend_personId;
  if (idx == 0)
  {
    addVitalFriend(30);
    addMotivation(1);
    addStatusFriend(3, 20);
    addJiBan(pid, 5, false);
  }
  else if (idx == 1)
  {
    addVitalFriend(30);
    addMotivation(1);
    addStatusFriend(0, 10);
    addStatusFriend(3, 10);
    isRefreshMind = true;
    addJiBan(pid, 5, false);
  }
  else if (idx == 2)
  {
    int remainVital = maxVital - vital;
    if (remainVital >= 20)//ѡ��
      addVitalFriend(43);
    else//ѡ��
      addStatusFriend(3, 29);
    addMotivation(1);
    addJiBan(pid, 5, false);
  }
  else if (idx == 3)
  {
    addVitalFriend(30);
    addMotivation(1);
    addStatusFriend(3, 25);
    addJiBan(pid, 5, false);
  }
  else if (idx == 4)
  {
    //�д�ɹ��ͳɹ�
    if (rand() % 4 != 0)//���Թ��ƣ�75%��ɹ�
    {
      addVitalFriend(30);
      addStatusFriend(3, 36);
      skillPt += 72;//���ܵȼ�
    }
    else
    {
      addVitalFriend(26);
      addStatusFriend(3, 24);
      skillPt += 40;//���ܵȼ�
    }
    addMotivation(1);
    addJiBan(pid, 5, false);
    isRefreshMind = true;
  }
  else assert(false && "δ֪�ĳ���");

}
void Game::handleFriendUnlock(std::mt19937_64& rand)
{
  assert(friend_stage == FriendStage_beforeUnlockOutgoing);
  if (maxVital - vital >= 40)
  {
    addVitalFriend(25);
    printEvents("�������������ѡ��");
  }
  else
  {
    addStatusFriend(0, 8);
    addStatusFriend(3, 8);
    skillPt += 10;//ֱ������+5
    printEvents("�������������ѡ��");
  }
  addMotivation(1);
  isRefreshMind = true;
  addJiBan(friend_personId, 5, false);
  friend_stage = FriendStage_afterUnlockOutgoing;
}
void Game::handleFriendClickEvent(std::mt19937_64& rand, int atTrain)
{
  assert(friend_type!=0 && (friend_personId<6&& friend_personId>=0) && persons[friend_personId].personType==PersonType_scenarioCard);
  if (friend_stage == FriendStage_notClicked)
  {
    printEvents("��һ�ε�����");
    friend_stage = FriendStage_beforeUnlockOutgoing;
    
    addStatusFriend(0, 14);
    addJiBan(friend_personId, 10, false);
    addMotivation(1);
  }
  else
  {
    if (rand() % 5 < 3)return;//40%���ʳ��¼���60%���ʲ���

    if (rand() % 10 == 0)
    {
      if (motivation != 5)
        printEvents("���˵���¼�:����+1");
      addMotivation(1);//10%���ʼ�����
    }

    if (turn < 24)
    {
      //�����͵��˼�3�
      int minJiBan = 10000;
      int minJiBanId = -1;
      for (int i = 0; i < 6; i++)
      {
        if (persons[i].personType == PersonType_card)
        {
          if (persons[i].friendship < minJiBan)
          {
            minJiBan = persons[i].friendship;
            minJiBanId = i;
          }
        }
      }
      if (minJiBanId != -1)
      {
        addJiBan(minJiBanId, 3, false);
      }
      addJiBan(friend_personId, 5, false);
      printEvents("���˵���¼�:" + persons[minJiBanId].getPersonName() + " �+3, ���³��+5");

     
    }
    else if (turn < 48)
    {
      addStatusFriend(0, 12);
      addJiBan(friend_personId, 5, false);
    }
    else
    {
      addStatusFriend(3, 12);
      addJiBan(friend_personId, 5, false);
    }
  }

}
void Game::handleFriendFixedEvent()
{
  if (friend_type == 0)return;//û���˿�
  if (friend_stage < FriendStage_beforeUnlockOutgoing)return;//����û������û�¼�
  if (turn == 23)
  {
    addMotivation(1);
    addStatusFriend(0, 24);
    addJiBan(friend_personId, 5, false);
    skillPt += 40;//�����������ߣ������н�����������hint����Ч��
  }
  else if (turn == 77)
  {
    if (friend_outgoingUsed[4])//�������
    {
      addStatusFriend(0, 20);
      addStatusFriend(3, 20);
      addStatusFriend(5, 56);
    }
    else
    {
      //just guess, to be filled
      addStatusFriend(0, 16);
      addStatusFriend(3, 16);
      addStatusFriend(5, 43);
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
  stage = ST_event;
  if (turn % 6 == 5)stage = ST_pickBuff;
  //�����������˻���ѡ���˳��У�stageҲ���ܻᱻ�ĳ�ST_decideEvent

  bool trainingSucceed = false;

  if (isRacing)
  {
    //�̶�����������checkEventAfterTrain()�ﴦ��
    assert(train == T_race);
    addLgGauge(lg_trainingColor[T_race], 1);
  }
  else
  {
    if (train == T_rest)//��Ϣ
    {
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
      if (turn <= 12 || turn >= 72)
      {
        printEvents("Cannot race now.");
        return false;
      }
      addAllStatus(1);//������
      runRace(2, 40);//���ԵĽ���

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


  return true;
}

void Game::applyNormalTraining(std::mt19937_64& rand, int16_t train, bool success)
{
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
      }
      else if (p >= PS_npc0 && p <= PS_npc4)//npc
      {
        addJiBan(p, 7, 0);
      }
      else if (p == PS_noncardYayoi)//�ǿ����³�
      {
        int jiban = friendship_noncard_yayoi;
        int g = jiban < 40 ? 2 : jiban < 60 ? 3 : jiban < 80 ? 4 : 5;
        skillPt += g;
        addJiBan(PS_noncardYayoi, 7, false);
      }
      else if (p == PS_noncardReporter)//����
      {
        int jiban = friendship_noncard_reporter;
        int g = jiban < 40 ? 2 : jiban < 60 ? 3 : jiban < 80 ? 4 : 5;
        addStatus(train, g);
        addJiBan(PS_noncardReporter, 7, false);
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


void Game::addYayoiJiBan(int value)
{
  if (friend_type != 0)
    addJiBan(friend_personId, value, true);
  else
    addJiBan(PS_noncardYayoi, value, true);
}

int Game::getYayoiJiBan() const
{
  if (friend_type != 0)
    return persons[friend_personId].friendship;
  else
    return friendship_noncard_yayoi;
}

void Game::randomPickBuff(std::mt19937_64& rand)
{

}

void Game::checkEvent(std::mt19937_64& rand)
{
  assert(stage == ST_event);
  checkFixedEvents(rand);
  checkRandomEvents(rand);


  //�غ���+1
  turn++;
  
  if (turn >= TOTAL_TURN)
  {
    printEvents("���ɽ���!");
    printEvents("��ĵ÷��ǣ�" + to_string(finalScore()));
  }
  else {
    isRacing = isRacingTurn[turn];
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
      addYayoiJiBan(4);
    }
    else if (turn == 73)//ura1
    {
      runRace(10, 40);
    }
    else if (turn == 75)//ura1
    {
      runRace(10, 60);
    }
    else if (turn == 77)//ura3
    {
      runRace(10, 80);
    }

  }

  if (turn == 11)//������
  {
    assert(isRacing);
  }
  else if (turn == 23)//��һ�����
  {
    //����¼���������ѡ������������ѡ����
    {
      int vitalSpace = maxVital - vital;//�������������
      handleFriendFixedEvent();
      if (vitalSpace >= 20)
        addVital(20);
      else
        addAllStatus(5);
    }
    printEvents("��һ�����");
  }
  else if (turn == 29)//�ڶ���̳�
  {

    for (int i = 0; i < 5; i++)
      addStatus(i, zhongMaBlueCount[i] * 6); //�����ӵ���ֵ

    double factor = double(rand() % 65536) / 65536 * 2;//�籾�������0~2��
    for (int i = 0; i < 5; i++)
      addStatus(i, int(factor*zhongMaExtraBonus[i])); //�籾����
    skillPt += int((0.5 + 0.5 * factor) * zhongMaExtraBonus[5]);//���߰��㼼�ܵĵ�Чpt

    for (int i = 0; i < 5; i++)
      fiveStatusLimit[i] += zhongMaBlueCount[i] * 2; //��������--�������ֵ��18�����μ̳й��Ӵ�Լ36���ޣ�ÿ��ÿ��������+1���ޣ�1200�۰��ٳ�2

    for (int i = 0; i < 5; i++)
      fiveStatusLimit[i] += rand() % 8; //��������--�����μ̳��������

    printEvents("�ڶ���̳�");
  }
  else if (turn == 35)
  {
    printEvents("�ڶ�����޿�ʼ");
  }
  else if (turn == 47)//�ڶ������
  {
    //����¼���������ѡ������������ѡ����
    {
      int vitalSpace = maxVital - vital;//�������������
      if (vitalSpace >= 30)
        addVital(30);
      else
        addAllStatus(8);
    }
    printEvents("�ڶ������");
  }
  else if (turn == 48)//�齱
  {
    int rd = rand() % 100;
    if (rd < 16)//��Ȫ��һ�Ƚ�
    {
      addVital(30);
      addAllStatus(10);
      addMotivation(2);

      printEvents("�齱�����������Ȫ/һ�Ƚ�");
    }
    else if (rd < 16 + 27)//���Ƚ�
    {
      addVital(20);
      addAllStatus(5);
      addMotivation(1);
      printEvents("�齱��������˶��Ƚ�");
    }
    else if (rd < 16 + 27 + 46)//���Ƚ�
    {
      addVital(20);
      printEvents("�齱������������Ƚ�");
    }
    else//��ֽ
    {
      addMotivation(-1);
      printEvents("�齱��������˲�ֽ");
    }
  }
  else if (turn == 49)
  {
    skillScore += 170;
    printEvents("���еȼ�+1");
  }
  else if (turn == 53)//������̳�
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

    printEvents("������̳�");

    if (getYayoiJiBan() >= 60)
    {
      skillScore += 170;//���м��ܵȼ�+1
      addMotivation(1);
    }
    else
    {
      addVital(-5);
      skillPt += 25;
    }
  }
  else if (turn == 59)
  {
    printEvents("��������޿�ʼ");
  }
  else if (turn == 70)
  {
    skillScore += 170;//���м��ܵȼ�+1
  }
  else if (turn == 77)//ura3����Ϸ����
  {
    //�����Ѿ���ǰ�洦����
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
      skillPt += 40;//�籾��
      addAllStatus(60);
      skillPt += 150;
    }
    else 
    {
      addAllStatus(25);
      //there should be something, but not important
    }


    //���˿��¼�
    handleFriendFixedEvent();

    addAllStatus(5);
    skillPt += 20;

    printEvents("ura3��������Ϸ����");
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
      if (randBool(rand, unlockOutgoingProb))//����
      {
        handleFriendUnlock(rand);
      }
    }
  }

  //ģ���������¼�

  //֧Ԯ�������¼��������һ������5�
  if (randBool(rand, GameConstants::EventProb))
  {
    int card = rand() % 6;
    addJiBan(card, 5, false);
    //addAllStatus(4);
    addStatus(rand() % 5, gameSettings.eventStrength);
    skillPt += gameSettings.eventStrength;
    printEvents("ģ��֧Ԯ������¼���" + persons[card].cardParam.cardName + " ���+5��pt���������+" + to_string(gameSettings.eventStrength));

    //֧Ԯ��һ����ǰ�����¼�������
    if (randBool(rand, 0.4 * (1.0 - turn * 1.0 / TOTAL_TURN)))
    {
      addMotivation(1);
      printEvents("ģ��֧Ԯ������¼�������+1");
    }
    if (randBool(rand, 0.5))
    {
      addVital(10);
      printEvents("ģ��֧Ԯ������¼�������+10");
    }
    else if (randBool(rand, 0.03))
    {
      addVital(-10);
      printEvents("ģ��֧Ԯ������¼�������-10");
    }
    if (randBool(rand, 0.03))
    {
      isPositiveThinking = true;
      printEvents("ģ��֧Ԯ������¼�����á�����˼����");
    }
  }

  //ģ����������¼�
  if (randBool(rand, 0.1))
  {
    addAllStatus(3);
    printEvents("ģ����������¼���ȫ����+3");
  }

  //������
  if (randBool(rand, 0.10))
  {
    addVital(5);
    printEvents("ģ������¼�������+5");
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
  if (turn >= 12 && randBool(rand, 0.04))
  {
    addMotivation(-1);
    printEvents("ģ������¼���\033[0m\033[33m����-1\033[0m\033[32m");
  }

}
void Game::applyAction(std::mt19937_64& rand, Action action)
{
  if (isEnd()) return;
  //assert(turn < TOTAL_TURN && "Game::applyTrainingAndNextTurn��Ϸ�ѽ���");
  //assert(!(isRacing && !isUraRace) && "��ura�ı����غ϶���checkEventAfterTrain��������");
  if (action.idx != T_none || isRacing)
  {
    bool suc = applyTraining(rand, action.idx);
    assert(suc && "Game::applyActionѡ���˲��Ϸ���ѵ��");
    
    checkEvent(rand);
    if (isEnd()) return;

    randomizeTurn(rand);


    //��ura�ı����غ�Ҳ���ܳԲˣ�����ˢpt�����Բ�����
    
    //if (isRacing && !isUraRace)//��ura�ı����غϣ�ֱ��������һ���غ�
    //{
    //  Action emptyAction;
    //  emptyAction.train = TRA_none;
    //  emptyAction.dishType = DISH_none;
    //  applyAction(rand, emptyAction);
    //}
  }
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
  if (buffId < 0)return -1;
  int idx = buffId % 19;
  if (idx < 4)return 1;
  else if (idx < 10)return 2;
  else return 3;
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
  return std::string();
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
