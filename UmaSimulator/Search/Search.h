#pragma once
#include <vector>
#include "SearchParam.h"
#include "../Game/Game.h"
#include "../NeuralNet/Evaluator.h"
#include "../NeuralNet/TrainingSample.h"

struct SearchResult
{
  static const int NormDistributionSampling = 128;//ͳ�Ʒ����ֲ�ʱ��ÿ����̬�ֲ���ɶ��ٸ�����
  static double normDistributionCdfInv[NormDistributionSampling];//��̬�ֲ��ۻ��ֲ������ķ�������0~1�Ͼ���ȡ��
  static void initNormDistributionCdfTable();//��ʼ��

  //Action action;
  bool isLegal;
  int num;
  int32_t finalScoreDistribution[MAX_SCORE];//ĳ��action�����շ����ֲ�Ԥ��
  void clear();
  void addResult(ModelOutputValueV1 v); //O(N*NormDistributionSampling)
  ModelOutputValueV1 getWeightedMeanScore(double radicalFactor);//slow, O(MAXSCORE), avoid frequently call

  ModelOutputValueV1 lastCalculate;//�ϴε���getWeightedMeanScore�ļ�����
  bool upToDate;//lastCalculate�Ƿ��ѹ�ʱ
  double lastRadicalFactor;//�ϴμ����radicalFactor
};


//һ����Ϸ��һ��search
class Search
{
public:
  // ����ϣ��������շ����ķֲ�
  // ʹ��������������ؿ���ʱ�����depth<TOTAL_TURN��û���������վ֡���ʱ�������緵��Ԥ��ƽ��ֵmean�ͱ�׼��stdev
  // �������ؿ����������������ÿ��������Ϊ��̬�ֲ���������̬�ֲ���ȡNormDistributionSampling���㣬����finalScoreDistribution��
  // ֮���finalScoreDistribution���д������������ModelOutputValueV1�еĸ������
  
  
  //����ÿ��action����searchFactorStage[0]�����ļ�����
  //���ĳ��action�ķ��������߷ֵ���searchThreholdStdevStage����׼����ų������ѡ��
  //û���ų���action���еڶ�����������������searchFactorStage[1]
  //stage������̫�࣬��Ϊÿ��ÿ��action��Ҫ����getWeightedMeanScore()
  static const int expectedSearchStdev = 2200;
  static const int searchStageNum = 3;
  static const double searchFactorStage[searchStageNum];
  static const double searchThreholdStdevStage[searchStageNum];



  Game rootGame;//��ǰ�������������ĸ�����
  int threadNumInGame;//һ��search���漸���߳�
  int batchSize;
  SearchParam param;
  std::vector<Evaluator> evaluators;

  std::vector<SearchResult> allActionResults;//���п��ܵ�ѡ��Ĵ��

  //����ÿ�����ܵ������ÿ������ģ��eachSamplingNum�֣�ģ��maxDepth�غϺ󷵻�����������������������ƽ���֡���׼��ֹ۷֣�
  //���ڶ��̣߳��ݶ��ķ������£�
  // 1.Model����������Ѿ�����õ�float����*batchsize�����Ҳ������*batchsize��Model��������ͬʱֻ�ܼ���һ��
  // 2.����evaluateSingleAction����eachSamplingNum����Ϸ���threadNumInGame�飬ÿ��һ���߳�(Evaluator)��ÿ���̷ֳ߳�eachSamplingNum/(threadNumInGame*batchsize)С�飬ÿС��batchsize����Ϸ�����μ���ÿ��С��ķ��������������֮����������
  // 3.���Ҫ�ܺܶ�֣��������ף�����ͬʱ��threadGame�֣����߳���ΪthreadGame*threadNumInGame����eachSamplingNum��Сbatchsize�ϴ󣬿�����threadNumInGame=1
  // Ƕ�׽ṹ��Search(threadGame��)->Evaluator(threadGame*threadNumInGame��)->Model(1��)

  Search(Model* model, int batchSize, int threadNumInGame);
  Search(Model* model, int batchSize, int threadNumInGame, SearchParam param0);

  void setParam(SearchParam param0);

  Action runSearch(const Game& game,
    std::mt19937_64& rand, bool twoStageSearchFirstYear = true);//���ڵ�ǰ���棬����ÿ��ѡ��ķ�������������ѡ��, twoStageSearchFirstYear�ǵ�һ�������Բ�֮���Ƿ�������ѵ��

  void printSearchResult(bool showSearchNum);//��ӡ�������

  ModelOutputValueV1 evaluateNewGame(const Game& game,
    std::mt19937_64& rand);//ֱ�Ӵӵ�һ�غϿ�ʼ���ؿ��壬���ڲ��Կ���ǿ�Ȼ���aiǿ��

  //�Ե���action�������ؿ��壬��������ӵ�allActionResults��
  void searchSingleAction(
    int searchN,
    std::mt19937_64& rand,
    SearchResult& searchResult,
    Action action);

  //�����������ѡ�����ѡ��policyҲ��һ������
  //mode=0�Ǹ���ʤ�ʣ�=1�Ǹ���ƽ����
  //ModelOutputPolicyV1 extractPolicyFromSearchResults(int mode, float delta = 0);

  //�����ϴ�������������Ϊѵ������
  TrainingSample exportTrainingSample(double policyDelta = 100);//policyDelta��policy����ϵ��


private:

  std::vector<ModelOutputValueV1> NNresultBuf;

  int calculateBatchNumEachThread(int searchN) const;//ÿ���̶߳���batch
  int calculateRealSearchN(int searchN) const;//���߳���*batchsizeȡ����ļ�����
  //���㵥��action����ֵ�������̡߳���ÿ�ֵĽ�����浽resultBuf��Ȳ���allActionResults���
  void searchSingleActionThread(
    int threadIdx,
    ModelOutputValueV1* resultBuf, 
    int batchNum,

    std::mt19937_64& rand,
    Action action
  );

};