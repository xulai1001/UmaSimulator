#include <cassert>
#include <iostream>
#include "Evaluator.h"
#include "../Search/Search.h"


const double statusWeights[5] = { 5,5,7,6,6 };
const double jibanValue = 3.5;
const double vitalFactorStart = 2;
const double vitalFactorEnd = 4;
const double vitalScaleTraining = 1;

const double reserveStatusFactor = 40;//������ʱ��ÿ�غ�Ԥ�����٣���0�����ӵ��������

const double smallFailValue = -150;
const double bigFailValue = -500;
const double outgoingBonusIfNotFullMotivation = 150;//������ʱ����������
const double raceBonus = 150;//�������棬����������

//const double materialValue[5] = { 0.5,0.2,0.5,0.5,0.3 };//ÿ������ԭ�ϵĹ�ֵ
//const double materialValueScale = 1.0;//����ԭ�ϵĹ�ֵ�������ϵ��������һ���
const double greenBonusBasicYear1 = 100;//��ɫ����ļӳɣ��û��ʱ����ϵ������һ��
const double greenBonusBasicYear2 = 100;//��ɫ����ļӳɣ��ڶ���
const double greenBonusBasicYear3 = 100;//��ɫ����ļӳɣ�������

const double cookingThreholdFactorLv2 = 0.5;//Խ��ڶ�����Բ�Խ������2���ˣ�
const double cookingThreholdFactorLv3 = 1.0;//Խ�������Բ�Խ������3���ˣ�


//const double xiangtanExhaustLossMax = 800;//��̸�ľ���û���Ĺ�ֵ�۷�

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
  if (g.turn >= 76)
    return 0;//���һ�غϲ���Ҫ����
  if (g.turn > 71)
    return 10;//ura�ڼ���ԳԲ�
  if (g.turn == 71)
    return 30;//uraǰ���һ�غϣ�30����������
  int nonRaceTurn = 0;//71�غ�ǰ��ÿ��ѵ���غϰ�����15��������
  for (int i = 71; i > g.turn; i--)
  {
    if (!g.isRacingTurn[i])nonRaceTurn++;
    if (nonRaceTurn >= 6)break;
  }
  int maxVitalEq = 30 + 15 * nonRaceTurn;
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

static double materialEvaluation(int turn, int count) //�����Բ˵Ŀ����������ڶ�����Բ��Բ�
{
  double bias =
    turn < 48 ? 100 :
    turn < 60 ? 100 :
    turn < 68 ? 60 :
    10;

  double scale =
    turn < 48 ? 20 :
    turn < 60 ? 30 :
    turn < 68 ? 40 :
    40;

  return sqrt(count + bias) * scale;
}


Action Evaluator::handWrittenStrategy(const Game& game)
{
  return Action();
  /*
  Action bestAction;
  bestAction.dishType = DISH_none;
  bestAction.train = TRA_none;

  if (game.isEnd())return bestAction;
  //����
  if (game.isRacing)
  {
    if (game.turn < 72)//����������Բ�
    {
      bestAction.train = TRA_race;
      return bestAction;
    }
    if (game.turn == TOTAL_TURN - 1)//ura���һ�غϣ��ܳ�ʲô�ͳ�ʲô
    {
      //�Ӻ���ǰ������
      for (int i = DISH_g1plate; i >= 1; i--)
      {
        bestAction.dishType = i;
        if (game.isLegal(bestAction))return bestAction;
      }
      //ʲô���Բ���
      bestAction.dishType = DISH_none;
      bestAction.train = TRA_race;
      return bestAction;
    }


    //ura1��ura2����һ���ܲ��ܳ�g1plate
    int g1plateCost = game.cook_win_history[4] == 2 ? 80 : 100;

    bool haveG1Plate = true;
    for (int matType = 0; matType < 5; matType++)
    {
      int matGain = 1.5001 * GameConstants::Cook_HarvestBasic[game.cook_farm_level[matType]] / 2;
      if (matType == game.cook_main_race_material_type)
        matGain += 1.5001 * GameConstants::Cook_HarvestExtra[game.cook_farm_level[matType]];
      int reserveMin = game.turn == 73 ?
        2 * g1plateCost + 80 - 3 * 1.5001 * GameConstants::Cook_HarvestBasic[game.cook_farm_level[matType]] / 2 :
        2 * g1plateCost;//Ҫ��֤������ѵ���غ�һֱ���ԳԵ�g1plate
      if (game.cook_material[matType] < g1plateCost || game.cook_material[matType] + matGain < reserveMin)
      {
        haveG1Plate = false;
        break;
      }
    }
    if (haveG1Plate)
    {
      bestAction.dishType = DISH_g1plate;
      return bestAction;
    }
    else
    {
      //�����»غ�ѵ��û������
      bestAction.train = TRA_race;
      return bestAction;
    }
  }

  //ura�ڼ�����ܳ�G1Plate��ֱ�ӳԣ����򲻳�
  if (game.turn >= 72 && game.cook_dish==DISH_none)
  {
    if (game.isDishLegal(DISH_g1plate))
    {
      bestAction.dishType = DISH_g1plate;
      return bestAction;
    }
    //�����ǳԹ��˻��߳Բ��ˣ�Ѱ������ѵ��
  }


  //����ѵ���غ�
  
  //�������2��3���ˣ�ֱ��ѡ���Ӧѵ��
  if (game.cook_dish != DISH_none)
  {
    int dishLevel = GameConstants::Cook_DishLevel[game.cook_dish];
    if (dishLevel == 2 || dishLevel == 3)
    {
      int tra = GameConstants::Cook_DishMainTraining[game.cook_dish];
      bestAction.dishType = DISH_none;
      bestAction.train = tra;
      return bestAction;
    }
  }




  double bestValue = -1e4;


  double vitalFactor = vitalFactorStart + (game.turn / double(TOTAL_TURN)) * (vitalFactorEnd - vitalFactorStart);

  int maxVitalEquvalant = calculateMaxVitalEquvalant(game);
  double vitalEvalBeforeTrain = vitalEvaluation(std::min(maxVitalEquvalant, int(game.vital)), game.maxVital);

  double greenBonus =
    game.turn < 24 ? greenBonusBasicYear1 :
    game.turn < 48 ? greenBonusBasicYear2 :
    greenBonusBasicYear3;
  //�û��ʱ������ɫ����ӳ�
  for (int i = 0; i < 6; i++)
    if (game.persons[i].friendship < 80)
      greenBonus *= 0.85;

  //���/��Ϣ
  {
    bool isFriendOutgoingAvailable =
      game.friend_type != 0 &&
      game.friend_stage >= 2 &&
      game.friend_outgoingUsed < 5 &&
      (!game.isXiahesu());
    Action action;
    action.dishType = DISH_none;
    action.train = TRA_rest;
    if (isFriendOutgoingAvailable || game.isXiahesu())action.train = TRA_outgoing;//������������������������Ϣ

    int vitalGain = isFriendOutgoingAvailable ? 50 : game.isXiahesu() ? 40 : 50;
    bool addMotivation = game.motivation < 5 && action.train == TRA_outgoing;

    int vitalAfterRest = std::min(maxVitalEquvalant, vitalGain + game.vital);
    double value = vitalFactor * (vitalEvaluation(vitalAfterRest, game.maxVital) - vitalEvalBeforeTrain);
    if (addMotivation)value += outgoingBonusIfNotFullMotivation;

    bool isGreen = game.cook_train_green[action.train];
    if (isFriendOutgoingAvailable && action.train == TRA_outgoing)
      isGreen = true;
    if (isGreen)value += greenBonus;

    if (PrintHandwrittenLogicValueForDebug)
      std::cout << action.toString() << " " << value << std::endl;
    if (value > bestValue)
    {
      bestValue = value;
      bestAction = action;
    }
  }
  //����
  Action raceAction;
  raceAction.dishType = DISH_none;
  raceAction.train = TRA_race;
  if(game.isLegal(raceAction))
  {
    double value = raceBonus;


    int vitalAfterRace = std::min(maxVitalEquvalant, -15 + game.vital);
    value += vitalFactor * (vitalEvaluation(vitalAfterRace, game.maxVital) - vitalEvalBeforeTrain);

    if (game.cook_train_green[TRA_race])
      value += greenBonus;

    if (PrintHandwrittenLogicValueForDebug)
      std::cout << raceAction.toString() << " " << value << std::endl;
    if (value > bestValue)
    {
      bestValue = value;
      bestAction = raceAction;
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
          else if(p.friendship < 60)
            value += 100;
          else value += 40;
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
            double hintBonus = p.cardParam.hintLevel==0?
              (1.6 * (statusWeights[0] + statusWeights[1] + statusWeights[2] + statusWeights[3] + statusWeights[4])) :
              game.hintPtRate*game.ptScoreRate*p.cardParam.hintLevel;
            value += hintBonus * hintProb;
          }
        }

      }

      int bestDish = 0;
      //���ǳԸ��ֲ�
      //��һ�꣺�Լ���Ӧѵ��ֻ��һ�ֲ�
      //�ڶ��꣺ֻ����lv2��
      //�����꣺lv2��lv3������
      //ura��������Ѿ��Թ�g1plate�ˣ����ﲻ���ǳԲ�
      if (game.cook_dish == DISH_none)
      {
        if (game.turn < 24)//��һ��
        {
          if (game.cook_dish_pt < 3000)//�Ե�2500pt
          {
            if (tra == 0)
            {
              if (game.isDishLegal(DISH_curry))
                bestDish = DISH_curry;
              else if (game.isDishLegal(DISH_sandwich))
                bestDish = DISH_sandwich;
            }
            else if (tra == 1 || tra == 3)
            {
              if (game.isDishLegal(DISH_curry))
                bestDish = DISH_curry;
            }
            else if (tra == 2 || tra == 4)
            {
              if (game.isDishLegal(DISH_sandwich))
                bestDish = DISH_sandwich;
            }
          }
        }
        else if (game.turn < 72)//�ڶ�����
        {
          int dish1 = DISH_speed1 + tra;
          int dish2 = DISH_speed2 + tra;
          assert(GameConstants::Cook_DishMainTraining[dish1] == tra);
          assert(GameConstants::Cook_DishMainTraining[dish2] == tra);

          if (game.isDishLegal(dish1))
          {
            int mat0 = game.cook_material[tra];
            assert(mat0 >= 150);
            double matEval0 = materialEvaluation(game.turn, mat0);
            double matEval1 = materialEvaluation(game.turn, mat0 - 150);
            double matEval2 = (game.turn >= 48 && mat0 >= 250) ? materialEvaluation(game.turn, mat0 - 250) : -99999;
            double trValue = statusGainE[tra];

            double gain1 = trValue * cookingThreholdFactorLv2 - matEval0 + matEval1;
            double gain2 = trValue * cookingThreholdFactorLv3 - matEval0 + matEval2;
            //std::cout << game.turn << " " << gain1 << std::endl;


            if (gain1 > 0)
            {
              bestDish = dish1;
            }

            if (gain2 > 0 && gain2 > gain1)
            {
              bestDish = dish2;
            }

          }
          
        } 
      }

      //�����ϣ���ֵ��Ҫ���ϲ˵�ѵ���ӳ�Ȼ���ȥ�˵Ŀ����������ڸ��ӣ����ÿ�����
      int vitalAfterTrain = std::min(maxVitalEquvalant, game.trainVitalChange[tra] + game.vital);
      value += vitalScaleTraining * vitalFactor * (vitalEvaluation(vitalAfterTrain, game.maxVital) - vitalEvalBeforeTrain);

      //��ĿǰΪֹ����ѵ���ɹ���value
      //����Բ�֮������������¼���ʧ����
      double failRate = game.failRate[tra];
      if (bestDish != 0 && failRate > 0)
      {
        int dishLevel = GameConstants::Cook_DishLevel[bestDish];
        int dishMainTraining = GameConstants::Cook_DishMainTraining[bestDish];
        int vitalGainDish = dishLevel==1?0:
          dishLevel == 2 ? (game.cook_farm_level[dishMainTraining] >= 3 ? 5 : 0):
          dishLevel == 3 ? (game.cook_farm_level[dishMainTraining] >= 3 ? 20 : 15): //����һ���ɹ�������
          25;
        failRate -= 1.7 * vitalGainDish;//�ֲڹ���
        if (failRate < 0)failRate = 0;
      }


      if (failRate > 0)
      {
        double bigFailProb = failRate;
        if (failRate < 20)bigFailProb = 0;
        double failValueAvg = 0.01 * bigFailProb * bigFailValue + (1 - 0.01 * bigFailProb) * smallFailValue;
        
        value = 0.01 * failRate * failValueAvg + (1 - 0.01 * failRate) * value;
      }

      Action action;
      if (bestDish != DISH_none)//�ȳԲˣ���һ����ѵ��
      {
        action.dishType = bestDish;
        action.train = TRA_none;
      }
      else
      {
        action.dishType = DISH_none;
        action.train = tra;
      }

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
  */
}

