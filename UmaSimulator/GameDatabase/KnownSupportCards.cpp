#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "GameDatabase.h"
#include "../Game/Game.h"

using json = nlohmann::json;
using namespace std;

unordered_map<int, SupportCard> GameDatabase::AllCards;
unordered_map<int, SupportCard> GameDatabase::DBCards;

void GameDatabase::loadCards(const string& dir)
{
    try
    {
        for (auto entry : filesystem::directory_iterator(dir + "/"))
        {
            //cout << entry.path() << endl;
            if (entry.path().extension() == ".json")
            {
                try
                {
                    ifstream ifs(entry.path());
                    stringstream ss;
                    ss << ifs.rdbuf();
                    ifs.close();
                    json j = json::parse(ss.str(), nullptr, true, true);

                    SupportCard jdata[5];

                    for (int x = 0; x < 5; ++x) {
                        jdata[x].load_from_json(j, x);
                    }

                    cout << "����֧Ԯ�� #" << jdata[4].cardName << " --- " << jdata[4].cardID << endl;
                    if (GameDatabase::AllCards.count(jdata[4].cardID) > 0)
                        cout << "�����ظ�֧Ԯ�� #" << jdata[4].cardName << " --- " << jdata[4].cardID << endl;
                    else {
                        for (int x = 0; x < 5; ++x) 
                            GameDatabase::AllCards[jdata[x].cardID] = jdata[x];
                    }
                        
                }
                catch (exception& e)
                {
                    cout << "֧Ԯ����ϢJSON����: " << entry.path() << endl << e.what() << endl;
                }
            }
        }
        cout << "������ " << GameDatabase::AllCards.size() << " ֧Ԯ��" << endl;
    }
    catch (exception& e)
    {
        cout << "��ȡ֧Ԯ����Ϣ����: " << endl << e.what() << endl;
    }
    catch (...)
    {
        cout << "��ȡ֧Ԯ����Ϣ����δ֪����" << endl;
    }
}

void GameDatabase::loadDBCards(const string& pathname)
{
    try
    {
        ifstream ifs(pathname);
        stringstream ss;
        ss << ifs.rdbuf();
        ifs.close();
        json j = json::parse(ss.str(), nullptr, true, true);

        for (auto & it : j.items()) 
        {
           // cout << it.key() << endl;
            for (int x = 0; x < 5; ++x) {
                SupportCard jdata;
                jdata.load_from_json(it.value(),x);
                jdata.isDBCard = true;
                if (GameDatabase::AllCards.count(jdata.cardID) > 0) // ����Ѿ���loadCards���루���ֶ����ݣ�
                    GameDatabase::DBCards[jdata.cardID] = jdata;    // ��Ȼ�������ݴ���DBCards�����ûʹ�����������������
                else
                    GameDatabase::AllCards[jdata.cardID] = jdata;   // û���ֶ����������Զ�����
            }
        }
        cout << "������ " << GameDatabase::AllCards.size()/5 << " ֧Ԯ��" << endl;
    }
    catch (exception& e)
    {
        cout << "��ȡ֧Ԯ����Ϣ����: " << endl << e.what() << endl;
    }
    catch (...)
    {
        cout << "��ȡ֧Ԯ����Ϣ����δ֪����" << endl;
    }
}

CardTrainingEffect SupportCard::getCardEffect(const Game& game, int atTrain, int jiBan, int effectFactor) const
{
    CardTrainingEffect effect(this);
    
    // �Ƿ�����
    // ���ڵ������ǣ������ﵥ���ж��Ƿ����ʾ��޷���֪�������Ĳ�Ȧ�����
    // ����취������Ҫ��cardEffect���ݴ�
    bool isShining = false;
    if (cardType < 5 && jiBan >= 80 && cardType == atTrain)
        isShining = true;

    if (isDBCard || effectFactor==-1)  // ��ʱʹ��effectFactor=-1��ʾ����ģʽ
    {
        // �µĹ��д������
        // ���ڲ���ά��isFixed״̬��ÿ�ζ�Ҫ���¼���
        double rate;
        int count, totalJiBan;
        double expectedVital;
        auto args = uniqueEffectParam;
        int type = uniqueEffectType;

        if (effectFactor == -1)
        {
            // ����д��Ҳʹ���´���/�����ݼ��㣬�����ȶԼ�����
            // �������¿����ݳ�ʼ���Լ�
            effect = CardTrainingEffect(&GameDatabase::DBCards[cardID]);
            type = GameDatabase::DBCards[cardID].uniqueEffectType;
            args = GameDatabase::DBCards[cardID].uniqueEffectParam;
        }


        //��֪������
        // 1.�Ǹ߷�ȣ�����xx���ܸ����������غ�������
        // 2.��˫���ֵȣ�����ѵ������������Ϊȫ��
        // 3.��̫����ȣ�ĳ�������������ʣ�����Ϊȫ��
        // 4.�ٳ���·����˿���������غ�������
        // 5.�ǲ��ԡ���С��ñ�ȣ���ʼ״̬��أ�����ʱûд
        // 6.������ɽ�������ʧ���ʱ��0������ʱûд
        // ��ȷ������û��������
        switch (type)
        {
            case 0:
                break;
            case 1:   // �>=args[1]ʱ��Ч
            case 2:
                if (jiBan >= args[1])
                {
                    if (args[2] > 0)
                        effect.apply(args[2], args[3]);
                    if (args[4] > 0)
                        effect.apply(args[4], args[5]);
                    if (cardID / 10 == 30137)    // ���Ŷ����������
                    {
                        effect.apply(1, 10).apply(2, 15);
                    }
                }
                break;
            case 3:   // �������+���ó�ѵ��
                if (jiBan >= args[1] && cardType != atTrain)
                    effect.xunLian += 20;
                break;
            case 4:   // ����·��û�з�˿�����ûغ�������
              rate = game.turn <= 33 ? 0 :
                game.turn <= 40 ? 0.2 :
                game.turn <= 42 ? 0.25 :
                game.turn <= 58 ? 0.7 :
                1.0;
                effect.xunLian += rate * (double)args[2];
                break;
            case 5:   // ���ݱ��֧Ԯ�����͵ĳ�ʼ���Լӳ�(��Ӧ����+10������/�Ŷӿ�ȫ����+2), �ݲ�����
                break;
            case 6:   // �����ǣ���Ҫ�õ�effectFactor
                effect.apply(1, max(0, min(args[1], args[1] - effectFactor)) * args[3]);
                break;
            case 7:   // ���񣬵�
                // ��Ҫ����ѵ�������������ʱ��-20����
                expectedVital = atTrain == 4 ? game.vital + 8 : game.vital - 20;
                rate = clamp(expectedVital / game.maxVital, 0.3, 1.0);
                // (0.3, 1) --> (1, 0)
                effect.apply(1, args[5] + args[2] * (1 - rate) / 0.7);
                break;
            case 8:   // ����
                effect.xunLian += 5 + 3 * clamp((game.maxVital - 100) / 4, 0, 5);
                break;
            case 9:   // �ر�����Ҫ�������
                totalJiBan = 0;
                assert(game.normalCardCount >= 5);
                for (int i = 0; i < game.normalCardCount; ++i)
                    totalJiBan += game.persons[i].friendship;
                if (game.larc_zuoyueType != 0)
                  totalJiBan += game.persons[17].friendship;
                rate = double(totalJiBan) / 600;
                effect.xunLian += rate * 20;
                break;
            case 10:   // ����ӥ����Ҫ����ͬʱѵ���Ŀ�����
                count = 0;
                for (int i = 0; i < 5; ++i)
                {
                    int pid = game.personDistribution[atTrain][i];
                    bool isCard = pid < game.normalCardCount || (game.larc_zuoyueType != 0 && pid == 17);
                    if(isCard)
                        ++count;
                }
                effect.xunLian += args[2] * min(5, count);
                break;
            case 11:   // ˮ˾������Ҫ��ǰѵ���ȼ�
                effect.xunLian += args[2] * min(5, 1 + game.getTrainingLevel(atTrain));
                break;
            case 12:   // ����ɽ
                break;
            case 13:   // B95���齴
                if (game.trainShiningCount(atTrain) >= 1)
                    effect.apply(args[1], args[2]);
                break;
            case 14:   // ������, ��ʱ��ѵ��ǰ������
                rate = clamp((double)game.vital / 100, 0.0, 1.0);    // >100Ҳ����ѵ��
                effect.xunLian += 5 + 15 * rate;
                break;
            case 15:   // ��ñ����ʱ����
                break;
            case 16:   // x��xxx���ͼ��ܣ����xxx�ӳɣ������Ǹ߷壩
                if (args[1] == 1)//�ٶȼ���
                {
                  count = 1 + game.turn / 6;
                }
                else if (args[1] == 2)//���ٶȼ���
                {
                  count = 0.7 + game.turn / 12.0;
                }
                else if (args[1] == 3)//���弼��
                {
                  count = 0.4 + game.turn / 15.0;
                }
                else
                {
                  count = 0;
                  assert(false && "δ֪�Ĺ�������֧Ԯ������");
                }
                if (count > args[4])
                  count = args[4];
                effect.apply(args[2], args[3] * count);
                break;
            case 17:   // �����
                count = 0;
                for (int i = 0; i < 5; ++i)
                    count += min(5, 1+game.getTrainingLevel(i));
                effect.xunLian += (count / 25.0) * args[3];
                break;
            case 18:   // ����
                break;
            case 19:    // ����
                break;
            case 20:    // �޽�
                {
                    int cardTypeCount[7] = { 0,0,0,0,0,0,0 };
                    for (int i = 0; i < 6; i++)
                    {
                        int t = game.cardParam[i].cardType;
                        assert(t <= 6 && t >= 0);
                        cardTypeCount[t]++;
                    }
                    cardTypeCount[5] += cardTypeCount[6];
                    for (int i = 0; i < 4; i++)
                        if (cardTypeCount[i] > 0)
                            effect.apply(i + 3, cardTypeCount[i]);  // ���������� = 0-4 = CardEffect����3-7
                    if (cardTypeCount[5] > 0)
                        effect.apply(30, cardTypeCount[5]); // pt = 30
                }
            break;
            case 21:   // ������������4��֧Ԯ��ʱ+10ѵ��
              {
                int cardTypeCount[7] = { 0,0,0,0,0,0,0 };
                for (int i = 0; i < 6; i++)
                {
                  int t = game.cardParam[i].cardType;
                  assert(t <= 6 && t >= 0);
                  cardTypeCount[t]++;
                }
                int cardTypes = 0;
                for (int i = 0; i < 7; i++)
                  if (cardTypeCount[i] > 0)
                    cardTypes++;
                if (cardTypes >= args[1])
                  effect.apply(args[2], args[3]);
              }
              break;
            case 22:    // ���³�
                break;
            default:   // type == 0
                if (uniqueEffectType != 0) {
                    //cout << "δ֪���� #" << uniqueEffectType << endl;
                }
                break;
        }   // switch
    }
    else
    {
        // �ϰ汾���д���
        int cardSpecialEffectId = cardID / 10;

        //�������Ǹ��ֹ���
        //1.����
        if (cardSpecialEffectId == 30137)
        {
            if (jiBan < 100)
            {
                if (isShining)
                    effect.youQing = 20;
                effect.ganJing = 0;
                effect.bonus[5] = 0;
            }
        }
        //2.�߷�
        //Ϊ�˼򻯣���Ϊ��ʼѵ���ӳ���4%����һ��������20%��Ҳ���ǵ�n���غ�4+n*(2/3)%
        else if (cardSpecialEffectId == 30134)
        {
            if (game.turn < 24)
                effect.xunLian = 4 + 0.6666667 * game.turn;
        }
        //3.����
        else if (cardSpecialEffectId == 30010)
        {
            //ɶ��û��
        }
        //4.��������
        else if (cardSpecialEffectId == 30019)
        {
            //ɶ��û��
        }
        //5.������
        else if (cardSpecialEffectId == 30011)
        {
            //ɶ��û��
        }
        //6.ˮ˾��
        else if (cardSpecialEffectId == 30107)
        {

            int traininglevel = game.getTrainingLevel(atTrain);
            effect.xunLian = 5 + traininglevel * 5;
            if (effect.xunLian > 25)effect.xunLian = 25;
        }
        //7.����˹
        else if (cardSpecialEffectId == 30130)
        {
            if (jiBan < 80)
            {
                effect.bonus[2] = 0;
            }
        }
        //8.���ʵ�
        else if (cardSpecialEffectId == 30037)
        {
            if (jiBan < 80)
            {
                effect.bonus[0] = 0;
            }
        }
        //9.������
        else if (cardSpecialEffectId == 30027)
        {
            //ɶ��û��
        }
        //10.�ٱ�Ѩ
        else if (cardSpecialEffectId == 30147)
        {
            if (jiBan < 100)
            {
                effect.bonus[0] = 0;
            }
        }
        //11.�ͺ���
        else if (cardSpecialEffectId == 30016)
        {
            //ɶ��û��
        }
        //12.�Ǻø��
        else if (cardSpecialEffectId == 30152)
        {
            if (jiBan < 80)
            {
                effect.bonus[0] = 0;
            }
        }
        //13.���ƽ��
        else if (cardSpecialEffectId == 30153)
        {
            if (jiBan < 100)
            {
                effect.bonus[3] = 0;
            }
        }
        //14.�ǲ���
        else if (cardSpecialEffectId == 30141)
        {
            //ɶ��û��
        }
        //15.�͵Ҷ�˹
        else if (cardSpecialEffectId == 30099)
        {
            int totalJiBan = 0;
            for (int i = 0; i < game.normalCardCount; i++)
                totalJiBan += game.persons[i].friendship;
            if (game.larc_zuoyueType != 0)
                totalJiBan += game.persons[17].friendship;
            effect.xunLian = totalJiBan / 30;
        }
        //����
        else if (cardSpecialEffectId == 30101) {
            if (jiBan < 100)
            {
                if (effect.youQing > 0)
                    effect.youQing = 20;
            }
        }
        //22���͹��
        else if (cardSpecialEffectId == 30142)
        {
            if (game.turn < 24)
                effect.bonus[1] = 1;
            else if (game.turn < 48)
                effect.bonus[1] = 2;
            else
                effect.bonus[1] = 3;
        }
        //23������
        else if (cardSpecialEffectId == 30123)
        {
            int traininglevel = game.getTrainingLevel(atTrain);
            effect.xunLian = 5 + traininglevel * 5;
            if (effect.xunLian > 25)effect.xunLian = 25;
        }
        //24������
        else if (cardSpecialEffectId == 30151)
        {
            if (jiBan < 100)
            {
                effect.xunLian = 0;
            }
        }
        //25����ǡ
        else if (cardSpecialEffectId == 30138)
        {
            if (jiBan < 100)
            {
                effect.bonus[2] = 0;
            }
        }
        //28������
        else if (cardSpecialEffectId == 30112)
        {
            //�Ժ�����취
        }
        //29������
        else if (cardSpecialEffectId == 30083)
        {
            if (jiBan < 80 || atTrain == 3)
                effect.xunLian = 0;
        }
        //30������
        else if (cardSpecialEffectId == 30094)
        {
            if (effect.youQing > 0)
            {
                float extraBonus = 5 + (100 - game.vital) / 7.0;
                if (extraBonus > 15)extraBonus = 15;
                if (extraBonus < 5)extraBonus = 5; // std::cout << effect.youQing << " ";

                effect.youQing = 120 * (1 + 0.01 * extraBonus) - 100;

            }

        }
        //Ҳ��
        else if (cardSpecialEffectId == 30126)
        {
            if (jiBan < 80)
            {

                effect.bonus[5] = 0;
            }
        }
        //����
        else if (cardSpecialEffectId == 30127)
        {
            if (isShining)
            {
                effect.ganJing = 60;
            }
        }
        //����
        else if (cardSpecialEffectId == 47)
        {
            //null
        }
        //�ٶ���
        else if (cardSpecialEffectId == 30119)
        {
            if (jiBan < 80)
            {
                effect.bonus[2] = 0;
            }
        }
        // ����
        else if (cardSpecialEffectId == 30067) {

            if (jiBan < 80)
                effect.bonus[5] = 0;

        }
        // �챦
        else if (cardSpecialEffectId == 30114) {
            if (jiBan < 80)
                effect.bonus[2] = 0;
        }
        // ����
        else if (cardSpecialEffectId == 30078) {
            effect.failRateDrop += 10;
        }
        // ��ñ
        else if (cardSpecialEffectId == 30021) {
            effect.failRateDrop += 7;
            effect.vitalCostDrop += 4;
        }
        // ������
        else if (cardSpecialEffectId == 30156) {
            if (jiBan < 80)
            {
                effect.bonus[2] = 0;
            }
        }
        // ���ɾ�
        else if (cardSpecialEffectId == 30132) {
            int guyouLevel = (game.maxVital - 100) / 4;
            if (guyouLevel > 5)guyouLevel = 5;
            effect.xunLian = 5 + 3 * guyouLevel;
        }
        //����ӥ
        else if (cardSpecialEffectId == 30161)
        {
            if (jiBan < 100)
            {
                for (int i = 0; i < 5; i++)
                    effect.bonus[i] -= 1;
            }
        }
        //�ٻƽ�
        else if (cardSpecialEffectId == 30168)
        {
            if (jiBan < 80)
            {
                effect.bonus[0] -= 1;
            }
        }
        else if (cardSpecialEffectId == 30154)
        {
            if (game.turn < 24)
                effect.bonus[0] = 1;
            else if (game.turn < 48)
                effect.bonus[0] = 2;
            else
                effect.bonus[0] = 3;
        }
        else if (cardSpecialEffectId == 30165)
        {
            if (jiBan >= 80)
            {
                effect.bonus[3] += 2;
            }
        }
        else if (cardSpecialEffectId == 30139)
        {
            if (jiBan >= 100)
            {
                effect.bonus[1] += 3;
            }
        }
        else if (cardSpecialEffectId == 30164)
        {
            if (jiBan >= 80)
            {
                effect.bonus[5] += 2;
            }
        }
        else if (cardSpecialEffectId == 30166)
        {
            if (game.turn < 12)
                effect.xunLian += 5.0 + (10.0 / 12) * game.turn;
            else
                effect.xunLian += 15;
        }
        else if (cardSpecialEffectId == 30158)
        {
            if (jiBan >= 100)
            {
                effect.youQing += (100 + effect.youQing) * 0.2;
            }
        }
        else if (cardSpecialEffectId == 30148)
        {
            int t = 0;
            for (int i = 0; i < 5; i++)
                t += game.getTrainingLevel(atTrain);
            int y = 5 + t;
            if (y > 20)y = 20;
            effect.xunLian += y;
        }
        else if (cardSpecialEffectId == 30163)
        {
            if (jiBan >= 80)
            {
                effect.xunLian += 10;
            }
        }
        // �ǲ���
        else if (cardSpecialEffectId == 30157) {
            if (jiBan >= 100)
            {
                effect.bonus[4] = 2;
                effect.bonus[5] = 1;
            }
        }
        //[��]�湭�쳵(id:30149)�Ĺ��������ʵ�ѵ��60�ɾ��ӳɣ������ڰ������ͷ���һ��֮ǰ����֪����û���ʣ���˼���������ͷ֮����Ҫ��������ſ��Ĳ������д���
        //��������д��Game�����ˣ���Ȼ�ܳ�ª����û�뵽ʲô�ð취
        else if (cardSpecialEffectId == 30149)
        {
            //��������ѵ��ʱ���ɾ��ӳ�60
            //���г����ȸ߷��150���
            //ʵ�ʱȹ��г�����100���
            effect.ganJing += 60;
        }
        else
        {
            //  std::cout << "δ֪��";
        }
    }
    
    // ��Ȧ���Ų�����
    if (!isShining)
        effect.youQing = 0;
    if (!isShining || atTrain != 4)
        effect.vitalBonus = 0;
    return effect;
}
