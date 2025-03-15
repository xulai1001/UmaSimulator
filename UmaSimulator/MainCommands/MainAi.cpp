#include <iostream>
#include <iomanip> 
#include <sstream>
#include <fstream>
#include <cassert>
#include <thread>  // for std::this_thread::sleep_for
#include <chrono>  // for std::chrono::seconds

#include "../Game/Game.h"
#include "../GameDatabase/GameDatabase.h"
#include "../GameDatabase/GameConfig.h"
#include "../Search/Search.h"
#include "../External/utils.h"
#include "../websocket.h"

#include "windows.h"
#include <filesystem>
#include <cstdlib>
using namespace std;
using json = nlohmann::json;

template <typename T, std::size_t N>
std::size_t findMaxIndex(const T(&arr)[N]) {
	return std::distance(arr, std::max_element(arr, arr + N));
}

map<string, string> rpText;
void loadRole()
{
	try
	{
		ifstream ifs("db/roleplay.json");
		stringstream ss;
		ss << ifs.rdbuf();
		ifs.close();

		rpText.clear();
		json j = json::parse(ss.str(), nullptr, true, true);
		json entry = j.at(GameConfig::role);
		for (auto& item : entry.items())
		{
			rpText[item.key()] = UTF8_To_string(item.value());
		}
		cout << "��ǰRP��ɫ��" << rpText["name"] << endl;
	}
	catch (...)
	{
		cout << "��ȡ������Ϣ����roleplay.json" << endl;
	}
}

void print_luck(int luck)
{
	int u = 0;//�°�ƽ��������Լ500����Ϊ���չ�����Ƚ�һ��Ϳ�û���Ƶ��ˣ����������ai��ֻ�ƫ�ߣ��������0��
	int sigma = 1500;
	string color = "";
	if (luck > 20000) u = 32000;//�õ�Ŀ�ƽ��ֵԼΪue6

	if (!GameConfig::noColor)
	{
		if (luck > u + sigma * 1.0)
			color = "\033[32m"; // 2 3 1
		else if (luck > u - sigma * 1.0)
			color = "\033[33m";
		else
			color = "\033[31m";
		cout << color << luck << "\033[0m";
	}
	else
		cout << luck;
}

void main_ai()
{
	//const double radicalFactor = 5;//������
	//const int threadNum = 16; //�߳���
	 // const int searchN = 12288; //ÿ��ѡ������ؿ���ģ��ľ���

	//������Ϊk��ģ��n��ʱ����׼��ԼΪsqrt(1+k^2/(2k+1))*1200/(sqrt(n))
	//��׼�����30ʱ������Ӱ���ж�׼ȷ��


	random_device rd;
	auto rand = mt19937_64(rd());

	int lastTurn = -1;
	ModelOutputValueV1 scoreFirstTurn = ModelOutputValueV1();   // ��һ�غϷ���
	ModelOutputValueV1 scoreLastTurn = ModelOutputValueV1();   // ��һ�غϷ���
	string lastJsonStr;//json str of the last time

	// ��鹤��Ŀ¼
	wchar_t buf[10240];
	GetModuleFileNameW(0, buf, 10240);
	filesystem::path exeDir = filesystem::path(buf).parent_path();
	filesystem::current_path(exeDir);
	std::cout << "��ǰ����Ŀ¼��" << filesystem::current_path() << endl;
	cout << "��ǰ����Ŀ¼��" << exeDir << endl;

#if USE_BACKEND == BACKEND_NONE
	GameConfig::load("./aiConfig_cpu.json");
#else
	GameConfig::load("./aiConfig.json");
#endif
	//GameDatabase::loadTranslation("./db/text_data.json");
	GameDatabase::loadUmas("./db/umaDB.json");
	//GameDatabase::loadCards("./db/card"); // ���벢����ʹ���ֶ�֧Ԯ������
	GameDatabase::loadDBCards("./db/cardDB.json"); //cardDB�����Ѿ���������
	//loadRole();   // roleplay

	bool uraFileMode = GameConfig::communicationMode == "urafile";
	//�Բ�Ӱ����ߣ�����ÿ���ļ��ı䶼ˢ��
	bool refreshIfAnyChanged = true;//if false, only new turns will refresh
	//bool refreshIfAnyChanged = GameConfig::communicationMode == "localfile";//if false, only new turns will refresh
	string currentGameStagePath = uraFileMode ?
		string(getenv("LOCALAPPDATA")) + "/UmamusumeResponseAnalyzer/GameData/thisTurn.json"
		: "./thisTurn.json";
	//string currentGameStagePath2 = 
	//	string(getenv("LOCALAPPDATA")) + "/UmamusumeResponseAnalyzer/GameData/turn34.json"
	//	
	//string currentGameStagePath = "./gameData/thisTurn.json";



	Model* modelptr = NULL;
	Model model(GameConfig::modelPath, GameConfig::batchSize);
	Model* modelSingleptr = NULL;
	Model modelSingle(GameConfig::modelPath, 1);
	if (GameConfig::modelPath != "")
	{
		modelptr = &model;
		modelSingleptr = &modelSingle;
	}
	else
	{
		GameConfig::maxDepth = 2 * TOTAL_TURN;
	}

	Model::printBackendInfo();

	SearchParam searchParam(
		GameConfig::searchSingleMax,
		GameConfig::searchTotalMax,
		GameConfig::searchGroupSize,
		GameConfig::searchCpuct,
		GameConfig::maxDepth,
		GameConfig::radicalFactor
	);
	Search search(modelptr, GameConfig::batchSize, GameConfig::threadNum, searchParam);
	//Search search2(modelptr, GameConfig::batchSize, GameConfig::threadNum, searchParam);
	//Evaluator evaSingle(modelSingleptr, 1);

	bool useWebsocket = GameConfig::communicationMode == "websocket";
	websocket ws(useWebsocket ? "http://127.0.0.1:4693" : "");
	if (useWebsocket)
	{
		do {
			Sleep(500);
			std::cout << "�ȴ�URA����" << std::endl;
		} while (ws.get_status() != "Open");
	}

	while (true)
	{
		Game game;
		//Game game2;
		string jsonStr;
		//string jsonStr2;
		if (useWebsocket)
		{
			jsonStr = lastFromWs;
		}
		else
		{

			while (!filesystem::exists(currentGameStagePath))
			{
				std::cout << "�Ҳ���" + currentGameStagePath + "������������δ��ʼ��С�ڰ�δ��������" << endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
			}
			ifstream fs(currentGameStagePath);
			if (!fs.good())
			{
				cout << "��ȡ�ļ�����" << endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
				continue;
			}
			ostringstream tmp;
			tmp << fs.rdbuf();
			fs.close();

			jsonStr = tmp.str();
			//ifstream fs2(currentGameStagePath2);
			//ostringstream tmp2;
			//tmp2 << fs2.rdbuf();
			//fs2.close();

			//jsonStr2 = tmp2.str();
		}

		if (lastJsonStr == jsonStr)//û�и���
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));//��һ��
			continue;
		}

		bool suc = game.loadGameFromJson(jsonStr);
		game.gameSettings.eventStrength = GameConfig::eventStrength;
		game.gameSettings.ptScoreRate = GameConfig::scorePtRate;
		game.gameSettings.scoringMode = GameConfig::scoringMode;
		//bool suc2 = game2.loadGameFromJson(jsonStr2);
		//game2.eventStrength = GameConfig::eventStrength;

		if (!suc)
		{
			cout << "���ִ���" << endl;
			if (jsonStr != "[test]" && jsonStr != "{\"Result\":1,\"Reason\":null}")
			{
				auto ofs = ofstream("lastError.json");
				ofs.write(jsonStr.data(), jsonStr.size());
				ofs.close();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));//�ӳټ��룬����ˢ��
			continue;
		}
		if (game.turn == lastTurn)
		{
			if (!refreshIfAnyChanged)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(300));//����Ƿ��и���
				continue;
			}
		}
		bool maybeNonTrainingTurn = true;//��ʱ���յ�һЩ��ѵ���غϵ���Ϣ����ͬ����û��ͷ������ѵ��û��ͷ�ĸ���Լ�����֮һ
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < 5; j++)
			{
				if (game.personDistribution[i][j] != -1)
					maybeNonTrainingTurn = false;
			}
		if (maybeNonTrainingTurn && !refreshIfAnyChanged)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));//����Ƿ��и���
			continue;
		}
		//cout << jsonStr << endl;
		lastTurn = game.turn;
		lastJsonStr = jsonStr;
		if (game.turn == 0)//��һ�غϣ���������ai�ĵ�һ�غ�
		{
			scoreFirstTurn = ModelOutputValueV1();
			scoreLastTurn = ModelOutputValueV1();
		}

		cout << endl;

		//if (game.stage == ST_train)
		//{
		//	Game game2 = game;
		//	for (int i = 0; i < 5; i++)
		//	{
		//		game2.randomizeTurn(rand);
				//game2.print();
		//	}
		//}
		/*{
			auto allActions = game.getAllLegalActions();
			for (int i = 0; i < allActions.size(); i++)
			{
				Action a = allActions[i];
				cout << i << " of " << allActions.size() << " " << a.stage << " " << a.idx << endl;
				Game game2 = game;
				game2.gameSettings.playerPrint = true;
				game2.applyActionUntilNextDecision(rand, a);
				game2.print();
				cout << endl;
			}
		}*/



		//game.print();


		//cout << rpText["name"] << rpText["calc"] << endl;
		auto printPolicy = [](float p)
			{
				cout << fixed << setprecision(1);
				if (!GameConfig::noColor)
				{
					if (p >= 0.3)cout << "\033[33m";
					//else if (p >= 0.1)cout << "\033[32m";
					else cout << "\033[36m";
				}
				cout << p * 100 << "% ";
				if (!GameConfig::noColor)cout << "\033[0m";
			};
		
		auto printColoredValue = [](double v0, double maxv, double ref)
			{
				double dif = v0 - maxv;
				if (dif < -5000)
				{
					cout <<  "---- ";
					return;
				}
				cout << fixed << setprecision(0);
				if (!GameConfig::noColor)
				{
					if (dif > -25) cout << "\033[41m\033[1;33m*";
					else if (dif > -100) cout << "\033[1;32m";
					else cout << "\033[33m";
				}
				cout << setw(6) << to_string(int(v0 - ref));
				if (!GameConfig::noColor)cout << "\033[0m";
				cout << " ";
			};

		//search.runSearch(game, GameConfig::searchN, TOTAL_TURN, 0, rand);
		
		if (game.turn < TOTAL_TURN)
		{

			//���ݻغ���Ϣ����debug
			try
			{
				std::filesystem::create_directories("log");
				string fname = "log/turn" + to_string(game.turn) + "_" + to_string(game.stage) + ".json";
				auto ofs = ofstream(fname);
				ofs.write(jsonStr.data(), jsonStr.size());
				ofs.close();
			}
			catch (...)
			{
				cout << "����غ���Ϣʧ��" << endl;
			}

			//game.applyAction(rand, Action(DISH_none, TRA_guts));
			game.print();
			//game2.print();
			//game = game2;

			Action hw = Evaluator::handWrittenStrategy(game);
			//cout << "��д�߼�: " << hw.toString(game) << endl;
			//Game game2 = game;
			//game2.gameSettings.playerPrint = true;
			//game2.applyActionUntilNextDecision(rand, hw);
			//game2.print();

			auto allAction = game.getAllLegalActions();
			vector<ModelOutputValueV1> actionValues;
			int bestIdx = -1;
			double bestValue = -1e9;
			for (int i = 0; i < allAction.size(); i++)
			{
				Action act = allAction[i];
				cout << act.toString(game) << " : ";
				auto v = search.evaluateAction(game, act, rand);
				cout << v.value << endl;
				actionValues.push_back(v);
				if (v.value > bestValue)
				{
					bestValue = v.value;
					bestIdx = i;
				}
			}


			double refScore = scoreLastTurn.value;

			ModelOutputValueV1 reRandomizeValue;//�������������ͷ����ȡbuff���������
			if (game.stage == ST_train || game.stage == ST_chooseBuff) {
				Game game3 = game;
				game3.undoRandomize();
				reRandomizeValue = search.evaluateNewGame(game3, rand);
				refScore = reRandomizeValue.value;
			}

			//ˢ�£�Ȼ��������ʾһ�����,�÷�����ȥ����
			for (int i = 0; i < allAction.size(); i++)
				cout << "\033[1A";
			for (int i = 0; i < allAction.size(); i++)
			{
				Action act = allAction[i];
				cout << act.toString(game) << " : ";
				printColoredValue(actionValues[i].value, bestValue, refScore);
				cout << endl;
			}


			cout << endl;
			Action bestAction = allAction[bestIdx];
			cout << "AI���飺" << bestAction.toString(game);
			if (game.stage == ST_chooseBuff && game.turn != 65)
				cout << " " << ScenarioBuffInfo::getScenarioBuffName(game.lg_pickedBuffs[bestAction.idx]);
			cout << endl;

			if (bestAction.stage == ST_train && bestAction.idx == T_outgoing)
			{
				Game game2 = game;
				game2.applyActionUntilNextDecision(rand, bestAction);
				if (game2.stage == ST_decideEvent && game2.decidingEvent == DecidingEvent_outing)
				{
					auto allAction2 = game2.getAllLegalActions();
					vector<ModelOutputValueV1> actionValues2;
					int bestIdx2 = -1;
					double bestValue2 = -1e9;
					for (int i = 0; i < allAction2.size(); i++)
					{
						Action act2 = allAction2[i];
						cout << act2.toString(game2) << " : ";
						auto v = search.evaluateAction(game2, act2, rand);
						printColoredValue(v.value, bestValue2, refScore);
						cout << endl;
						actionValues2.push_back(v);
						if (v.value > bestValue2)
						{
							bestValue2 = v.value;
							bestIdx2 = i;
						}
					}
					Action bestAction2 = allAction2[bestIdx2];
					cout << "AI���飺" << bestAction2.toString(game2) << endl;
				}
			}



			auto maxV = actionValues[bestIdx];


			cout << endl;
			if (game.turn == 0 || scoreFirstTurn.value == 0)
			{
				cout << "����Ԥ��: ƽ��\033[1;32m" << int(maxV.scoreMean) << "\033[0m" << "���ֹ�\033[1;36m+" << int(maxV.value - maxV.scoreMean) << "\033[0m" << endl;
				scoreFirstTurn = maxV;
			}
			else
			{
				cout << "����ָ�꣺" << " | ���֣�";
				print_luck(int(maxV.scoreMean - scoreFirstTurn.scoreMean));
				cout << " | ���غϣ�" << int(maxV.scoreMean - scoreLastTurn.scoreMean);
				if (game.stage==ST_train || game.stage == ST_chooseBuff) {
					cout << "��ѵ����\033[1;36m" << int(maxV.scoreMean - reRandomizeValue.scoreMean) << "\033[0m";
				}
				cout << " | ����Ԥ��: \033[1;32m" << int(maxV.scoreMean) << "\033[0m"
					<< "���ֹ�\033[1;36m+" << int(maxV.value - maxV.scoreMean) << "\033[0m��" << endl;

			}
			if (game.stage == ST_train && allAction[allAction.size() - 1].idx == T_race)
			{
				double raceLoss = bestValue - actionValues[actionValues.size() - 1].value;
				cout << "��������" << int(raceLoss) << endl;
			}


			scoreLastTurn = maxV;

			if (false)//debug
			{
				cout << "�ο��֣�" << refScore << endl;

				Game game4 = game;
				game4.gameSettings.playerPrint = true;
				game4.applyActionUntilNextDecision(rand, bestAction);
				game4.print(); 
				if (game4.stage == ST_train || game4.stage == ST_chooseBuff) {
					Game game3 = game;
					game3.undoRandomize();
					auto reRandomizeValue2 = search.evaluateNewGame(game3, rand);

					cout << "�»غϲο��֣�" << reRandomizeValue2.value << endl;
				}

			}

			/*

		evaSingle.gameInput[0] = game;
		evaSingle.evaluateSelf(1, searchParam);
		Action hl = evaSingle.actionResults[0];
		if (GameConfig::modelPath == "")
			cout << "��д�߼�: " << hl.toString() << endl;
		else
			cout << "��������: " << hl.toString() << endl;

		search.param.maxDepth = game.turn < 70 - GameConfig::maxDepth ? GameConfig::maxDepth : 2 * TOTAL_TURN;
		Action bestAction = search.runSearch(game, rand);
		cout << "���ؿ���: " << bestAction.toString() << endl;


		//������·��俨�飬ƽ�����Ƕ��٣��뵱ǰ�غ϶Աȿ��Ի���������
		ModelOutputValueV1 trainAvgScore = { -1,-1,-1 };
		double trainLuckRate = -1;

		if (game.cook_dish == DISH_none)
		{
			trainAvgScore = search2.evaluateNewGame(game, rand);

			//���·��俨�飬�ж����ʱ���غϺ�
			if (modelptr != NULL)//ֻ���������֧�ִ˹���
			{
				int64_t count = 0;
				int64_t luckCount = 0;
				auto& eva = search2.evaluators[0];
				eva.gameInput.assign(eva.maxBatchsize, game);
				eva.evaluateSelf(0, search2.param);
				double refValue = eva.valueResults[0].scoreMean;//��ǰѵ����ƽ����

				int batchN = 1 + 4 * GameConfig::searchSingleMax / eva.maxBatchsize;
				for (int b = 0; b < batchN; b++)
				{
					for (int i = 0; i < eva.maxBatchsize; i++)
					{
						eva.gameInput[i] = game;
						eva.gameInput[i].randomDistributeCards(rand);
					}
					eva.evaluateSelf(0, search2.param);
					for (int i = 0; i < eva.maxBatchsize; i++)
					{
						count++;
						if (eva.valueResults[i].scoreMean < refValue)
							luckCount++;
					}

				}
				trainLuckRate = double(luckCount) / count;
			}
		}
		double maxMean = -1e7;
		double maxValue = -1e7;
		for (int i = 0; i < Action::MAX_ACTION_TYPE; i++)
		{
			if (!search.allActionResults[i].isLegal)continue;
			auto v = search.allActionResults[i].lastCalculate;
			if (v.value > maxValue)
				maxValue = v.value;
			if (v.scoreMean > maxMean)
				maxMean = v.scoreMean;
		}

		Action restAction;
		restAction.dishType = DISH_none;
		restAction.train = TRA_rest;
		Action outgoingAction;
		outgoingAction.dishType = DISH_none;
		outgoingAction.train = TRA_outgoing;
		//��Ϣ������������ߵ��Ǹ������������Ϊ��ʾ�ο�
		double restValue = search.allActionResults[restAction.toInt()].lastCalculate.value;
		double outgoingValue = search.allActionResults[outgoingAction.toInt()].lastCalculate.value;
		if (outgoingValue > restValue)
			restValue = outgoingValue;


		wstring strToSendURA = L"UMAAI_COOK";
		strToSendURA += L" " + to_wstring(game.turn) + L" " + to_wstring(maxMean) + L" " + to_wstring(scoreFirstTurn) + L" " + to_wstring(scoreLastTurn) + L" " + to_wstring(maxValue);
		if (game.turn == 0 || scoreFirstTurn == 0)
		{
			//cout << "����Ԥ��: ƽ��\033[1;32m" << int(maxMean) << "\033[0m" << "���ֹ�\033[1;36m+" << int(maxValue - maxMean) << "\033[0m" << endl;
			scoreFirstTurn = search.allActionResults[outgoingAction.toInt()].lastCalculate.scoreMean;
		}
		//else
		{
			cout << "����ָ�꣺" << " | ���֣�";
			print_luck(maxMean - scoreFirstTurn);
			cout << " | ���غϣ�" << maxMean - scoreLastTurn;
			if (trainAvgScore.value >= 0) {
				cout << "��ѵ����\033[1;36m" << int(maxMean - trainAvgScore.scoreMean) << "\033[0m";

				if (trainLuckRate >= 0)
				{
					cout << fixed << setprecision(2) << " ������\033[1;36m" << trainLuckRate * 100 << "%\033[0m";
				}
				cout << "��";
			}
			cout	<< " | ����Ԥ��: \033[1;32m" << maxMean << "\033[0m"
				<< "���ֹ�\033[1;36m+" << int(maxValue - maxMean) << "\033[0m��" << endl;

		}
		cout.flush();
		scoreLastTurn = maxMean;

		for (int tr = 0; tr < 8; tr++)
		{
			Action a;
			a.dishType = DISH_none;
			a.train = tr;
			double value = search.allActionResults[a.toInt()].lastCalculate.value;
			strToSendURA += L" " + to_wstring(tr) + L" " + to_wstring(value - restValue) + L" " + to_wstring(maxValue - restValue);
			printValue(a.toInt(), value - restValue, maxValue - restValue);
			//cout << "(" << search.allActionResults[a.toInt()].num << ")";
			//cout << "(��" << 2 * int(Search::expectedSearchStdev / sqrt(search.allActionResults[a.toInt()].num)) << ")";
			if (tr == TRA_race && game.isLegal(a))
			{
				cout << "(��������:\033[1;36m" << maxValue - value << "\033[0m��";
			}
		}
		cout << endl;


		//strToSendURA = L"0.1234567 5.4321";
		if (useWebsocket)
		{
			wstring s = L"{\"CommandType\":1,\"Command\":\"PrintUmaAiResult\",\"Parameters\":[\"" + strToSendURA + L"\"]}";
			//ws.send(s);
		}*/

		}
	
	}
}