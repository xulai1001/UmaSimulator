#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../config.h"

const int TOTAL_TURN = 78;
const int MAX_INFO_PERSON_NUM = 6;//有单独信息的人头个数（此剧本只有支援卡）

class GameConstants
{
public:
  static const int TrainingBasicValue[5][5][7]; //TrainingBasicValue[颜色][第几种训练][LV几][速耐力根智pt体力]
  static const int FailRateBasic[5][5];//[第几种训练][LV几]，失败率= 0.025*(x0-x)^2 + 1.25*(x0-x)
  static const int BasicFiveStatusLimit[5];//初始上限，1200以上翻倍

  //各种游戏参数
  //static const int NormalRaceFiveStatusBonus;//常规比赛属性加成=3，特殊马娘特殊处理（狄杜斯等）
  //static const int NormalRacePtBonus;//常规比赛pt加成
  static const double EventProb;//每回合有EventProb概率随机一个属性以及pt +EventStrengthDefault，模拟支援卡事件
  static const int EventStrengthDefault;

  //剧本卡相关
  static const int FriendCardIdSSR = 30241;//SSR团
  static const int FriendCardIdR = 19999;//无r卡
  static const double FriendUnlockOutgoingProbEveryTurnLowFriendship;//每回合解锁外出的概率，羁绊小于60
  static const double FriendUnlockOutgoingProbEveryTurnHighFriendship;//每回合解锁外出的概率，羁绊大于等于60
  //static const double FriendEventProb;//友人事件概率//常数0.4写死在对应函数里了
  static const double FriendVitalBonusSSR[5];//友人SSR卡的回复量倍数（满破1.6）
  static const double FriendVitalBonusR[5];//友人R卡的回复量倍数
  static const double FriendStatusBonusSSR[5];//友人SSR卡的事件效果倍数（满破1.25）
  static const double FriendStatusBonusR[5];//友人R卡的事件效果倍数
  

  //剧本相关
  static const std::vector<int> LinkCharas;// Link角色


  static const double LG_redLvXunlianCard[10];//红登不同等级的卡的训练加成
  static const double LG_redLvXunlianNPC[10];//红登不同等级的npc的训练加成
  static const double LG_redLvYouqingNPC[10];//红登不同等级的npc的友情加成



  //评分
  static const int FiveStatusFinalScore[1200+800*2+1];//不同属性对应的评分
  static const double ScorePtRateDefault;//为了方便，直接视为每1pt对应多少分。
  static const double HintLevelPtRateDefault;//为了方便，直接视为每一级hint多少pt。
  static const double HintProbTimeConstantDefault;//为了方便，直接视为每一级hint多少pt。
  //static const double ScorePtRateQieZhe;//为了方便，直接视为每1pt对应多少分。切者

  static bool isLinkChara(int id);

};