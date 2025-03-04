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

  std::vector<int> probs = { 1,1,1,1,1,1 }; //���������Ǹ�
  distribution = std::discrete_distribution<>(probs.begin(), probs.end());
}

void Person::setCard(int cardId)
{
  cardParam = GameDatabase::AllCards[cardId];


  friendship = cardParam.initialJiBan;
  isHint = false;
  cardRecord = 0;

  int cardType = cardParam.cardType;
  if (cardType == 5 || cardType == 6)//�����Ŷӿ�
  {
    int realCardId = cardId / 10;

    std::vector<int> probs = { 100,100,100,100,100,100 }; //�������ʣ����������Ǹ�
    distribution = std::discrete_distribution<>(probs.begin(), probs.end());

    if (realCardId == GameConstants::FriendCardIdR || realCardId == GameConstants::FriendCardIdSSR)//�籾���˿�
    {
      personType = PersonType_scenarioCard;
    }
    else
    {
      throw string("��֧�ִ��籾����������˻��Ŷӿ�");
    }
  }
  else if (cardType >= 0 && cardType <= 4)//���������ǿ�
  {
    personType = 2;
    std::vector<int> probs = { 100,100,100,100,100,50 }; //�������ʣ����������Ǹ�
    probs[cardType] += int(cardParam.deYiLv);
    distribution = std::discrete_distribution<>(probs.begin(), probs.end());
  }

}
void Person::setExtraDeyilvBonus(int deyilvBonus)
{
  if (personType == PersonType_card)
  {
    int newDeyilv = int((100.0 + cardParam.deYiLv) * (1.00 + 0.01 * deyilvBonus) - 100); //�Ҳ�֪������Ӧ�üӻ��ǳˣ�����ɳ˸���ģ�����
    std::vector<int> probs = { 100,100,100,100,100,50 }; //�������ʣ����������Ǹ�
    probs[cardParam.cardType] += newDeyilv;
    distribution = std::discrete_distribution<>(probs.begin(), probs.end());
  }
}
/*
void Person::setNonCard(int pType)
{
  personType = pType;
  if (personType != PersonType_lishizhang && personType != PersonType_jizhe && personType != PersonType_lianghuaNonCard)
  {
    assert(false && "setNonCardֻ���ڷ�֧Ԯ����ͷ�ĳ�ʼ��");
  }

  friendship = 0;
  isHint = false;
  cardRecord = 0;
  friendOrGroupCardStage = 0;
  groupCardShiningContinuousTurns = 0;
  std::vector<int> probs = { 100,100,100,100,100,200 }; //�������ʣ����������Ǹ�
  distribution = std::discrete_distribution<>(probs.begin(), probs.end());
}
*/