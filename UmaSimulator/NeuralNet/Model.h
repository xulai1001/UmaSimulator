#pragma once
#include "NNInput.h"
#include "../config.h"
#include "../Game/Game.h"
#include <string>
#include <vector>
#include <mutex>

#define NO_NN_GAME_PERSON //uaf�籾û�е�����person����������ƿ����ŵľ籾��ע�͵����У�����cuda
//#define COMPRESS_NNINPUT //ѹ�����룬��ʡpcie��������cuda

const int NN_Game_Card_Num = 6;//7�ſ���ÿ�����ҽ���һ��λ��Ϊ�գ�
const int NN_Game_Person_Num = 0;//20����ͷ��ÿ�����ҽ��ж���λ��Ϊ�գ�

#ifdef NO_NN_GAME_PERSON
static_assert(NN_Game_Person_Num == 0);
const int NN_TF_NUM = NN_Game_Card_Num;//transformer�м�����
#else
static_assert(NN_Game_Person_Num != 0);
const int NN_TF_NUM = NN_Game_Person_Num;//transformer�м�����
#endif

#if USE_BACKEND == BACKEND_LIBTORCH
#include <torch/torch.h>
#include <torch/script.h>
#endif

#if USE_BACKEND == BACKEND_CUDA
#include <cublas_v2.h>
#include <cuda_runtime.h>
#endif

#if USE_BACKEND == BACKEND_ONNX
#include <onnxruntime_cxx_api.h>
#endif

//��-1e7��ʾ���Ϸ���������������ѵ��ʱֱ�Ӻ��Ե���Щֵ
struct ModelOutputPolicyV1
{
  float actionPolicy[Action::MAX_ACTION_TYPE];//ֱ�Ӱ�˳���о�

};
static_assert(sizeof(ModelOutputPolicyV1) == sizeof(float) * NNOUTPUT_CHANNELS_POLICY_V1, "NNOUTPUT_CHANNELS_POLICY_V1����");


struct ModelOutputValueV1
{
  float scoreMean;//score��ƽ��ֵ
  float scoreStdev;//score�ı�׼��
  //float winRate;//score>=target�ĸ���
  float value;//���Ǽ�����֮��Ĵ��
  //float extract(int i);
  static const ModelOutputValueV1 illegalValue;
};
static_assert(sizeof(ModelOutputValueV1) == sizeof(float) * NNOUTPUT_CHANNELS_VALUE_V1, "NNOUTPUT_CHANNELS_VALUE_V1����");

struct ModelOutputV1
{
  ModelOutputValueV1 value;
  ModelOutputPolicyV1 policy;
};
static_assert(sizeof(ModelOutputV1) == sizeof(float) * NNOUTPUT_CHANNELS_V1,"NNOUTPUT_CHANNELS_V1����");

#if USE_BACKEND != BACKEND_NONE && USE_BACKEND != BACKEND_LIBTORCH 
struct ModelWeight
{
  static const int encoderLayer = 1;
  static const int encoderCh = 256;
  static const int mlpBlock = 2;
  static const int globalCh = 256;
  static const int mlpCh = 256;

  //Ϊ��ʡ�£����󲿷����Ա任û��biasֻ��weight
  //�ڴ����з�ʽ������ͨ������
  std::vector<float> inputheadGlobal1;// [globalCh * (NNINPUT_CHANNELS_GAMEGLOBAL_V1 + NNINPUT_CHANNELS_SEARCHPARAM_V1)] ;
  std::vector<float> inputheadGlobal2;// [encoderCh * globalCh];
  std::vector<float> inputheadCard;// [encoderCh * NNINPUT_CHANNELS_CARD_V1];
#ifndef NO_NN_GAME_PERSON
  std::vector<float> inputheadPerson;// [encoderCh * NNINPUT_CHANNELS_PERSON_V1];
#endif // !NO_NN_GAME_PERSON


  std::vector<float> encoder_Q[encoderLayer];// [encoderCh * encoderCh];
  std::vector<float> encoder_V[encoderLayer];// [encoderCh * encoderCh];
  std::vector<float> encoder_global[encoderLayer];// [encoderCh * globalCh];

  std::vector<float> linBeforeMLP1;// [mlpCh * encoderCh];
  std::vector<float> linBeforeMLP2;// [mlpCh * globalCh];

  std::vector<float> mlp_lin[mlpBlock][2];// [mlpCh * mlpCh];

  std::vector<float> outputhead_w;// [NNOUTPUT_CHANNELS_V1 * mlpCh];
  std::vector<float> outputhead_b;// [NNOUTPUT_CHANNELS_V1];

  void load(std::string path);
};
#endif


#if USE_BACKEND == BACKEND_CUDA
struct ModelCudaBuf
{
  cublasHandle_t cublas;
  //cudnnHandle_t cudnn;


  //�ڴ����з�ʽ������ͨ������
  float* inputheadGlobal1;
  float* inputheadGlobal2;
  float* inputheadCard;
#ifndef NO_NN_GAME_PERSON
  float* inputheadPerson;
#endif // !NO_NN_GAME_PERSON

  float* encoder_Q[ModelWeight::encoderLayer];
  float* encoder_V[ModelWeight::encoderLayer];
  float* encoder_global[ModelWeight::encoderLayer];

  float* linBeforeMLP1;
  float* linBeforeMLP2;

  float* mlp_lin[ModelWeight::mlpBlock][2];

  float* outputhead_w;
  float* outputhead_b;

  //�м����
  uint16_t* inputOnesIdx;
  uint16_t* inputFloatIdx;
  float* inputFloatValue;

  float* input;
  float* inputGlobal;
  float* inputCard;
#ifndef NO_NN_GAME_PERSON
  float* inputPerson;
#endif // !NO_NN_GAME_PERSON
  float* gf;
  float* encoderInput_gf;
#ifndef NO_NN_GAME_PERSON
  float* encoderInput_cardf;
#endif // !NO_NN_GAME_PERSON
  float* encoderInput;//self.inputheadPerson(x3)ֱ�ӱ���������Ȼ�����gf��cardf��Ȼ��relu

  float* encoderQ;
  float* encoderV;
  float* encoderAtt;
  float* encoderGf;
  float* encoderOutput;

  float* encoderSum;
  float* mlpInput;
  float* mlpMid;
  float* mlpMid2;

  float* outputhead_b_copied;
  float* output;


  void init(const ModelWeight& weight, int batchSize);
  ~ModelCudaBuf();
};
#endif


class Evaluator;



class Model
{
public:
  //static lock;//���е�evaluator����һ��lock
  Model(std::string path, int batchsize);
  void evaluate(Evaluator* eva, float* inputBuf, float* outputBuf, int gameNum);//����gamesBuf��gameNum����Ϸ������������outputBuf
  static void printBackendInfo();



private:
#if USE_BACKEND == BACKEND_ONNX
  Ort::Env onnxEnv;
  Ort::Session onnxSession{ nullptr };
  Ort::Value onnxInput{ nullptr };
  Ort::MemoryInfo onnxMemoryInfo{ nullptr }; 

#endif

#if USE_BACKEND == BACKEND_LIBTORCH
  torch::jit::script::Module model;
#endif

#if USE_BACKEND != BACKEND_NONE && USE_BACKEND != BACKEND_LIBTORCH 
  const int batchSize;
  std::mutex mtx; // ������
#endif

#if USE_BACKEND == BACKEND_CUDA
  ModelWeight modelWeight;
  ModelCudaBuf cb;
#endif
};