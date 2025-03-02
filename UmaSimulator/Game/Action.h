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
  TRA_outgoing, //�������޵ġ���Ϣ&�����
  TRA_race,
  TRA_none = -1, //��Action��ѵ����ֻ����
  TRA_redistributeCardsForTest = -2 //ʹ��������ʱ��˵��ҪrandomDistributeCards�����ڲ���ai��������Search::searchSingleActionThread��ʹ��
};


struct Action 
{
  static const std::string trainingName[8];
  static const std::string dishName[14];
  static const Action Action_RedistributeCardsForTest;
  static const int MAX_ACTION_TYPE = 21;//��׼��Action�б�ţ�8+13=21��
  static const int MAX_TWOSTAGE_ACTION_TYPE = 21 + 8 + 8;//���׶��������ǵ����Action������ֻ������1������Ҫ���Ƕ��׶�������8+13+2*8=37��


  
  int16_t dishType;//���ˣ�0Ϊ������

  int16_t train;//-1��ʱ��ѵ����01234���������ǣ�5�����6��Ϣ��7���� 
  //ע��������������������û������ͨ��������ṩѡ��
  bool isActionStandard() const;
  int toInt() const;
  std::string toString() const;
  static Action intToAction(int i);
};