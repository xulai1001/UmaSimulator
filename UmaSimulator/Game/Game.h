#pragma once
#include <random>
#include <array>
#include "../GameDatabase/GameDatabase.h"
#include "Person.h"
#include "Action.h"

struct SearchParam;

struct Game;

enum LegendColorEnum :int16_t
{
  L_blue,//��
  L_green,//��
  L_red,//�죨�ۣ�
  L_unknown=-1,
};

//�籾���ĵá�
//��Ϸ��id�� 4λ��"XY0Z"��X����ɫ��Y��������Z�ǵڼ���
//�˴�-1�ǿգ�57��buff��0~56��ʾ
struct ScenarioBuffInfo
{
  int16_t buffId;
  bool isActive;
  int16_t coolTime;
  ScenarioBuffInfo();
  int16_t getBuffColor() const;
  int16_t getBuffStar() const;
  std::string getName() const;
  std::string getColoredState() const;

  static int16_t getBuffStarStatic(int16_t id);
  static std::string buffDescriptions[57];
  static std::string getScenarioBuffName(int16_t buffId);
  static bool defaultOrder(int16_t a, int16_t b);
};

//�籾�ӳ�   //��ֻ�������ѵ�����еĲ��֣�����ͷ�Ĳ���
//ÿ�μ���ѵ����ֵǰ�������¼������
struct ScenarioBonus
{
  float hintProb;//����������%
  int16_t hintLv;//���ȼ�����
  int16_t moreHint;//׷�Ӽ���hint
  bool alwaysHint;//һ������hint
  
  float vitalReduce;//�������ļ���%
  float jibanAdd1;//�������1�������hint��
  float jibanAdd2;//�������2��ֻ���������
  float deyilv;//����������%
  float disappearRateReduce;//��ʧ�ʼ��١�ԭ������ͨ����ʧ����50������100���ж�Ӧbuffʱ����25

  float xunlian;//ѵ���ӳ�%
  float ganjing;//�ɾ��ӳ�%
  float youqing;//����ӳ�%

  int16_t extraHead;//׷����ͷ�ĸ���

  float xunlianPerHead;//ÿ����ͷ�������ѵ���ӳ�
  float ganjingPerHead;//ÿ����ͷ������ٸɾ��ӳ�
  float youqingPerShiningHead;//ÿ��������ͷ�����������ӳ�
  ScenarioBonus();
  void clear();
};

struct ScenarioBuffCondition //����buff�ĸ�������
{
  bool isRest;
  bool isTraining;
  bool isYouqing;
  int16_t trainingSucceed;
  int16_t trainingHead;
  ScenarioBuffCondition();
  void clear();
};

struct GameSettings
{
  //��ʾ���
  bool playerPrint;//�������ʱ����ʾ������Ϣ

  //��������

  float ptScoreRate;//ÿpt���ٷ�
  float hintPtRate;//ÿһ��hint�ȼ۶���pt
  float hintProbTimeConstant;//�Ѿ���t������hintʱ����hint�����ܵĸ���=0.9*(1-exp(-exp(2-t/T)))������T=hintProbTimeConstant��Ĭ��80
  int16_t eventStrength;//ÿ�غ��У����⣩���ʼ���ô�����ԣ�ģ��֧Ԯ���¼�
  int16_t scoringMode;//���ַ�ʽ

  int16_t color_priority;//����ѡ��������ɫ��-1Ϊ������

  GameSettings();
};

enum scoringModeEnum :int16_t
{
  SM_normal,//��ͨ(���֡����۵�)ģʽ
  SM_race,//ͨ�ô���ģʽ
  SM_jjc,//������ģʽ
  SM_long,//������ģʽ
  SM_2400m,//2400mģʽ
  SM_2000m,//2000mģʽ
  SM_mile,//Ӣ��ģʽ
  SM_short,//�̾���ģʽ
  SM_debug //debugģʽ
};

enum personIdEnum :int16_t
{
  PS_none = -1,//δ����
  PS_noncardYayoi = 6,//�ǿ����³�
  PS_noncardReporter = 7,//�ǿ�����
  PS_npc = 8,//NPC�����ã�
  PS_npc0 = 10,//NPC��
  PS_npc1 = 11,//NPC��
  PS_npc2 = 12,//NPC��
  PS_npc3 = 13,//NPC��
  PS_npc4 = 14,//NPC��
};


//int16_t stage;//������ͷǰ1��������ͷ��c2��ѵ�������ſ��¼���ѡ���л���ѡһ��3����ȡ�ĵ�ǰ4������У���ѡ�ĵ�5������У����̶�������¼�ǰ6��6֮�����stage1
//����������stage2���Լ���policy��stage4/6���Լ���value
//stage2ʱ������ֱ�����ѡ���ѵ��
//stage3/5ʱ���о�����ѡ��ֱ����һ�����˹���û������ԣ���Ȼ��ֱ�������������value��ȡvalue��ߵ���Ϊѡ��
enum StageEnum :int16_t
{
  ST_none,
  ST_distribute,//������ͷǰ
  ST_train,//ѵ��ǰ
  ST_decideEvent,//ѡ���ſ��¼�ǰ
  ST_pickBuff,//�����ȡ�ĵ�ǰ
  ST_chooseBuff,//ѡ�ĵ�ǰ
  ST_event,//�����¼�ǰ
  ST_action_randomize,//������action����ʾST_train��ST_chooseBuffǰ�������
};

//int16_t decidingEvent;//��Ҫ����ĺ�ѡ������¼���ѡbuff 1���ſ����� 2���ſ���ѡһ 3
enum DecidingEventEnum :int16_t
{
  DecidingEvent_none,
  DecidingEvent_RESERVED,
  DecidingEvent_outing,//�ſ�����
  DecidingEvent_three,//�ſ���ѡһ
};

enum TrainActionTypeEnum :int16_t
{
  T_speed = 0,
  T_stamina,
  T_power,
  T_guts,
  T_wiz,
  T_rest,
  T_outgoing, //�������޵ġ���Ϣ&�����
  T_race, //�������ı���
  T_none = -1, //��Action��ѵ����ֻ����
  //TRA_redistributeCardsForTest = -2 //ʹ��������ʱ��˵��ҪrandomDistributeCards�����ڲ���ai��������Search::searchSingleActionThread��ʹ��
};


struct Action
{
  static const std::string trainingName[8];
  //static const Action Action_RedistributeCardsForTest;
  static const int MAX_ACTION_TYPE = 8;
  
  int16_t stage;//���action���������ĸ�stage�ģ������ST_distribute��ST_pickBuff��ST_event����������Ҫ�����κ�ѡ���stage����������������
  int16_t idx;//stage=ST_trainʱΪѵ����01234���������ǣ�5�����6��Ϣ��7������stage=ST_decideEvent��ST_chooseBuffʱ��ѡ�ڼ���
  Action();//��Action
  Action(int st);//ST_distribute��ST_pickBuff��ST_event����������Ҫ�����κ�ѡ���stage
  Action(int st, int idx);//��Ҫ��ѡ���stage

  //bool isActionStandard() const;
  //int toInt() const;
  std::string toString() const;
  std::string toString(const Game& game) const;
  //static Action intToAction(int i);
};

struct Game
{
  GameSettings gameSettings;//�û�����

  //����״̬����������ǰ�غϵ�ѵ����Ϣ
  int32_t umaId;//�����ţ���KnownUmas.cpp
  bool isLinkUma;//�Ƿ�Ϊlink��
  bool isRacingTurn[TOTAL_TURN];//��غ��Ƿ����
  int16_t fiveStatusBonus[5];//�������ά���Եĳɳ���

  int16_t turn;//�غ�������0��ʼ����77����
  int16_t vital;//������������vital������Ϊ��Ϸ��������е�
  int16_t maxVital;//��������
  int16_t motivation;//�ɾ�����1��5�ֱ��Ǿ����������õ��������õ���������

  int16_t fiveStatus[5];//��ά���ԣ�1200���ϲ�����
  int16_t fiveStatusLimit[5];//��ά�������ޣ�1200���ϲ�����
  int32_t skillPt;//���ܵ�
  int32_t skillScore;//�����ܵķ���
  int32_t hintSkillLvCount;//�Ѿ��ж��ټ�hint�ļ����ˡ�hintSkillLvCountԽ�࣬hint�����ܵĸ���ԽС�������Եĸ���Խ��
  int16_t trainLevelCount[5];//ѵ���ȼ�������ÿ��4�¼�һ��

  int16_t failureRateBias;//ʧ���ʸı�������ϰ����=-2����ϰ����=2
  bool isQieZhe;//���� 
  bool isAiJiao;//����
  bool isPositiveThinking;//�ݥ��ƥ���˼�������˵����γ���ѡ�ϵ�buff�����Է�һ�ε�����
  bool isRefreshMind;//+5 vital every turn

  bool haveCatchedDoll;//�Ƿ�ץ������

  int16_t zhongMaBlueCount[5];//����������Ӹ���������ֻ��3��
  int16_t zhongMaExtraBonus[6];//����ľ籾�����Լ����ܰ����ӣ���Ч��pt����ÿ�μ̳мӶ��١�ȫ��ʦ�����ӵ���ֵ��Լ��30��30��200pt

  int16_t stage;//int16_t stage;//������ͷǰ1��������ͷ��2��ѵ�������ſ��¼���ѡ���л���ѡһ��3����ȡ�ĵ�ǰ4������У���ѡ�ĵ�5������У����̶�������¼�ǰ6��6֮�����stage1
  int16_t decidingEvent;//��Ҫ����ĺ�ѡ������¼���ѡbuff 1���ſ����� 2���ſ���ѡһ 3
  bool isRacing;//����غ��Ƿ��ڱ���

  int16_t friendship_noncard_yayoi;//�ǿ����³��
  int16_t friendship_noncard_reporter;//�ǿ������

  Person persons[MAX_INFO_PERSON_NUM];//������6�ſ����ǿ����³������ߣ�NPC�ǲ���������person�࣬���һ��8
  int16_t personDistribution[5][5];//ÿ��ѵ������Щ��ͷid��personDistribution[�ĸ�ѵ��][�ڼ�����ͷ]����λ��Ϊ-1��0~5��6�ſ����ǿ����³�6������7��NPC�μ�personIdEnum
  //int lockedTrainingId;//�Ƿ���ѵ�����Լ��������ĸ�ѵ���������Ȳ��ӣ���ai��������ʱ���ټӡ�

  int16_t saihou;//����ӳ�


  //�籾���--------------------------------------------------------------------------------------
  
  int16_t lg_mainColor;//��ɫ
  int16_t lg_gauge[3];//������ɫ�ĸ���
  int16_t lg_trainingColor[8];//ѵ����gauge��ɫ
  //bool lg_trainingColorBoost[8];//ѵ����gauge�Ƿ�+3


  ScenarioBuffInfo lg_buffs[10];//10��buff������buffId=0
  bool lg_haveBuff[57];//����Щbuff����lg_buffs�ظ����Ǳ��ڲ���
  int16_t lg_pickedBuffsNum;//��ȡ���˼���buff
  int16_t lg_pickedBuffs[9];//��ȡ����buff��id


  int16_t lg_blue_active;//���ǵĳ����õ�
  int16_t lg_blue_remainCount;//�����õ���ʣ�����غ�
  int16_t lg_blue_currentStepCount;//��3�����������õ�
  int16_t lg_blue_canExtendCount;//�����ӳ�����

  int16_t lg_green_todo;//�̵�

  int16_t lg_red_friendsGauge[16];//��ǵ��������ź�personIdEnum��Ӧ
  int16_t lg_red_friendsLv[16];//��ǵĵȼ�������ź�personIdEnum��Ӧ



  //��������籾���˿�����Ϊ�ӽ��ش������������Ŷӿ����Ժ��ٿ���
  int16_t friend_type;//0û�����˿���1 ssr����2 r��
  int16_t friend_personId;//���˿���persons��ı��
  double friend_vitalBonus;//���˿��Ļظ�������
  double friend_statusBonus;//���˿����¼�Ч������

  int16_t friend_stage;//0��δ�����1���ѵ����δ�������У�2���ѽ�������
  bool friend_outgoingUsed[5];//���˵ĳ����ļ����߹���   ��ʱ���������������Ŷӿ��ĳ���
  bool friend_qingre;//�ſ��Ƿ�����
  int16_t friend_qingreTurn;//�ſ��������ȶ��ٻغ���





  //����ͨ���������Ϣ�����õķǶ�������Ϣ��ÿ�غϸ���һ�Σ�����Ҫ¼��
  ScenarioBonus lg_bonus;
  int16_t trainValue[5][6];//ѵ����ֵ���������²�+�ϲ㣩����һ�����ǵڼ���ѵ�����ڶ���������������������pt
  int16_t trainVitalChange[5];//ѵ����������仯�������������ģ�
  int16_t failRate[5];//ѵ��ʧ����
  int16_t trainHeadNum[5];//ѵ����ͷ���������������³�����
  int16_t trainShiningNum[5];//ѵ�����ʸ���


  //ѵ����ֵ������м������������������д�߼����й���
  int16_t trainValueLower[5][6];//ѵ����ֵ���²㣬��һ�����ǵڼ���ѵ�����ڶ���������������������pt����
  //double trainValueCardMultiplier[5];//֧Ԯ������=(1+��ѵ���ӳ�)(1+�ɾ�ϵ��*(1+�ܸɾ��ӳ�))(1+0.05*�ܿ���)(1+����1)(1+����2)...

  //bool cardEffectCalculated;//֧Ԯ��Ч���Ƿ��Ѿ�����������޹ز˲���Ҫ���¼��㣬���俨����߶�jsonʱ��Ҫ��Ϊfalse
  //CardTrainingEffect cardEffects[6];

  ScenarioBuffCondition lg_buffCondition;



  //��Ϸ�������------------------------------------------------------------------------------------------

public:

  void newGame(std::mt19937_64& rand,
    GameSettings settings,
    int newUmaId,
    int umaStars,
    int newCards[6],
    int newZhongMaBlueCount[5],
    int newZhongMaExtraBonus[6]);//������Ϸ�����֡�umaId��������


  //��������Ƿ������Һ���
  bool isLegal(Action action) const;


  std::vector<Action> getAllLegalActions() const;
  //����ʲôstage�������½���һ������action���Ϸ��򷵻�false
  void applyAction(
    std::mt19937_64& rand,
    Action action); 

  void continueUntilNextDecision(  //��������Ҫ���ѡ���stage��ֱ���´���Ҫѡ��
    std::mt19937_64& rand);

  void applyActionUntilNextDecision(
      std::mt19937_64& rand,
      Action action);




  int finalScore() const;//�����ܷ�
  int finalScore_rank() const;//���۵�
  int finalScore_sum() const;//����������ƣ�����֮��*4+���ܷ�
  int finalScore_mile() const;//�������֡�Ӣ��
  bool isEnd() const;//�Ƿ��Ѿ��վ�




  //ԭ�����⼸��private���У����private��ĳЩ�ط��ǳ��������Ǿ͸ĳ�public
  void calculateScenarioBonus();//����籾buff�ĸ��ּӳ�
  void randomizeTurn(std::mt19937_64& rand);//�غϳ�������������������ͷ�������ɫ�ȣ�ST_distribute->ST_train
  void undoRandomize();//׼�����·��䣬ST_train->ST_distribute��ST_chooseBuff->ST_pickBuff
  void randomDistributeHeads(std::mt19937_64& rand);//���������ͷ
  void randomInviteHeads(std::mt19937_64& rand, int num);//���ҡnum����ͷ
  void inviteOneHead(std::mt19937_64& rand, int idx);//ҡpersonId=idx�����ͷ
  void calculateTrainingValue();//��������ѵ���ֱ�Ӷ��٣�������ʧ���ʡ�ѵ���ȼ�������
  bool applyTraining(std::mt19937_64& rand, int16_t train);//ST_train->ST_decideEvent/ST_pickBuff/ST_event������ ѵ��/����/���� �������������˵���¼�����������buff���������̶��¼��;籾�¼���������Ϸ����򷵻�false���ұ�֤�����κ��޸�
  void updateScenarioBuffAfterTrain(int16_t trainIdx, bool trainSucceed);//���¸����ĵõĴ�������
  void maybeSkipPickBuffStage();//ѵ�����������Ƿ�Ӧ������ѡbuff�׶Ρ�ÿ���غϱ�����ã��������ѡbuff�غϻ�ֱ���޸�stage
  void decideEvent(std::mt19937_64& rand, int16_t idx);//�ſ���ѡһ/����ѡ��
  void decideEvent_outing(std::mt19937_64& rand, int16_t idx);//�ſ�����ѡ��
  void decideEvent_three(std::mt19937_64& rand, int16_t idx);//�ſ���ѡһ
  void randomPickBuff(std::mt19937_64& rand);//ST_pickBuff->ST_chooseBuff����buff(�ĵ�)���������ȡbuff
  int pickSingleBuff(std::mt19937_64& rand, int16_t color, int16_t star);//���������ȡcolor��ɫstar�������ĵã����ȫ�������򷵻�-1
  void chooseBuff(int16_t idx); //ST_chooseBuff->ST_event��ѡ��ڼ���buff
  
  void checkEvent(std::mt19937_64& rand);//ST_chooseBuff->ST_distribute���̶��¼�������¼�����������һ���غ�
  void checkFixedEvents(std::mt19937_64& rand);//ÿ�غϵĹ̶��¼��������籾�¼��͹̶������Ͳ��������¼���
  void checkRandomEvents(std::mt19937_64& rand);//ģ��֧Ԯ���¼�����������¼������������������飬������ȣ�

  //���ýӿ�-----------------------------------------------------------------------------------------------

  bool loadGameFromJson(std::string jsonStr);

  //����������
  void getNNInputV1(float* buf, const SearchParam& param) const;

  void print() const;//�ò�ɫ������ʾ��Ϸ����
  void printFinalStats() const;//��ʾ���ս��




  //���ָ���������ӿڣ����Ը�����Ҫ���ӻ���ɾ��-------------------------------------------------------------------------------

  inline bool isXiahesu() const //�Ƿ�Ϊ�ĺ���
  {
    return (turn >= 36 && turn <= 39) || (turn >= 60 && turn <= 63);
  }
  inline bool isRaceAvailable() const //�Ƿ���Զ������
  {
    return turn >= 13 && turn <= 71;
  }

  int calculateRealStatusGain(int value, int gain) const;//����1200����Ϊ2�ı�����ʵ����������ֵ
  void addStatus(int idx, int value);//��������ֵ�����������
  void addAllStatus(int value);//ͬʱ�����������ֵ
  void addVital(int value);//���ӻ�������������������
  void addVitalMax(int value);//�����������ޣ�����120
  void addMotivation(int value);//���ӻ�������飬ͬʱ���ǡ�isPositiveThinking������
  void addJiBan(int idx,int value,int type);//����������ǰ�����buff��Ҳ���Ǻ�ǳ�硣type0�ǵ����type1��hint��type2�ǲ����κμӳɵ�
  void addStatusFriend(int idx, int value);//���˿��¼�����������ֵ����pt��idx=5���������¼��ӳ�
  void addVitalFriend(int value);//���˿��¼����������������ǻظ����ӳ�
  void runRace(int basicFiveStatusBonus, int basicPtBonus);//�ѱ��������ӵ����Ժ�pt�ϣ������ǲ�������ӳɵĻ���ֵ
  void addTrainingLevelCount(int trainIdx, int n);//Ϊĳ��ѵ������n�μ���
  void applyNormalTraining(std::mt19937_64& rand, int16_t train, bool success);//��������ѵ��
  void addHintWithoutJiban(std::mt19937_64& rand, int idx);
  void jicheng(std::mt19937_64& rand);//�ڶ�����ļ̳�

  int getTrainingLevel(int trainIdx) const;//����ѵ���ȼ�
  int calculateFailureRate(int trainType, double failRateMultiply) const;//����ѵ��ʧ���ʣ�failRateMultiply��ѵ��ʧ���ʳ���=(1-֧Ԯ��1��ʧ�����½�)*(1-֧Ԯ��2��ʧ�����½�)*...

  bool isCardShining(int personIdx, int trainIdx) const;    // �ж�ָ�����Ƿ����ʡ���ͨ�����������ѵ�����Ŷӿ���friendOrGroupCardStage
  //bool trainShiningCount(int trainIdx) const;    // ָ��ѵ����Ȧ�� //uaf��һ������
  void calculateTrainingValueSingle(int tra);//����ÿ��ѵ���Ӷ���   //uaf�籾�������ѵ��һ����ȽϷ���

  //�籾���
  void addScenarioBuffBonus(int idx);//��Ӿ籾�ĵüӳɵ�lg_bonus�������жϲ���buff����Ч�������ɾ����õ��ȣ�����ѵ���ɹ���֮����ж���������
  void updateScenarioBuffCondition(int idx);//���¸����ĵõĴ�������
  void addLgGauge(int16_t color, int num);//��color��num��ȥ������8�������
  void setMainColorTurn36(std::mt19937_64& rand);//36�غ�ʱȷ����ɫ��color_priority��Ϊ��ʱǿ��ָ�������ɫ�������ԭ��ɫ��ָ����ɫ��ͬ���3000��


  //���˿�����¼�
  void handleFriendUnlock(std::mt19937_64& rand);//�����������
  void handleOutgoing(std::mt19937_64& rand);//���
  void handleFriendClickEvent(std::mt19937_64& rand, int atTrain);//���˵���¼�
  void handleFriendFixedEvent();//���˹̶��¼�������+����

  void runNormalOutgoing(std::mt19937_64& rand);//�������
  void runFriendOutgoing(std::mt19937_64& rand, int idx, int subIdx);//�������
  void runFriendClickEvent(std::mt19937_64& rand, int idx);//���˵���¼�
  

  //���
  float getSkillScore() const;//���ܷ֣�����������֮ǰҲ������ǰ��ȥ


  //��ʾ
  void printEvents(std::string s) const;//����ɫ������ʾ�¼�
  std::string getPersonStrColored(int personId, int atTrain) const;//���������������ϳɴ���ɫ���ַ�������С�ڰ�������ʾ
};

