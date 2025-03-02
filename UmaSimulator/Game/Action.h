#pragma once
#include <cstdint>
#include <random>
#include <string>

enum TrainActionTypeEnum :int16_t
{
  TRA_speed = 0,
  TRA_stamina,
  TRA_power,
  TRA_guts,
  TRA_wiz,
  TRA_rest, 
  TRA_outgoing, //包括合宿的“休息&外出”
  TRA_race,
  TRA_none = -1, //此Action不训练，只做菜
  TRA_redistributeCardsForTest = -2 //使用这个标记时，说明要randomDistributeCards，用于测试ai分数，在Search::searchSingleActionThread中使用
};


struct Action 
{
  static const std::string trainingName[8];
  static const std::string dishName[14];
  static const Action Action_RedistributeCardsForTest;
  static const int MAX_ACTION_TYPE = 21;//标准的Action有编号，8+13=21种
  static const int MAX_TWOSTAGE_ACTION_TYPE = 21 + 8 + 8;//二阶段搜索考虑的最多Action个数，只有两种1级菜需要考虑二阶段搜索，8+13+2*8=37种


  
  int16_t dishType;//做菜，0为不做菜

  int16_t train;//-1暂时不训练，01234速耐力根智，5外出，6休息，7比赛 
  //注：外出是优先友人外出，没有再普通外出，不提供选项
  bool isActionStandard() const;
  int toInt() const;
  std::string toString() const;
  static Action intToAction(int i);
};