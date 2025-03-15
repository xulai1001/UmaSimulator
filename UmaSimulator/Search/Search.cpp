#include <random>
#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <future>
#include <iostream>
#include "Search.h"
#include "../GameDatabase/GameConfig.h"
#include "../External/mathFunctions.h"
using namespace std;

const ModelOutputValueV1 ModelOutputValueV1::illegalValue = { 1e-5,0,1e-5 };

const double Search::searchFactorStage[searchStageNum] = { 0.25,0.25,0.5 };
const double Search::searchThreholdStdevStage[searchStageNum] = { 4,4,0 };//4����׼��Ƚϱ���

double SearchResult::normDistributionCdfInv[NormDistributionSampling];

static void softmax(float* f, int n)
{
  float max = -1e30;
  for (int i = 0; i < n; i++)
    if (f[i] > max)max = f[i];

  float total = 0;
  for (int i = 0; i < n; i++)
  {
    f[i] = exp(f[i] - max);
    total += f[i];
  }

  float totalInv = 1 / total;
  for (int i = 0; i < n; i++)
    f[i] *= totalInv;
}

//���ݻغ�������������
static double adjustRadicalFactor(double maxRf, int turn)
{
  //�����ȡ�ļ�����
  double remainTurns = TOTAL_TURN - turn;
  double factor = pow(remainTurns / TOTAL_TURN, 0.5);
  return factor * maxRf;
}

MonteCarloSingle::MonteCarloSingle():rootGame(), stopAtTurn(-1), finished(false)
{
}

void MonteCarloSingle::setGame(Game g, int st)
{
  rootGame = g;
  stopAtTurn = st;
  finished = false;
}

void MonteCarloSingle::step(std::mt19937_64& rand)
{
#if USE_BACKEND == BACKEND_NONE
  while (true)
  {
    Action a = Evaluator::handWrittenStrategy(rootGame);
    rootGame.applyActionUntilNextDecision(rand, a);
    if (rootGame.isEnd())break;
  }
  finished = true;
#else
  TODO
#endif
  
}

ModelOutputValueV1 MonteCarloSingle::getFinalValue() const
{
  assert(finished);
#if USE_BACKEND == BACKEND_NONE
  assert(rootGame.isEnd());
  ModelOutputValueV1 v;
  v.value = rootGame.finalScore();
  v.scoreMean = v.value;
  v.scoreStdev = 0;
  return v; 
#else
    TODO
#endif
}

Search::Search(Model* model, int batchSize, int threadNumInGame):threadNumInGame(threadNumInGame), batchSize(batchSize)
{
  evaluators.resize(threadNumInGame);
  for (int i = 0; i < threadNumInGame; i++)
    evaluators[i] = Evaluator(model, batchSize);

  allActionResults.resize(Action::MAX_ACTION_TYPE);
  for (int i = 0; i < Action::MAX_ACTION_TYPE; i++)
    allActionResults[i].clear();

  param.searchSingleMax = 0;
}

Search::Search(Model* model, int batchSize, int threadNumInGame, SearchParam param0) :Search(model, batchSize, threadNumInGame)
{
  setParam(param0);
}
void Search::setParam(SearchParam param0)
{
  param = param0;

  //��searchGroupSize����batch
  param.searchGroupSize = calculateRealSearchN(param.searchGroupSize);
  param.searchSingleMax = calculateRealSearchN(param.searchSingleMax);

  //��param.samplingNum����batch
  //int batchEveryThread = (param.samplingNum - 1) / (threadNumInGame * batchSize) + 1;//�൱������ȡ��
  //if (batchEveryThread <= 0)batchEveryThread = 1;
  //int samplingNumEveryThread = batchSize * batchEveryThread;
  //param.samplingNum = threadNumInGame * samplingNumEveryThread;
  //NNresultBuf.resize(param.samplingNum);
}



Action Search::runSearch(const Game& game,
  std::mt19937_64& rand, bool twoStageSearchFirstYear)
{

  throw("TODO");
  /*
  assert(param.searchSingleMax > 0 && "Search.param not initialized");

  rootGame = game;
  rootGame.playerPrint = false;
  double radicalFactor = adjustRadicalFactor(param.maxRadicalFactor, rootGame.turn);

  bool needTwoStageSearch =
    twoStageSearchFirstYear &&
    game.turn < 24 &&
    !game.isRacing &&
    game.cook_dish == DISH_none &&
    (game.isDishLegal(DISH_curry) || game.isDishLegal(DISH_sandwich));

  int maxActionType = needTwoStageSearch ? Action::MAX_TWOSTAGE_ACTION_TYPE : Action::MAX_ACTION_TYPE;

  //bool shouldContinueSearch[Action::MAX_ACTION_TYPE];
  for (int actionInt = 0; actionInt < maxActionType; actionInt++)
  {
    Action action = intToTwoStageAction(actionInt);

    allActionResults[actionInt].clear();
    bool islegal = false;
    if (needTwoStageSearch)
    {
      //���׶���������������
      if (action.train == TRA_none)islegal = false;
      else if (action.dishType == DISH_none)islegal = rootGame.isLegal(action);
      else
        islegal = rootGame.isLegal(Action(action.dishType, TRA_none)) && rootGame.isLegal(Action(DISH_none, action.train));
    }
    else
      islegal = rootGame.isLegal(action);

    allActionResults[actionInt].isLegal = islegal;
    //shouldContinueSearch[actionInt] = allActionResults[actionInt].isLegal;
  }

  assert(param.searchGroupSize == calculateRealSearchN(param.searchGroupSize));//setParamӦ�ô������
  int totalSearchN = 0;//��Ŀǰһ�����˶���

  //ÿ��action����һ��
  for (int actionInt = 0; actionInt < maxActionType; actionInt++)
  {
    if (!allActionResults[actionInt].isLegal)continue;
    Action action = intToTwoStageAction(actionInt);
    //cout << action.dishType << action.train << endl;
    searchSingleAction(param.searchGroupSize, rand, allActionResults[actionInt], action);
    totalSearchN += param.searchGroupSize;
  }

  //ÿ�η���searchGroupSize�ļ�������searchValue�����Ǹ�action��ֱ���ﵽsearchSingleMax��searchTotalMax��ֹ����
  while (true)
  {
    if (param.searchGroupSize >= param.searchSingleMax)//ǰ��������һ��group�Ѿ��ﵽԤ��Ŀ��������������������
      break;

    double bestSearchValue = -1e4;
    int bestActionIntToSearch = -1;

    for (int actionInt = 0; actionInt < maxActionType; actionInt++)
    {
      if (!allActionResults[actionInt].isLegal)continue;
      double value = allActionResults[actionInt].getWeightedMeanScore(radicalFactor).value;
      double n = allActionResults[actionInt].num;
      assert(n > 0);
      double tn = double(totalSearchN);
      double policy = 1.0;//�����������1���У��������������������
      double searchValue = value + param.searchCpuct * policy * Search::expectedSearchStdev * sqrt(tn) / n;//��������ai�Ĺ�ʽ��ƽ����Խ�߻������Խ�٣�searchValueԽ��
      if (searchValue > bestSearchValue)
      {
        bestSearchValue = searchValue;
        bestActionIntToSearch = actionInt;
      }
    }

    assert(bestActionIntToSearch >= 0);

    Action action = intToTwoStageAction(bestActionIntToSearch);
    searchSingleAction(param.searchGroupSize, rand, allActionResults[bestActionIntToSearch], action);
    totalSearchN += param.searchGroupSize;

    if (allActionResults[bestActionIntToSearch].num >= param.searchSingleMax)
      break;
    if (param.searchTotalMax > 0 && totalSearchN >= param.searchTotalMax)
      break;
  }

  //������ϣ�����߷ֵ�ѡ��

  //�Ѷ��׶��������ϵ���һ���׶���
  if (needTwoStageSearch)
    integrateTwoStageResults();

  double bestValue = -1e4;
  int bestActionInt = -1;

  for (int actionInt = 0; actionInt < Action::MAX_ACTION_TYPE; actionInt++)
  {
    if (!allActionResults[actionInt].isLegal)continue;

    ModelOutputValueV1 value = allActionResults[actionInt].getWeightedMeanScore(radicalFactor);
    if (value.value > bestValue)
    {
      bestValue = value.value;
      bestActionInt = actionInt;
    }

  }

  Action bestAction = Action::intToAction(bestActionInt);
  assert(rootGame.isLegal(bestAction));
  return bestAction;
  */
}

void Search::printSearchResult(bool showSearchNum)
{
  

  throw("TODO");
  /*
  for (int actionInt = 0; actionInt < Action::MAX_ACTION_TYPE; actionInt++)
  {
    Action action = Action::intToAction(actionInt);
    SearchResult& res = allActionResults[actionInt];
    if (!res.isLegal)continue;
    ModelOutputValueV1 value = res.getWeightedMeanScore(param.maxRadicalFactor);
    cout << action.toString() << ":" << int(value.value);
    if (showSearchNum)
      cout << ", searchNum=" << res.num ;
    cout << endl;
  }*/
}

ModelOutputValueV1 Search::evaluateNewGame(const Game& game, std::mt19937_64& rand)
{
  return evaluateAction(game, Action(ST_action_randomize), rand);
}

ModelOutputValueV1 Search::evaluateAction(const Game& game, Action action, std::mt19937_64& rand)
{
  rootGame = game;

  double radicalFactor = adjustRadicalFactor(param.maxRadicalFactor, game.turn);
  allActionResults[0].clear();
  allActionResults[0].isLegal = true;
  searchSingleAction(param.searchSingleMax, rand, allActionResults[0], action);
  return allActionResults[0].getWeightedMeanScore(adjustRadicalFactor(radicalFactor, game.turn));
}


void Search::searchSingleAction(
  int searchN,
  std::mt19937_64& rand,
  SearchResult& searchResult,
  Action action)
{
  
  int batchNumEachThread = calculateBatchNumEachThread(searchN);
  searchN = calculateRealSearchN(searchN);
  if (NNresultBuf.size() < searchN) NNresultBuf.resize(searchN);

  int samplingNumEveryThread = batchNumEachThread * batchSize;

  if (threadNumInGame > 1)
  {
    std::vector<std::mt19937_64> rands;
    for (int i = 0; i < threadNumInGame; i++)
      rands.push_back(std::mt19937_64(rand()));

    std::vector<std::thread> threads;
    for (int i = 0; i < threadNumInGame; ++i) {
      threads.push_back(std::thread(

        [this, i, batchNumEachThread, samplingNumEveryThread, &rands, action]() {
          searchSingleActionThread(
            i,
            NNresultBuf.data() + samplingNumEveryThread * i,
            batchNumEachThread,
            rands[i],
            action
          );
        })
      );


    }
    for (auto& thread : threads) {
      thread.join();
    }
  }
  else //single thread for debug/speedtest
  {
    searchSingleActionThread(
      0,
      NNresultBuf.data(),
      batchNumEachThread,
      rand,
      action
    );
  }


  for (int i = 0; i < searchN; i++)
  {
    searchResult.addResult(NNresultBuf[i]);
  }
  
}

void Search::searchSingleActionThread(
  int threadIdx,
  ModelOutputValueV1* resultBuf,
  int batchNum,

  std::mt19937_64& rand,
  Action action
)
{
  vector<MonteCarloSingle> mcThreads;
  mcThreads.resize(batchSize);
  for (int batch = 0; batch < batchNum; batch++)
  {
    for (int g = 0; g < batchSize; g++)
    {
      mcThreads[g].setGame(rootGame, -1);
      mcThreads[g].rootGame.applyActionUntilNextDecision(rand, action);
    }

    while (true)
    {
      bool anyUnfinished = false;
      for (int g = 0; g < batchSize; g++)
      {
        if (!mcThreads[g].finished)
        {
          mcThreads[g].step(rand);
          anyUnfinished = true;
        }
      }
      if (!anyUnfinished)break;
    }

    for (int i = 0; i < batchSize; i++)
    {
      resultBuf[batch * batchSize + i] = mcThreads[i].getFinalValue();
    }
  }


  //throw("TODO");
  /*
  Evaluator& eva = evaluators[threadIdx];
  assert(eva.maxBatchsize == batchSize);
  bool isTwoStageAction = (action.dishType != DISH_none && action.train != TRA_none);
  bool isNewGame = action.train == TRA_redistributeCardsForTest;

  for (int batch = 0; batch < batchNum; batch++)
  {
    eva.gameInput.assign(batchSize, rootGame);

    //���ߵ�һ��
    for (int i = 0; i < batchSize; i++)
    {
      if (isNewGame)//������Ϸ
        eva.gameInput[i].randomDistributeCards(rand);
      else if(!isTwoStageAction)//����
        eva.gameInput[i].applyAction(rand, action);
      else//���׶�Action
      {
        Action action1 = { action.dishType,TRA_none };
        Action action2 = { DISH_none,action.train };
        eva.gameInput[i].applyAction(rand, action1);
        eva.gameInput[i].applyAction(rand, action2);
      }
    }
    int maxdepth = isNewGame ? param.maxDepth + 1 : param.maxDepth;
    for (int depth = 0; depth < param.maxDepth; depth++)
    {
      eva.evaluateSelf(1, param);//����policy
      //bool distributeCards = (depth != maxDepth - 1);//���һ��Ͳ����俨���ˣ�ֱ�ӵ����������ֵ


      bool allFinished = true;
      for (int i = 0; i < batchSize; i++)
      {
        if(!eva.gameInput[i].isEnd())
          eva.gameInput[i].applyAction(rand, eva.actionResults[i]);
        //Search::runOneTurnUsingPolicy(rand, gamesBuf[i], evaluators->policyResults[i], distributeCards);
        if (!eva.gameInput[i].isEnd())allFinished = false;
      }
      if (allFinished)break;
    }
    eva.evaluateSelf(0, param);//����value
    for (int i = 0; i < batchSize; i++)
    {
      resultBuf[batch * batchSize + i] = eva.valueResults[i];
    }

  }
  */
}


void SearchResult::initNormDistributionCdfTable()
{
  //��̬�ֲ��ۻ��ֲ������ķ�������0~1�Ͼ���ȡ��
  for (int i = 0; i < NormDistributionSampling; i++)
  {
    double x = (i + 0.5) / NormDistributionSampling;
    normDistributionCdfInv[i] = normalCDFInverse(x);
  }
}

void SearchResult::clear()
{
  isLegal = false;
  num = 0;
  for (int i = 0; i < MAX_SCORE; i++)
    finalScoreDistribution[i] = 0;
  upToDate = true;
  lastCalculate = ModelOutputValueV1::illegalValue;
}

void SearchResult::addResult(ModelOutputValueV1 v)
{
  upToDate = false;
  num += 1;
  for (int i = 0; i < NormDistributionSampling; i++)
  {
    int y = int(v.scoreMean + v.scoreStdev * normDistributionCdfInv[i] + 0.5);
    if (y < 0)y = 0;
    if (y >= MAX_SCORE)y = MAX_SCORE - 1;
    finalScoreDistribution[y] += 1;
  }
}

ModelOutputValueV1 SearchResult::getWeightedMeanScore(double radicalFactor) 
{
  if (upToDate && lastRadicalFactor == radicalFactor)
    return lastCalculate;
  if (!isLegal)
  {
    lastCalculate = ModelOutputValueV1::illegalValue;
    return ModelOutputValueV1::illegalValue;
  }
  double N = 0;//��������
  double scoreTotal = 0;//score�ĺ�
  double scoreSqrTotal = 0;//score��ƽ����
  //double winNum = 0;//score>=target�Ĵ�������

  double valueWeightTotal = 0;//sum(n^p*x[n]),x[n] from small to big
  double valueTotal = 0;//sum(n^p)
  double totalNinv = 1.0 / (num * NormDistributionSampling);
  for (int s = 0; s < MAX_SCORE; s++)
  {
    double n = finalScoreDistribution[s]; //��ǰ�����Ĵ���
    double r = (N + 0.5 * n) * totalNinv; //��ǰ��������������
    N += n;
    scoreTotal += n * s;
    scoreSqrTotal += n * s * s;

    //��������Ȩƽ��
    double w = pow(r, radicalFactor);
    valueWeightTotal += w * n;
    valueTotal += w * n * s;
  }

  ModelOutputValueV1 v;
  v.scoreMean = scoreTotal / N;
  v.scoreStdev = sqrt(scoreSqrTotal * N - scoreTotal * scoreTotal) / N;
  v.value = valueTotal / valueWeightTotal;
  upToDate = true;
  lastRadicalFactor = radicalFactor;
  lastCalculate = v;
  return v;
}

int Search::calculateBatchNumEachThread(int searchN) const
{
  int batchEveryThread = (searchN - 1) / (threadNumInGame * batchSize) + 1;//�൱������ȡ��
  if (batchEveryThread <= 0)batchEveryThread = 1;
  return batchEveryThread;
}

int Search::calculateRealSearchN(int searchN) const
{
  return calculateBatchNumEachThread(searchN) * threadNumInGame * batchSize;
}

