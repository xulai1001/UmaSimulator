#include "Person.h"
#include "../GameDatabase/GameDatabase.h"
using namespace std;
Person::Person()
{
  cardParam = SupportCard();
  personType = 0; 
  //cardId = 0;
  charaId = 0;

  friendship = 0;
  //for (int i = 0; i < 5; i++)atTrain[i] = false;
  isHint = false;
  cardRecord = 0;

}

void Person::setCard(int cardId)
{
  cardParam = GameDatabase::AllCards[cardId];


  friendship = cardParam.initialJiBan;
  isHint = false;
  cardRecord = 0;

  int cardType = cardParam.cardType;
  if (cardType == 5 || cardType == 6)//友人团队卡
  {
    int realCardId = cardId / 10;


    if (realCardId == GameConstants::FriendCardIdR || realCardId == GameConstants::FriendCardIdSSR)//剧本友人卡
    {
      personType = PersonType_scenarioCard;
    }
    else
    {
      throw string("不支持带剧本卡以外的友人或团队卡");
    }
  }
  else if (cardType >= 0 && cardType <= 4)//速耐力根智卡
  {
    personType = 2;
  }

}
/*
void Person::setNonCard(int pType)
{
  personType = pType;
  if (personType != PersonType_lishizhang && personType != PersonType_jizhe && personType != PersonType_lianghuaNonCard)
  {
    assert(false && "setNonCard只用于非支援卡人头的初始化");
  }

  friendship = 0;
  isHint = false;
  cardRecord = 0;
  friendOrGroupCardStage = 0;
  groupCardShiningContinuousTurns = 0;
  std::vector<int> probs = { 100,100,100,100,100,200 }; //基础概率，速耐力根智鸽
  distribution = std::discrete_distribution<>(probs.begin(), probs.end());
}
*/