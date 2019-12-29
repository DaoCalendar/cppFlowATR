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


#include <cppflowATRInterface/Object_Detection_API.h>
#include <cppflowATRInterface/Object_Detection_Types.h>
#include <utils/imgUtils.h>



using namespace std; 
using namespace std::chrono;

int main() {

    bool SHOW = true;
    float numIter = 3.0;
    
    unsigned int W =  4096;
    unsigned int H =  2160;
    unsigned int frameID = 42;

    // Mission
    MB_Mission mission1 = {
        MB_MissionType::MATMON,       //mission1.missionType
        e_OD_TargetSubClass::PRIVATE, //mission1.targetClas
        e_OD_TargetColor::WHITE       //mission1.targetColor
    };

    // support data
    OD_SupportData supportData1 = {
        W, H,                        //imageHeight//imageWidth
        e_OD_ColorImageType::RGB,    //colorType;
        100,                         //rangeInMeters
        70.0f,                       //fcameraAngle; //BE
        0,                           //TEMP:cameraParams[10];//BE
        0                            //TEMP: float	spare[3];
    };

    OD_InitParams initParams1 = 
    {   
        //(char*)"/home/magshim/MB2/TrainedModels/faster_MB_140719_persons_sel4/frozen_390k/frozen_inference_graph.pb", //fails
        //(char*)"tryTRT_humans.pb", //sometimes works  
        //(char*)"/home/magshim/MB2/TrainedModels/MB3_persons_likeBest1_default/frozen_378K/frozen_inference_graph.pb", //works
       //(char*)"tryTRT_humans.pb", //sometimes OK, sometimes crashes the system
       (char*)"graphs/frozen_inference_graph_humans.pb",
       //  (char*)"tryTRT_all.pb", //Nope
       // (char*)"/home/magshim/cppflowATR/frozen_inference_graph_all.pb",
        100,                  // max number of items to be returned
        supportData1,
        mission1
    };

     // Creation of ATR manager + new mission
    OD::ObjectDetectionManager *atrManager;
    atrManager = OD::CreateObjectDetector(&initParams1); //first mission

    cout << " ***  ObjectDetectionManager created  *** " << endl;

    // new mission
    OD::InitObjectDetection(atrManager, &initParams1);
   

     //emulate buffer from TIF
    cout << " ***  Read tif image to rgb buffer  ***  " << endl;

    cv::Mat inp1 = cv::imread("media/00000018.tif", CV_LOAD_IMAGE_COLOR);
    cv::cvtColor(inp1, inp1, CV_BGR2RGB);

    //put image in vector
    std::vector<uint8_t> img_data1;
    img_data1.assign(inp1.data, inp1.data + inp1.total() * inp1.channels());
    
    unsigned char *ptrTif = new unsigned char[img_data1.size()];
    std::copy(begin(img_data1), end(img_data1), ptrTif);

    OD_CycleInput *ci = new OD_CycleInput();
    ci->ImgID_input = 42;
    ci->ptr = ptrTif;

    OD_CycleOutput *co = new OD_CycleOutput(); // allocate empty cycle output buffer
    OD_ErrorCode statusCycle;

    co->maxNumOfObjects = 300;
    co->ImgID_output = -1;
    co->numOfObjects = 300;
    co->ObjectsArr = new OD_DetectionItem[co->maxNumOfObjects];


    cout << " ***  Run inference on RGB image  ***  " << endl;

    statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);

    cout << " ***  .... After inference on RGB image   *** " << endl;


    // //draw
    atrManager->SaveResultsATRimage(ci, co, (char *)"out_res1.tif", false);

    //release buffer
    delete ptrTif;

    W = 4056;
    H = 3040;
    frameID++;

    // change  support data
    OD_SupportData supportData2 = {
        W, H,                        //imageHeight//imageWidth
        e_OD_ColorImageType::YUV422, // colorType;
        100,                         //rangeInMeters
        70.0f,                       //fcameraAngle; //BE
        0,                           //TEMP:cameraParams[10];//BE
        0                            //TEMP: float	spare[3];
    };

    initParams1.supportData = supportData2;
    // new mission because of support data
    InitObjectDetection(atrManager, &initParams1);

    //emulate buffer from RAW
    std::vector<unsigned char> vecFromRaw = readBytesFromFile("media/00006160.raw");

     
    unsigned char *ptrRaw = new unsigned char[vecFromRaw.size()];
    std::copy(begin(vecFromRaw), end(vecFromRaw), ptrRaw);

    ci->ptr = ptrRaw;//use same ci and co

    cout << " ***  Run inference on RAW image  ***  " << endl;

    statusCycle = OD::OperateObjectDetectionAPI(atrManager, ci, co);

    cout << " ***  .... After inference on RAW image   *** " << endl;

    atrManager->SaveResultsATRimage(ci, co, (char *)"out_res2.tif", false);

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
