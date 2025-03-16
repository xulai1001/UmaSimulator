#pragma once
#include <cstdint>
#include <random>
#include <string>
#include "../SupportCard/SupportCard.h"
struct Game;


enum PersonTypeEnum :int8_t
{
  PersonType_unknown = 0,
  PersonType_scenarioCard,
  PersonType_card,
  PersonType_npc,
  PersonType_yayoi,
  PersonType_reporter,
  PersonType_otherFriend,
  PersonType_groupCard
};

enum FriendStageEnum :int8_t
{
  FriendStage_notClicked = 0,
  FriendStage_beforeUnlockOutgoing,
  FriendStage_afterUnlockOutgoing,
  FriendStage_refusedOutgoing
};

struct Person //任何一个可能出现在训练里的人头
{
  //bool isCard;//是否为支援卡，否则为理事长记者或者不带卡的凉花   用personType替代了
  SupportCard cardParam;//支援卡参数
  int8_t personType;//0代表未知，1代表剧本友人支援卡（R或SSR都行），2代表普通支援卡 //其他的种田杯ai不放在person类里了（3代表npc(uaf剧本没有)，4理事长，5记者，6不带卡的凉花，7其他友人卡，8其他团队卡。）
  int16_t charaId;//人头对应的马娘id，懒得写可以一律0（只用于获得名字）

  int8_t friendship;//羁绊
  //bool atTrain[5];//是否在五个训练里。对于普通的卡只是one-hot或者全空，对于ssr佐岳可能有两个true
  bool isHint;//是否有hint。友人卡或者npc恒为false
  int8_t cardRecord;//记录一些可能随着时间而改变的参数，例如根涡轮的固有
  //int8_t friendOrGroupCardStage;//只对友人卡团队卡有效，0是未点击，1是已点击但未解锁出行，2是已解锁出行但没情热，3是情热状态
  //int8_t groupCardShiningContinuousTurns;//团队卡情热了几个回合了（下回合结束情热的概率与此有关，数据可以在大师杯版ai里找到）


  
  Person();//未加载的人头
  void setCard(int cardId);//把此人头设置为某个支援卡，只考虑刚开局的状态，如果是游戏半途，需要手动修改羁绊等
  

  void getCardNNInputV1(float* buf, const Game& game, int index) const;//神经网络输入向量，不包括支援卡参数，Game类会把支援卡参数放在对应位置
  std::string getPersonName() const;//获得人物名称
  //std::string getPersonStrColored(const Game& game, int personId, int atTrain) const;//人物名称与羁绊等整合成带颜色的字符串，在小黑板表格中显示
};