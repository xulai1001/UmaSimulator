#include <cassert>
#include <iostream>
#include "Evaluator.h"
#include "../Search/Search.h"


const double statusWeights[5] = { 6,6,6,6,6 };
const double jibanValue = 3;
const double vitalFactorStart = 3;
const double vitalFactorEnd = 10;
const double vitalScaleTraining = 1.0;

const double reserveStatusFactor = 50;//������ʱ��ÿ�غ�Ԥ�����٣���0�����ӵ��������

const double smallFailValue = -300;
const double bigFailValue = -800;
const double outgoingBonusIfNotFullMotivation = 300;//������ʱ����������
const double raceBonus = 150;//�������棬����������

const double lg_controlColorFactor = 50;


//һ���ֶκ���������������
inline double statusSoftFunction(double x, double reserve, double reserveInvX2)//reserve�ǿ����Ա����ռ䣨����Ȩ�أ���reserveInvX2��1/(2*reserve)
{
  if (x >= 0)return 0;
  if (x > -reserve)return -x * x * reserveInvX2;
  return x + 0.5 * reserve;
}

static void statusGainEvaluation(const Game& g, double* result) { //result����������ѵ���Ĺ�ֵ
  int remainTurn = TOTAL_TURN - g.turn - 1;//���ѵ�����м���ѵ���غ�
  //ura�ڼ��һ���غ���Ϊ�����غϣ���˲���Ҫ���⴦��
  //if (remainTurn == 2)remainTurn = 1;//ura�ڶ��غ�
  //else if (remainTurn >= 4)remainTurn -= 2;//ura��һ�غ�

  double reserve = reserveStatusFactor * remainTurn * (1 - double(remainTurn) / (TOTAL_TURN * 2));
  double reserveInvX2 = 1 / (2 * reserve);

  double finalBonus0 = 60;
  finalBonus0 += 30;//ura3�������¼�
  if (remainTurn >= 1)finalBonus0 += 20;//ura2
  if (remainTurn >= 2)finalBonus0 += 20;//ura1

  double remain[5]; //ÿ�����Ի��ж��ٿռ�

  for (int i = 0; i < 5; i++)
  {
    remain[i] = g.fiveStatusLimit[i] - g.fiveStatus[i] - finalBonus0;
  }

  if (g.friend_type != 0)
  {
    remain[0] -= 25;
    remain[4] -= 25;
  }


  for (int tra = 0; tra < 5; tra++)
  {
    double res = 0;
    for (int sta = 0; sta < 5; sta++)
    {
      double s0 = statusSoftFunction(-remain[sta], reserve, reserveInvX2);
      double s1 = statusSoftFunction(g.trainValue[tra][sta] - remain[sta], reserve, reserveInvX2);
      res += statusWeights[sta] * (s1 - s0);
    }
    res += g.gameSettings.ptScoreRate * g.trainValue[tra][5];
    result[tra] = res;
  }
}




static double calculateMaxVitalEquvalant(const Game& g)
{
  int t = g.turn;
  if (g.turn == 72)
    return 0;//���һ�غ�
  int nonRaceTurn = 0;//71�غ�ǰ��ÿ��ѵ���غϰ�����15��������
  for (int i = 72; i > g.turn; i--)
  {
    if (!g.isRacingTurn[i])nonRaceTurn++;
    if (nonRaceTurn >= 6)break;
  }
  int maxVitalEq = 28 + 25 * nonRaceTurn;
  if(maxVitalEq>g.maxVital)
    maxVitalEq = g.maxVital;
  return maxVitalEq;

}

static double vitalEvaluation(int vital, int maxVital)
{
  if (vital <= 50)
    return 2.0 * vital;
  else if (vital <= 70)
    return 1.5 * (vital - 50) + vitalEvaluation(50, maxVital);
  else if (vital <= maxVital)
    return 1.0 * (vital - 70) + vitalEvaluation(70, maxVital);
  else
    return vitalEvaluation(maxVital, maxVital);
}

const double LgBuffValuesForRed[3 * 19] = { //��������ɫ����������������̵������޶��ӳ�
  1,3,2,4, 7,6,4,7,8,7, 4,10,6,9,15,14,21,8,25,
  1,3,2,4, 7,6,4,8,6,4, 15,22,20,17,12,16,21,10,24,
  1,3,2,6, 7,6,4,8,4,5, 12,14,15,22,26,7,24,10,13
};

double getLgBuffColorWrongProb(int c0, int c1, int c2)
{
  int total = c0 + c1 + c2;
  assert(total <= 6);
  if (c0 >= 4 || (c0 >= 3 && c1 > 0 && c2 > 0))
    return 0;//�ٷְ�ȷ����ɫ
  if (c1 >= 4 || (c1 >= 3 && c0 > 0 && c2 > 0))
    return 1;//�ٷְ�ȷ����ɫ
  if (c2 >= 4 || (c2 >= 3 && c0 > 0 && c1 > 0))
    return 1;//�ٷְ�ȷ����ɫ

  if (c1 < c2)
  {
    int t = c1;
    c1 = c2;
    c2 = t;
  }
  if (total == 6)
  {
    if (c0 == 3 && c1 == 3 && c2 == 0)
      return 0.5;
    else if (c0 == 2 && c1 == 2 && c2 == 2)
      return 0.667;
    else if (c0 == 0 && c1 == 3 && c2 == 3)
      return 1;
    else assert(false);
  }
  else if (total == 5)
  {
    if (c0 == 3 && c1 == 2 && c2 == 0)
      return 0.05;
    else if (c0 == 2 && c1 == 3 && c2 == 0)
      return 0.7;
    else if (c0 == 2 && c1 == 2 && c2 == 1)
      return 0.3;//����0.5��Ҫ���������ܶ���
    else if (c0 == 1 && c1 == 2 && c2 == 2)
      return 0.8;
    else if (c0 == 0 && c1 == 3 && c2 == 2)
      return 1;
    else assert(false);
  }
  else if (total == 4)
  {
    if (c0 == 3 && c1 == 1 && c2 == 0)
      return 0.01;
    else if (c0 == 2 && c1 == 2 && c2 == 0)
      return 0.15;
    else if (c0 == 2 && c1 == 1 && c2 == 1)
      return 0.1;
    else if (c0 == 1 && c1 == 3 && c2 == 0)
      return 0.8;
    else if (c0 == 1 && c1 == 2 && c2 == 1)
      return 0.5;
    else if (c0 == 0 && c1 == 3 && c2 == 1)
      return 1;
    else if (c0 == 0 && c1 == 2 && c2 == 2)
      return 0.85;
    else assert(false);
  }
  else if (total == 3)
  {
    if (c0 == 3 && c1 == 0 && c2 == 0)
      return 0.001;
    else if (c0 == 2 && c1 == 1 && c2 == 0)
      return 0.05;
    else if (c0 == 1 && c1 == 2 && c2 == 0)
      return 0.15;
    else if (c0 == 1 && c1 == 1 && c2 == 1)
      return 0.13;
    else if (c0 == 0 && c1 == 3 && c2 == 0)
      return 0.9;
    else if (c0 == 0 && c1 == 2 && c2 == 1)
      return 0.7;
    else assert(false);
  }
  else if (total == 2)
  {
    if (c0 == 2 && c1 == 0 && c2 == 0)
      return 0.01;
    else if (c0 == 1 && c1 == 1 && c2 == 0)
      return 0.1;
    else if (c0 == 0 && c1 == 2 && c2 == 0)
      return 0.2;
    else if (c0 == 0 && c1 == 1 && c2 == 1)
      return 0.15;
    else assert(false);
  }
  else if (total == 1)
  {
    if (c0 == 1 && c1 == 0 && c2 == 0)
      return 0.1;
    else if (c0 == 0 && c1 == 1 && c2 == 0)
      return 0.2;
    else assert(false);
  }
  else if (total == 0)
    return 0.0;

  return 9999;
}

double getLgBuffEva(const Game& game, int idx)
{
  double v = LgBuffValuesForRed[idx];
  //���˫�
  if (idx == 0 * 19 + 2 || idx == 1 * 19 + 2 || idx == 2 * 19 + 2 || idx == 2 * 19 + 10)
  {
    int extraJiban = game.lg_bonus.jibanAdd1 + game.lg_bonus.jibanAdd2 + (game.isAiJiao ? 2 : 0);
    if (extraJiban < 3)
    {
      v += 3.5;
    }
  }
  //��ɫ
  if (game.turn < 36 && game.gameSettings.color_priority >= 0)
  {
    int counts[3] = { 0,0,0 };
    for (int i = 0; i < game.turn / 6; i++)
    {
      int16_t color = game.lg_buffs[i].getBuffColor();
      assert(color >= 0);
      counts[color] += 1;
    }
    int16_t newColor = idx / 19;
    counts[newColor] += 1;

    int count0 = counts[game.gameSettings.color_priority];
    int count1 = counts[(game.gameSettings.color_priority + 1) % 3];
    int count2 = counts[(game.gameSettings.color_priority + 2) % 3];
    v -= lg_controlColorFactor * getLgBuffColorWrongProb(count0, count1, count2);
  }
  return v;
}

Action Evaluator::handWrittenStrategy(const Game& game)
{
  auto allActions = game.getAllLegalActions();
  if (allActions.size() == 1)
    return allActions[0];
  if (allActions.size() == 0)
    return Action();

  if (game.stage == ST_decideEvent)
  {
    if (game.decidingEvent == DecidingEvent_outing)
    {
      if (!game.friend_outgoingUsed[0])
        return Action(ST_decideEvent, 1);
      else if (!game.friend_outgoingUsed[1])
        return Action(ST_decideEvent, 2);
      else if (!game.friend_outgoingUsed[2])
        return Action(ST_decideEvent, 3);
      else if (!game.friend_outgoingUsed[3])
      {
        //��4�γ��У�ѡ��������ٵ�

        if (game.turn < 36 && game.gameSettings.color_priority >= 0 && game.lg_gauge[game.gameSettings.color_priority] < 8)
          return Action(ST_decideEvent, 4 + game.gameSettings.color_priority);

        int minGauge = 100;
        int minGaugeIdx = -1;
        for (int i = 0; i < 3; i++)
        {
          if (game.lg_gauge[i] < minGauge)
          {
            minGauge = game.lg_gauge[i];
            minGaugeIdx = i;
          }
        }
        return Action(ST_decideEvent, 4 + minGaugeIdx);
      }
      else if (!game.friend_outgoingUsed[4])
        return Action(ST_decideEvent, 7);
      else throw "handWrittenStrategy���˳���������";
    }
    else if (game.decidingEvent == DecidingEvent_three)//���Ȳ���8��
    {
      if (game.lg_gauge[2] == 7)
        return Action(ST_decideEvent, 2);
      if (game.lg_gauge[1] == 7)
        return Action(ST_decideEvent, 1);
      if (game.lg_gauge[0] == 7)
        return Action(ST_decideEvent, 0);
      if (game.lg_gauge[2] < 8)
        return Action(ST_decideEvent, 2);
      if (game.lg_gauge[1] < 8)
        return Action(ST_decideEvent, 1);
      if (game.lg_gauge[0] < 8)
        return Action(ST_decideEvent, 0);
      return Action(ST_decideEvent, 2);
    }
    else throw "handWrittenStrategyδ֪decidingEvent";
  }
  else if (game.stage == ST_chooseBuff)
  {
    double bestValue = -1e9;
    int bestBuff = -1;
    for (int i = 0; i < game.lg_pickedBuffsNum; i++)
    {
      double v = getLgBuffEva(game, game.lg_pickedBuffs[i]);
      if (v > bestValue)
      {
        bestValue = v;
        bestBuff = i;
      }
    }
    if (game.turn == 65)
    {
      bestBuff = bestBuff + 10;//�滻����һ��buff
    }
    return Action(ST_chooseBuff, bestBuff);
  }
  else if (game.stage == ST_train)
  {
    Action bestAction;
    bestAction.stage = ST_train;
    bestAction.idx = -1;

    if (game.isEnd())return bestAction;
    //����
    if (game.isRacing)
    {
      bestAction.idx = T_race;
      return bestAction;
    }




    double bestValue = -1e4;


    double vitalFactor = vitalFactorStart + (game.turn / double(TOTAL_TURN)) * (vitalFactorEnd - vitalFactorStart);

    int maxVitalEquvalant = calculateMaxVitalEquvalant(game);
    double vitalEvalBeforeTrain = vitalEvaluation(std::min(maxVitalEquvalant, int(game.vital)), game.maxVital);


    //���/��Ϣ
    {
      bool isFriendOutgoingAvailable =
        game.friend_type != 0 &&
        game.friend_stage >= 2 &&
        !game.friend_outgoingUsed[4] &&
        (!game.isXiahesu());
      Action action(ST_train,T_rest);
      if (isFriendOutgoingAvailable || game.isXiahesu())action.idx = T_outgoing;//������������������������Ϣ

      int vitalGain = isFriendOutgoingAvailable ? 50 : game.isXiahesu() ? 40 : 50;
      bool addMotivation = game.motivation < 5 && action.idx == T_outgoing;

      int vitalAfterRest = std::min(maxVitalEquvalant, vitalGain + game.vital);
      double value = vitalFactor * (vitalEvaluation(vitalAfterRest, game.maxVital) - vitalEvalBeforeTrain);
      if (addMotivation)value += outgoingBonusIfNotFullMotivation;


      if (PrintHandwrittenLogicValueForDebug)
        std::cout << action.toString() << " " << value << std::endl;
      if (value > bestValue)
      {
        bestValue = value;
        bestAction = action;
      }
    }
    //����
    if (game.isRaceAvailable())
    {
      double value = raceBonus;


      int vitalAfterRace = std::min(maxVitalEquvalant, -15 + game.vital);
      value += vitalFactor * (vitalEvaluation(vitalAfterRace, game.maxVital) - vitalEvalBeforeTrain);


      if (PrintHandwrittenLogicValueForDebug)
        std::cout << " " << value << std::endl;
      if (value > bestValue)
      {
        bestValue = value;
        bestAction.idx = T_race;
      }
    }


    //ѵ��

    //���ҵ���õ�ѵ����Ȼ�����Ҫ��Ҫ�Բ�
    {
      double statusGainE[5];
      statusGainEvaluation(game, statusGainE);


      for (int tra = 0; tra < 5; tra++)
      {
        double value = statusGainE[tra];


        //����hint���
        int cardHintNum = 0;//����hint���ȡһ�������Դ�ֵ�ʱ��ȡƽ��
        for (int j = 0; j < 5; j++)
        {
          int p = game.personDistribution[tra][j];
          if (p < 0)break;//û��
          if (p >= 6)continue;//���ǿ�
          if (game.persons[p].isHint)
            cardHintNum += 1;
        }
        double hintProb = 1.0 / cardHintNum;
        bool haveFriend = false;
        for (int j = 0; j < 5; j++)
        {
          int pi = game.personDistribution[tra][j];
          if (pi < 0)break;//û��
          if (pi >= 6)continue;//���ǿ�
          const Person& p = game.persons[pi];
          if (p.personType == PersonType_scenarioCard)//���˿�
          {
            haveFriend = true;
            if (game.friend_stage == FriendStage_notClicked)
              value += 150;
            else if (game.friend_qingre)
              value += 30;
            else if (game.friend_stage == FriendStage_afterUnlockOutgoing)
              value += 100;
            else 
              value += 20;
          }
          else if (p.personType == PersonType_card)
          {
            if (p.friendship < 80)
            {
              double jibanAdd = 7;
              if (game.friend_type == 1)
                jibanAdd += 1;
              if (haveFriend && game.friend_type == 1)
                jibanAdd += 2;
              if (game.isAiJiao)jibanAdd += 2;
              if (p.isHint)
              {
                jibanAdd += 5 * hintProb;
                if (game.isAiJiao)jibanAdd += 2 * hintProb;
              }
              jibanAdd = std::min(double(80 - p.friendship), jibanAdd);

              value += jibanAdd * jibanValue;
            }

            if (p.isHint)
            {
              double hintBonus = p.cardParam.hintLevel == 0 ?
                (1.6 * (statusWeights[0] + statusWeights[1] + statusWeights[2] + statusWeights[3] + statusWeights[4])) :
                game.gameSettings.hintPtRate * game.gameSettings.ptScoreRate * p.cardParam.hintLevel;
              value += hintBonus * hintProb;
            }
          }

        }


        //�����ϣ���ֵ��Ҫ���ϲ˵�ѵ���ӳ�Ȼ���ȥ�˵Ŀ����������ڸ��ӣ����ÿ�����
        int vitalAfterTrain = std::min(maxVitalEquvalant, game.trainVitalChange[tra] + game.vital);
        value += vitalScaleTraining * vitalFactor * (vitalEvaluation(vitalAfterTrain, game.maxVital) - vitalEvalBeforeTrain);

        //��ĿǰΪֹ����ѵ���ɹ���value
        //����Բ�֮������������¼���ʧ����
        double failRate = game.failRate[tra];
        

        if (failRate > 0)
        {
          double bigFailProb = failRate;
          if (failRate < 20)bigFailProb = 0;
          double failValueAvg = 0.01 * bigFailProb * bigFailValue + (1 - 0.01 * bigFailProb) * smallFailValue;

          value = 0.01 * failRate * failValueAvg + (1 - 0.01 * failRate) * value;
        }

        Action action(ST_train,tra);

        if (value > bestValue)
        {
          bestValue = value;
          bestAction = action;
        }
        if (PrintHandwrittenLogicValueForDebug)
          std::cout << action.toString() << " " << value << std::endl;
      }


    }
    return bestAction;
  }
  else throw "δ֪stage";
  return Action();
}

