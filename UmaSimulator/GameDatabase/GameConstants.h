#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../config.h"

const int TOTAL_TURN = 78;
const int MAX_INFO_PERSON_NUM = 6;//�е�����Ϣ����ͷ�������˾籾ֻ��֧Ԯ����

class GameConstants
{
public:
  static const int TrainingBasicValue[5][5][7]; //TrainingBasicValue[��ɫ][�ڼ���ѵ��][LV��][����������pt����]
  static const int FailRateBasic[5][5];//[�ڼ���ѵ��][LV��]��ʧ����= 0.025*(x0-x)^2 + 1.25*(x0-x)
  static const int BasicFiveStatusLimit[5];//��ʼ���ޣ�1200���Ϸ���

  //������Ϸ����
  //static const int NormalRaceFiveStatusBonus;//����������Լӳ�=3�������������⴦���Ҷ�˹�ȣ�
  //static const int NormalRacePtBonus;//�������pt�ӳ�
  static const double EventProb;//ÿ�غ���EventProb�������һ�������Լ�pt +EventStrengthDefault��ģ��֧Ԯ���¼�
  static const int EventStrengthDefault;

  //�籾�����
  static const int FriendCardIdSSR = 30241;//SSR��
  static const int FriendCardIdR = 19999;//��r��
  static const double FriendUnlockOutgoingProbEveryTurnLowFriendship;//ÿ�غϽ�������ĸ��ʣ��С��60
  static const double FriendUnlockOutgoingProbEveryTurnHighFriendship;//ÿ�غϽ�������ĸ��ʣ����ڵ���60
  //static const double FriendEventProb;//�����¼�����//����0.4д���ڶ�Ӧ��������
  static const double FriendVitalBonusSSR[5];//����SSR���Ļظ�������������1.6��
  static const double FriendVitalBonusR[5];//����R���Ļظ�������
  static const double FriendStatusBonusSSR[5];//����SSR�����¼�Ч������������1.25��
  static const double FriendStatusBonusR[5];//����R�����¼�Ч������
  

  //�籾���
  static const std::vector<int> LinkCharas;// Link��ɫ


  static const double LG_redLvXunlianCard[10];//��ǲ�ͬ�ȼ��Ŀ���ѵ���ӳ�
  static const double LG_redLvXunlianNPC[10];//��ǲ�ͬ�ȼ���npc��ѵ���ӳ�
  static const double LG_redLvYouqingNPC[10];//��ǲ�ͬ�ȼ���npc������ӳ�



  //����
  static const int FiveStatusFinalScore[1200+800*2+1];//��ͬ���Զ�Ӧ������
  static const double ScorePtRateDefault;//Ϊ�˷��㣬ֱ����Ϊÿ1pt��Ӧ���ٷ֡�
  static const double HintLevelPtRateDefault;//Ϊ�˷��㣬ֱ����Ϊÿһ��hint����pt��
  static const double HintProbTimeConstantDefault;//Ϊ�˷��㣬ֱ����Ϊÿһ��hint����pt��
  //static const double ScorePtRateQieZhe;//Ϊ�˷��㣬ֱ����Ϊÿ1pt��Ӧ���ٷ֡�����

  static bool isLinkChara(int id);

};