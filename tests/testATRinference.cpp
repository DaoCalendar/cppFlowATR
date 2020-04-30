//
// Created by BE.
// Changed be
//

#include "cppflow/Model.h"
#include "cppflow/Tensor.h"
#include <opencv2/opencv.hpp>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <thread>

#include <cppflowATRInterface/Object_Detection_API.h>
#include <cppflowATRInterface/Object_Detection_Types.h>
#include <utils/imgUtils.h>

using namespace std;
using namespace std::chrono;
using namespace OD;

vector<String> GetFileNames();
vector<String> GetFileNames1(const char *nm);

void MyWait(string s, float ms)
{
    std::cout << s << " Waiting sec:" << (ms / 1000.0) << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds((uint)ms));
}

int main()
{

    int numInf1 = 10;
    int numInf2 = 10;
    int numInf3 = 10;
    bool runInf4 = true;
    bool SHOW = false;
    float numIter = 3.0;
#ifdef TEST_MODE
    cout << "Test Mode" << endl;
#endif//TEST_MODE

#ifdef WIN32
    unsigned int W = 1292;
    unsigned int H = 969;
#else
    unsigned int W = 4096;
    unsigned int H = 2160;
#endif
    unsigned int frameID = 42;

    // Mission
    MB_Mission mission1 = {
        MB_MissionType::MATMON,       //mission1.missionType
        e_OD_TargetSubClass::PRIVATE, //mission1.targetClas
        e_OD_TargetColor::WHITE       //mission1.targetColor
    };

    // support data
    OD_SupportData supportData1 = {
        H, W,                     //imageHeight//imageWidth
        e_OD_ColorImageType::RGB, //colorType;
        100,                      //rangeInMeters
        70.0f,                    //fcameraAngle; //BE
        0,                        //TEMP:cameraParams[10];//BE
        0                         //TEMP: float	spare[3];
    };

    OD_InitParams initParams =
        {
    //(char*)"/home/magshim/MB2/TrainedModels/faster_MB_140719_persons_sel4/frozen_390k/frozen_inference_graph.pb", //fails
    //(char*)"tryTRT_humans.pb", //sometimes works
    //(char*)"/home/magshim/MB2/TrainedModels/MB3_persons_likeBest1_default/frozen_378K/frozen_inference_graph.pb", //works
//(char*)"tryTRT_humans.pb", //sometimes OK, sometimes crashes the system
#ifdef WIN32
            (char *)"graphs/ssd_resnet50_v1_fpn_shared_box_predictor_640x640_coco14_sync_2018_07_03_frozen_inference_graph.pb",
#else
            //works bad performance
            //(char *)"/home/borisef/projects/MB2/TrainedModels/ckpts_along_the_train_process/local_frozen_1_2M/frozen_inference_graph.pb", 
            // (char *)"graphs/frozen_inference_graph_all_size_4096x2160.pb",
            //(char*)"graphs/frozen_inference_graph_all_3040_4056.pb",
            (char*)"graphs/frozen_inference_graph_humans.pb",
#endif
            350, // max number of items to be returned
            supportData1,
            mission1};

    // Creation of ATR manager + new mission
    OD::ObjectDetectionManager *atrManager;
    atrManager = OD::CreateObjectDetector(&initParams); //first mission

    cout << " ***  ObjectDetectionManager created  *** " << endl;


    // new mission (RGB)
    if(numInf1 > 0)
        OD::InitObjectDetection(atrManager, &initParams);

    //emulate buffer from TIF
    cout << " ***  Read tif image to rgb buffer  ***  " << endl;

#ifdef WIN32
    cv::Mat inp1 = cv::imread("media/girl.jpg", CV_LOAD_IMAGE_COLOR);
#else
    #ifdef OPENCV_MAJOR_4
    cv::Mat inp1 = cv::imread("media/00000018.tif", IMREAD_COLOR);//CV_LOAD_IMAGE_COLOR
    #else
    cv::Mat inp1 = cv::imread("media/00000018.tif", CV_LOAD_IMAGE_COLOR);//
    #endif
#endif

    cv::cvtColor(inp1, inp1, CV_BGR2RGB);

    //put image in vector
    std::vector<uint8_t> img_data1(inp1.total() * inp1.channels());
    img_data1.assign(inp1.data, inp1.data + inp1.total() * inp1.channels());

    unsigned char *ptrTif = new unsigned char[img_data1.size()];
    std::copy(begin(img_data1), end(img_data1), ptrTif);

    OD_CycleInput *ci = new OD_CycleInput();
    // ci->ImgID_input = 42;
    ci->ptr = ptrTif;

    OD_ErrorCode statusCycle;

    OD_CycleOutput *co = new OD_CycleOutput(); // allocate empty cycle output buffer

    co->maxNumOfObjects = 350;
    co->ImgID_output = 0;
    co->numOfObjects = 0;
    co->ObjectsArr = new OD_DetectionItem[co->maxNumOfObjects];



    uint lastReadyFrame = 0;
    co->ImgID_output = 0;
    for (int i = 1; i < numInf1; i++)
    {
        ci->ImgID_input = 0 + i;
      

        
        cout << " ***  Run inference on RGB image  ***  step " << i << endl;

        statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);

        cout << " ***  .... After inference on RGB image   *** " << endl;

        if (lastReadyFrame != co->ImgID_output)
        { //draw
            cout << " Detected new results for frame " << co->ImgID_output << endl;
            string outName = "outRes/out_res1_" + std::to_string(co->ImgID_output) + ".png";
            lastReadyFrame = co->ImgID_output;
            atrManager->SaveResultsATRimage(co, (char *)outName.c_str(), true);
        }

         MyWait("Small pause", 1000.0);
    }

    //MyWait("Long pause", 1000.0);

    //release buffer
    delete ptrTif;

    W = 4056;
    H = 3040;
    frameID++;

    // change  support data
    OD_SupportData supportData2 = {
        H, W,                        //imageHeight//imageWidth
        e_OD_ColorImageType::YUV422, // colorType;
        100,                         //rangeInMeters
        70.0f,                       //fcameraAngle; //BE
        0,                           //TEMP:cameraParams[10];//BE
        0                            //TEMP: float	spare[3];
    };

    OD_InitParams initParams2(initParams);

    initParams2.supportData = supportData2;
    // new mission because of support data
    InitObjectDetection(atrManager, &initParams2);

    //emulate buffer from RAW
    std::vector<unsigned char> vecFromRaw = readBytesFromFile("media/00006160.raw");

    unsigned char *ptrRaw = new unsigned char[vecFromRaw.size()];
    std::copy(begin(vecFromRaw), end(vecFromRaw), ptrRaw);

    ci->ptr = ptrRaw; //use same ci and co

    lastReadyFrame = 0;
    co->ImgID_output = 0;

    for (int i = 1; i < numInf2; i++)
    {
        ci->ImgID_input = 0 + i;
        cout << " ***  Run inference on RAW image  ***  " << endl;

        statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);

        cout << " ***  .... After inference on RAW image   *** " << endl;

        if (lastReadyFrame != co->ImgID_output)
        { //draw
            cout << " Detected new results for frame " << co->ImgID_output << endl;
            string outName = "outRes/out_res2_" + std::to_string(co->ImgID_output) + ".png";
            lastReadyFrame = co->ImgID_output;
            atrManager->SaveResultsATRimage(co, (char *)outName.c_str(), true);
        }

        MyWait("Small pause", 100.0);
    }
    //atrManager->SaveResultsATRimage(ci, co, (char *)"out_res2.tif", false);
    // MyWait("Long pause", 1000.0);
    W = 3840;
    H = 2160;
#ifdef WIN32
    W = 1920;
    H = 1080;
#endif
    frameID++;

    // change  support data
    OD_SupportData supportData3 = {
        H, W,                     //imageHeight//imageWidth
        e_OD_ColorImageType::RGB, //colorType;
        100,                      //rangeInMeters
        70.0f,                    //fcameraAngle; //BE
        0,                        //TEMP:cameraParams[10];//BE
        0                         //TEMP: float	spare[3];
    };

    initParams.supportData = supportData3;
    // new mission because of support data
    if(numInf3 > 0)
        InitObjectDetection(atrManager, &initParams);

    vector<String> ff = GetFileNames();
    int N = ff.size();
    N = min(50, N);
    lastReadyFrame = 0;
    co->ImgID_output = 0;
    int temp = 0;
    int flag = 1;
    for (size_t i1 = 0; i1 < numInf3; i1++)
    {
        //  MyWait("**** Long pause in-between **** ", 1000.0);

        for (size_t i = 0; i < N; i++)
        {
            temp++;
            ci->ImgID_input = 0 + i + temp;

            ptrTif = ParseImage(ff[i]);
            ci->ptr = ptrTif;
            if (flag > 0)
            { //Check null ptrs
                ci->ptr = nullptr;
                cout << "!!!!!!!!!----------> !!!!!!!!!!!!  Test with ci->ptr = nullptr !!!!!!!!!!" << endl;
            }

            flag = -flag;

            statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);
            if (lastReadyFrame != co->ImgID_output)
            { //draw
                cout << " Detected new results for frame " << co->ImgID_output << endl;
                string outName = "outRes/out_res3_" + std::to_string(co->ImgID_output) + ".png";
                lastReadyFrame = co->ImgID_output;
                atrManager->SaveResultsATRimage(co, (char *)outName.c_str(), true);
            }
            // MyWait("Small pause", 10.0);
            delete ptrTif;
        }
    }

    //
    W = 4056;
    H = 3040;
    // change  support data
    OD_SupportData supportData4 = {
        H, W,                        //imageHeight//imageWidth
        e_OD_ColorImageType::YUV422, //colorType;
        100,                         //rangeInMeters
        70.0f,                       //fcameraAngle; //BE
        0,                           //TEMP:cameraParams[10];//BE
        0                            //TEMP: float	spare[3];
    };

    initParams.supportData = supportData4;
    // new mission because of support data
    if(runInf4)
        InitObjectDetection(atrManager, &initParams);

    ff = GetFileNames1("media/raw/*");
    N = ff.size() *((int)runInf4);
    lastReadyFrame = 0;
    co->ImgID_output = 0;

    for (size_t i = 0; i < N; i = i+1)
    {
        if(i%100==0)
            cout << "Run in folder,  frame " << i << endl;
        temp++;
        ci->ImgID_input = 0 + i + temp;

        ptrTif = (uchar*)fastParseRaw(ff[i]);
        ci->ptr = ptrTif;

        statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);
        if (lastReadyFrame != co->ImgID_output)
        { 
            cout << " Detected new results for frame " << co->ImgID_output << endl;
            string outName = "outRes/out_res3_" + std::to_string(co->ImgID_output) + ".png";
            lastReadyFrame = co->ImgID_output;
            atrManager->SaveResultsATRimage(co, (char *)outName.c_str(), true);
        }
        MyWait("Small pause", 100.0);
        delete ptrTif;
    }

    delete ptrRaw;

    //at the end
    OD::TerminateObjectDetection(atrManager);

    //release OD_CycleInput
    delete ci;

    //release OD_CycleOutput
    delete co->ObjectsArr;
    delete co;

    return 0;
}

vector<String> GetFileNames()
{

    vector<String> fn;
    cv::glob("media/spliced/*", fn, true);

    return fn;
}

vector<String> GetFileNames1(const char *nm)
{

    vector<String> fn;
    cv::glob(nm, fn, true);

    return fn;
}

