//
// 1) Run on thousands of images with different delays
// 2) Create and destroy managers
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

unsigned char *ParseImage(String path);
vector<String> GetFileNames();

void MyWait(string s, float ms)
{
    std::cout << s << " Waiting sec:" << (ms / 1000.0) << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds((uint)ms));
}

vector<String> GetFileNames(const char *pa)
{

    vector<String> fn;
    cv::glob(pa, fn, true);

    return fn;
}

unsigned char *ParseImage(String path)
{
    cv::Mat inp1 = cv::imread(path, CV_LOAD_IMAGE_COLOR);
    cv::cvtColor(inp1, inp1, CV_BGR2RGB);

    //put image in vector
    std::vector<uint8_t> img_data1(inp1.rows * inp1.cols * inp1.channels());
    img_data1.assign(inp1.data, inp1.data + inp1.total() * inp1.channels());

    unsigned char *ptrTif = new unsigned char[img_data1.size()];
    std::copy(begin(img_data1), end(img_data1), ptrTif);

    return ptrTif;
}

unsigned char *ParseRaw(String path)
{

    //emulate buffer from RAW
    std::vector<unsigned char> vecFromRaw = readBytesFromFile((char *)path.c_str());

    unsigned char *ptrRaw = new unsigned char[vecFromRaw.size()];
    std::copy(begin(vecFromRaw), end(vecFromRaw), ptrRaw);

    return ptrRaw;
}

struct OneRunStruct
{
    int H;
    int W;
    string splicePath;
    int numRepetiotions = 10;
    float minDelay = 0;
    bool toShow = true;
    e_OD_ColorImageType imType = e_OD_ColorImageType::RGB;

#ifdef WIN32
    string graph = (char *)"graphs/ssd_resnet50_v1_fpn_shared_box_predictor_640x640_coco14_sync_2018_07_03_frozen_inference_graph.pb";
#else
    string graph = (char *)"graphs/frozen_inference_graph_humans.pb";
#endif

    bool toDeleteATRM = true;
    int startFrameID = 1;
};

void OneRun(OD::ObjectDetectionManager *atrManager, OneRunStruct ors)
{
    // Mission
    MB_Mission mission = {
        MB_MissionType::MATMON,       //mission1.missionType
        e_OD_TargetSubClass::PRIVATE, //mission1.targetClas
        e_OD_TargetColor::WHITE       //mission1.targetColor
    };

    // support data
    OD_SupportData supportData = {
        ors.H, ors.W, //imageHeight//imageWidth
        ors.imType,   //colorType;
        100,          //rangeInMeters
        70.0f,        //fcameraAngle; //BE
        0,            //TEMP:cameraParams[10];//BE
        0             //TEMP: float	spare[3];
    };

    OD_InitParams initParams =
        {
            (char *)ors.graph.c_str(),
            350, // max number of items to be returned
            supportData,
            mission};

    // // Creation of ATR manager + new mission
    if (!atrManager)
        atrManager = OD::CreateObjectDetector(&initParams); //first mission

    cout << " ***  ObjectDetectionManager created  *** " << endl;

    // // new mission
    OD::InitObjectDetection(atrManager, &initParams);

    OD_CycleInput *ci = new OD_CycleInput();
    OD_CycleOutput *co = new OD_CycleOutput(); // allocate empty cycle output buffer
    OD_ErrorCode statusCycle;
    co->maxNumOfObjects = 350;
    co->ImgID_output = 0;
    co->numOfObjects = 0;
    co->ObjectsArr = new OD_DetectionItem[co->maxNumOfObjects];

    vector<String> ff = GetFileNames((char *)ors.splicePath.c_str());
    int N = ff.size();
    unsigned char *ptrTif;
    int lastReadyFrame = 0;
    co->ImgID_output = 0;
    int temp = 0;
    for (size_t i1 = 0; i1 < ors.numRepetiotions; i1++)
    {
        for (size_t i = 0; i < N; i++)
        {
            temp++;
            ci->ImgID_input = temp + ors.startFrameID;
            if (ors.imType == e_OD_ColorImageType::RGB)
                ptrTif = ParseImage(ff[i]);
            else
            {
                ptrTif = ParseRaw(ff[i]);
            }

            ci->ptr = ptrTif;
            statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);
            if (lastReadyFrame != co->ImgID_output)
            { //draw
                cout << " Detected new results for frame " << co->ImgID_output << endl;
                string outName = "outRes/out_res3_" + std::to_string(co->ImgID_output) + ".png";
                lastReadyFrame = co->ImgID_output;
                atrManager->SaveResultsATRimage(co, (char *)outName.c_str(), ors.toShow);
            }
            MyWait("Small pause", ors.minDelay);
            delete ptrTif;
        }
    }
    //at the end
    OD::TerminateObjectDetection(atrManager);
    atrManager = nullptr;
    //release OD_CycleInput
    delete ci;

    //release OD_CycleOutput
    delete co->ObjectsArr;
    delete co;
}

int main()
{
    OD::ObjectDetectionManager *atrManager = nullptr;

    OneRunStruct ors4;
    ors4.W = 4056;
    ors4.H = 3040;
    ors4.splicePath = "media/raw/*";
    ors4.imType = e_OD_ColorImageType::YUV422;
    ors4.numRepetiotions = 400;
    ors4.minDelay = 50;
    ors4.startFrameID = 100000;
    OneRun(atrManager, ors4);

    OneRunStruct ors1;
    ors1.H = 1080;
    ors1.W = 1920;
    ors1.splicePath = "media/spliced/*";
    ors1.numRepetiotions = 2;
    ors1.minDelay = 10;
    ors1.startFrameID = 1;
    OneRun(atrManager, ors1);

    OneRunStruct ors2;
    ors2.H = 1071;
    ors2.W = 1904;
    ors2.splicePath = "media/filter/*";
    ors2.numRepetiotions = 30;
    ors2.minDelay = 0;
    ors2.startFrameID = 1000;
    OneRun(atrManager, ors2);

    OneRunStruct ors3;
    ors3.W = 2393;
    ors3.H = 1867;
    ors3.splicePath = "media/filterUCLA/*";
    ors3.numRepetiotions = 30;
    ors3.minDelay = 0;
    ors3.startFrameID = 100000;
    OneRun(atrManager, ors3);

    return 0;
}