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
  static const std::string Cook_MaterialNames[5];//����ԭ������

  static const std::vector<int> Cook_LinkCharas;// Link��ɫ
  //static const int Cook_DishPtBounds[7];//����pt�ķֵ�
  static int Cook_DishPtLevel(int dishPt);//����pt�ķֵ�
  static const int Cook_DishPtTrainingBonus[8];//����ptѵ���ӳ�
  static const int Cook_DishPtSkillPtBonus[8];//����pt���ܵ�ӳ�
  static const int Cook_DishPtDeyilvBonus[8];//����pt�����ʼӳ�
  static const int Cook_DishPtBigSuccessRate[8];//�����ɹ�����
  //��ɹ�ʱ���Ȱ��������ʵı���ѡ��һ��buff��������Чbuff�����ٶ�ÿ��buff��׷�Ӹ��ʾ����Ƿ�׷��
  static const int Cook_DishPtBigSuccessBuffProb[5][6];//�����ɹ���buff�������ʣ�[����ȼ�][buff����]������buff��������1������2���飬3�4����5��������
  static const int Cook_DishPtBigSuccessBuffExtraProb[5][6];//�����ɹ���buff׷�Ӹ��ʣ�[����ȼ�][buff����]������buff��������1������2���飬3�4����5��������

  static const double Cook_RestGreenRate;//��Ϣ��ɫ����
  static const double Cook_RaceGreenRate;//������ɫ����

  static const int Cook_DishLevel[14];//1������һ���������2�����ڶ����5����3�����������5����4�����������G1Plate
  static const int Cook_DishMainTraining[14];//�������ѵ����1����4��û��
  static const int Cook_DishGainPt[14];//�����õ�����pt
  static const int Cook_DishCost[14][5];//����ԭ������
  static const bool Cook_DishTrainingBonusEffective[14][5];//�������Щѵ���мӳ�
  static const int Cook_FarmLvCost[5];//ũ����������
  static const int Cook_HarvestBasic[6];//ũ��ȼ��ջ����ֵ
  static const int Cook_HarvestExtra[6];//ũ��ȼ��ջ�׷��ֵ
  static const int Cook_MaterialLimit[6];//��������


  //����
  static const int FiveStatusFinalScore[1200+800*2+1];//��ͬ���Զ�Ӧ������
  static const double ScorePtRateDefault;//Ϊ�˷��㣬ֱ����Ϊÿ1pt��Ӧ���ٷ֡�
  static const double HintLevelPtRateDefault;//Ϊ�˷��㣬ֱ����Ϊÿһ��hint����pt��
  //static const double ScorePtRateQieZhe;//Ϊ�˷��㣬ֱ����Ϊÿ1pt��Ӧ���ٷ֡�����

  static bool isLinkChara(int id);

};