锘�#pragma once

#define UMAAI_MAINAI   //浣跨敤ai
//#define UMAAI_TESTSCORE   //娴嬭瘯ai鍒嗘暟
//#define UMAAI_TESTCARDSSINGLE   //娴嬪崱锛屾帶鍒朵簲寮犲崱涓嶅彉鍙敼鍙樹竴寮�
//#define UMAAI_SIMULATOR   //鍏婚┈妯℃嫙鍣�
//#define UMAAI_SELFPLAY   //璺戞暟鎹紙鐢ㄤ簬绁炵粡缃戠粶璁粌锛�
//#define UMAAI_TESTLIBTORCH   //娴嬭瘯c++鐗坱orch
//#define UMAAI_MODELBENCHMARK   //娴嬭瘯绁炵粡缃戠粶閫熷害
//#define UMAAI_TESTSCORESEARCH //娴嬭瘯钂欑壒鍗℃礇寮哄害
//#define UMAAI_TESTSCORENOSEARCH //娴嬭瘯绁炵粡缃戠粶/鎵嬪啓閫昏緫policy寮哄害


#define PRINT_GAME_EVENT
#if defined UMAAI_TESTSCORE || defined UMAAI_SIMULATOR 
#endif

const bool PrintHandwrittenLogicValueForDebug = false;

#define BACKEND_NONE 0//涓嶄娇鐢ㄧ缁忕綉缁滐紝浣跨敤鎵嬪啓閫昏緫
#define BACKEND_LIBTORCH 1//浣跨敤libtorch(GPU鎴朇PU)璁＄畻绁炵粡缃戠粶
#define BACKEND_CUDA 2//浣跨敤cuda(GPU)璁＄畻绁炵粡缃戠粶
#define BACKEND_EIGEN 3//浣跨敤Eigen搴�(CPU)璁＄畻绁炵粡缃戠粶
#define BACKEND_ONNX 4//浣跨敤ONNX-DirectML搴�(GPU)璁＄畻绁炵粡缃戠粶

#define USE_BACKEND BACKEND_NONE

const int MAX_SCORE = 200000;//鏈�澶у厑璁哥殑鍒嗘暟

#if USE_BACKEND == BACKEND_LIBTORCH || defined UMAAI_TESTLIBTORCH

const int LIBTORCH_USE_GPU = true;//鏄惁浣跨敤GPU

//淇敼浠ヤ笅涓や釜鐩綍鐨勫悓鏃讹紝闄勫姞鍖呭惈鐩綍涔熼渶瑕佷慨鏀�
#define TORCH_LIBROOT "C:/local/libtorch/lib/"
#define TORCH_LIBROOT_DEBUG "C:/local/libtorch_debug/lib/"

#endif


#if USE_BACKEND == BACKEND_CUDA

//淇敼浠ヤ笅鐩綍鐨勫悓鏃讹紝闄勫姞鍖呭惈鐩綍涔熼渶瑕佷慨鏀�
#define CUDA_LIBROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.1/lib/x64/"

#endif