锘�#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <cassert>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>
#include "../Game/Game.h"
#include "../NeuralNet/Evaluator.h"
#include "../Search/Search.h"
#include "../External/termcolor.hpp"

#include "../GameDatabase/GameDatabase.h"
#include "../GameDatabase/GameConfig.h"
#include "../Tests/TestConfig.h"

using namespace std;

namespace TestScoreSearch
{
#if USE_BACKEND == BACKEND_LIBTORCH
  const string modelpath = "../training/example/model_traced.pt";
#elif USE_BACKEND == BACKEND_NONE
  const string modelpath = "";
#else
  const string modelpath = "../training/example/model.txt";
#endif

  const int threadNum = 1;
  const int batchsize = 1;
  const int threadNumInner = 8;
  const double radicalFactor = 3;//婵�杩涘害
  const int searchDepth = 2 * TOTAL_TURN;
  const int searchN = 2048;
  const bool recordGame = true;
  const bool debugPrint = false;

  int totalGames = 100000;
  int gamesEveryThread = totalGames / threadNum;

  TestConfig test;
  /*
  //int umaId = 108401;//璋锋按锛�30鍔涘姞鎴�
  int umaId = 106501;//澶槼绁烇紝15閫�15鍔涘姞鎴�
  int umaStars = 5;
  //int cards[6] = { 301604,301344,301614,300194,300114,301074 };//鍙嬩汉锛岄珮宄帮紝绁為拱锛屼箤鎷夋媺锛岄绁烇紝鍙告満
  int cards[6] = { 301604,301724,301614,301304,300114,300374 };//鍙嬩汉锛屾櫤楹︽槅锛岄�熺楣帮紝鏍瑰嚡鏂紝鏍归绁烇紝鏍圭殗甯�

  int zhongmaBlue[5] = { 18,0,0,0,0 };
  int zhongmaBonus[6] = { 10,10,30,0,10,70 };
  bool allowedDebuffs[9] = { false, false, false, false, false, false, true, false, false };//绗簩骞村彲浠ヤ笉娑堢鍑犱釜debuff銆傜浜斾釜鏄櫤鍔涳紝绗竷涓槸寮哄績鑴�
  */
  std::atomic<double> totalScore = 0;
  std::atomic<double> totalScoreSqr = 0;

  std::atomic<int> bestScore = 0;
  std::atomic<int> n = 0;
  std::mutex printLock;
  vector<atomic<int>> segmentStats = vector<atomic<int>>(700);//100鍒嗕竴娈碉紝700娈�
  std::atomic<int> printThreshold = 2187;

  void worker()
  {
    random_device rd;
    auto rand = mt19937_64(rd());

    Model* modelptr = NULL;
    Model model(modelpath, batchsize);
    if (modelpath != "")
    {
      modelptr = &model;
    }

    SearchParam searchParam(searchN, radicalFactor);
    searchParam.maxDepth = searchDepth;
    //searchParam.searchCpuct = 10.0;
    Search search(modelptr, batchsize, threadNumInner, searchParam);

    vector<Game> gameHistory;

    //if (recordGame)
    //  gameHistory.resize(TOTAL_TURN);

    for (int gamenum = 0; gamenum < gamesEveryThread; gamenum++)
    {
      Game game;
      game.newGame(rand, GameSettings(), test.umaId, test.umaStars, &test.cards[0], &test.zhongmaBlue[0], &test.zhongmaBonus[0]);


      while (!game.isEnd())
      {
        if (recordGame)
          gameHistory.push_back(game);
        Action action;
        action = search.runSearch(game, rand);
        if (debugPrint)
        {
          game.print();
          search.printSearchResult(true);
          cout<<action.toString()<<endl;
        }
        game.applyAction(rand, action);
      }
      //cout << termcolor::red << "鑲叉垚缁撴潫锛�" << termcolor::reset << endl;
      int64_t score = game.finalScore();
      if (score >= 57000)
      {
        if (recordGame)
          for (int i = 0; i < gameHistory.size(); i++)
            gameHistory[i].print();
        game.printFinalStats();
      }
      n += 1;
      totalScore += score;
      totalScoreSqr += score * score;
      for (int i = 0; i < 700; i++)
      {
        int refScore = i * 100;
        if (score >= refScore)
        {
          segmentStats[i] += 1;
        }
      }

      int bestScoreOld = bestScore;
      if (score > bestScore + printThreshold)
      {
        if (printThreshold < 100)
        {
          std::lock_guard<std::mutex> lock(printLock);
          game.printFinalStats();
          //cout << printThreshold << endl;
          cout.flush();
        }
        printThreshold = printThreshold / 3;
      }

      while (score > bestScoreOld && !bestScore.compare_exchange_weak(bestScoreOld, score)) {
        // 濡傛灉val澶т簬old_max锛屽苟涓攎ax_val鐨勫�艰繕鏄痮ld_max锛岄偅涔堝氨灏唌ax_val鐨勫�兼洿鏂颁负val
        // 濡傛灉max_val鐨勫�煎凡缁忚鍏朵粬绾跨▼鏇存柊锛岄偅涔堝氨涓嶅仛浠讳綍浜嬫儏锛屽苟涓攐ld_max浼氳璁剧疆涓簃ax_val鐨勬柊鍊�
        // 鐒跺悗鎴戜滑鍐嶆杩涜姣旇緝鍜屼氦鎹㈡搷浣滐紝鐩村埌鎴愬姛涓烘
      }

      //game.print();
      game.printFinalStats();
      cout << endl << n << "灞�锛屾悳绱㈤噺=" << searchN << "锛屽钩鍧囧垎" << totalScore / n << "锛屾爣鍑嗗樊" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "锛屾渶楂樺垎" << bestScore << endl;
      //for (int i=0; i<400; ++i)
      //    cout << i*100 << ",";
      //cout << endl;
      //for (int i=0; i<400; ++i)
      //    cout << float(segmentStats[i]) / n << ",";
      //cout << endl;
      cout

        << "UE7姒傜巼=" << float(segmentStats[327]) / n << ","
        << "UE8姒傜巼=" << float(segmentStats[332]) / n << ","
        << "UE9姒傜巼=" << float(segmentStats[338]) / n << ","
        << "UD0姒傜巼=" << float(segmentStats[344]) / n << ","
        << "UD1姒傜巼=" << float(segmentStats[350]) / n << ","
        << "UD2姒傜巼=" << float(segmentStats[356]) / n << ","
        << "UD3姒傜巼=" << float(segmentStats[362]) / n << ","
        << "UD4姒傜巼=" << float(segmentStats[368]) / n << ","
        << "UD5姒傜巼=" << float(segmentStats[375]) / n << ","
        << "UD6姒傜巼=" << float(segmentStats[381]) / n << ","
        << "UD7姒傜巼=" << float(segmentStats[387]) / n << ","
        << "UD8姒傜巼=" << float(segmentStats[394]) / n << ","
        << "UD9姒傜巼=" << float(segmentStats[400]) / n << ","
        << "UC0姒傜巼=" << float(segmentStats[407]) / n << ","
        << "UC1姒傜巼=" << float(segmentStats[413]) / n << ","
        << "UC2姒傜巼=" << float(segmentStats[420]) / n << ","
        << "UC3姒傜巼=" << float(segmentStats[427]) / n << ","
        << "UC4姒傜巼=" << float(segmentStats[434]) / n << ","
        << "UC5姒傜巼=" << float(segmentStats[440]) / n << ","
        << "UC6姒傜巼=" << float(segmentStats[447]) / n << ","
        << "UC7姒傜巼=" << float(segmentStats[454]) / n << ","
        << "UC8姒傜巼=" << float(segmentStats[462]) / n << ","
        << "UC9姒傜巼=" << float(segmentStats[469]) / n << ","
        << "UB0姒傜巼=" << float(segmentStats[476]) / n << ","
        << "UB1姒傜巼=" << float(segmentStats[483]) / n << ","
        << "UB2姒傜巼=" << float(segmentStats[490]) / n << ","
        << "UB3姒傜巼=" << float(segmentStats[498]) / n << ","
        << "UB4姒傜巼=" << float(segmentStats[505]) / n << ","
        << "UB5姒傜巼=" << float(segmentStats[513]) / n << ","
        << "UB6姒傜巼=" << float(segmentStats[520]) / n << ","
        << "UB7姒傜巼=" << float(segmentStats[528]) / n << ","
        << "UB8姒傜巼=" << float(segmentStats[536]) / n << ","
        << "UB9姒傜巼=" << float(segmentStats[544]) / n << ","
        << "UA0姒傜巼=" << float(segmentStats[552]) / n << endl;
    }

  }

}
using namespace TestScoreSearch;
void main_testScoreSearch()
{
  // 妫�鏌ュ伐浣滅洰褰�
  GameDatabase::loadTranslation("./db/text_data.json");
  GameDatabase::loadUmas("./db/umaDB.json");
  GameDatabase::loadDBCards("./db/cardDB.json");

  test = TestConfig::loadFile("./testConfig.json");  
  cout << test.explain() << endl;
  totalGames = test.totalGames;
  gamesEveryThread = totalGames / threadNum;

  for (int i = 0; i < 700; i++)segmentStats[i] = 0;

  cout << "姝ｅ湪娴嬭瘯鈥︹�033[?25l" << endl;

  std::vector<std::thread> threads;
  for (int i = 0; i < threadNum; ++i) {
    threads.push_back(std::thread(worker));
  }
  for (auto& thread : threads) {
    thread.join();
  }

  cout << n << "灞�锛屾悳绱㈤噺=" << searchN << "锛屽钩鍧囧垎" << totalScore / n << "锛屾爣鍑嗗樊" << sqrt(totalScoreSqr / n - totalScore * totalScore / n / n) << "锛屾渶楂樺垎" << bestScore << endl;
  system("pause");

}