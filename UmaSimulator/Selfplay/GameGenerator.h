﻿#pragma once
#include <vector>
#include "SelfplayParam.h"
#include "../Game/Game.h"
#include "../NeuralNet/Evaluator.h"
class GameGenerator
{
  //先随机生成一些开局，然后随机往后进行一些回合数，储存在gameBuf，在发出来之前再加随机性
  SelfplayParam param;
  Evaluator evaluator;
  
  std::mt19937_64 rand;
  std::vector<Game> gameBuf;
  int nextGamePointer;

  std::vector<int> cardRank[5];//速耐力根智卡的排行

  void loadCardRankFile();

  Game randomOpening();
  Game randomizeBeforeOutput(const Game& game0);
  void newGameBatch();
  bool isVaildGame(const Game& game);


  std::vector<int> getRandomCardset(); //获取一组随机卡组，概率带友人，越好的卡的概率越大
  void randomizeUmaCardParam(Game& game); //给卡组
public:
  GameGenerator(SelfplayParam param, Model* model);
  Game get();
};