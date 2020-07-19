#include <opencv2/opencv.hpp>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstdlib>

#include <cppflowATRInterface/Object_Detection_API.h>
#include <cppflowATRInterface/Object_Detection_Types.h>
#include <utils/imgUtils.h>
#include <utils/odUtils.h>

#include <cppflowATRInterface/Object_Detection_Handler.h>

using namespace std;
using namespace std::chrono;
using namespace OD;

void PrintCO(OD::OD_CycleOutput *co1)
{
    for (size_t i = 0; i < co1->numOfObjects; i++)
    {
        if (i == 0)
            cout << "-----------------------------" << endl;
        cout << "tarClass :" << co1->ObjectsArr[i].tarClass << endl;
        cout << "tarSubClass :" << co1->ObjectsArr[i].tarSubClass << endl;
        cout << "tarColor :" << co1->ObjectsArr[i].tarColor << endl;
        cout << "tarScore :" << co1->ObjectsArr[i].tarScore << endl;
        cout << "-     -     -      - " << endl;
        if (i == 0)
            cout << "-----------------------------" << endl;
    }
}

int main()
{

    const char *tiles[26] = {
        "media/tiles/004.jpg",
        "media/tiles/005.jpg",
        "media/tiles/006.jpg",
        "media/tiles/007.jpg",
        "media/tiles/008.jpg",
        "media/tiles/009.jpg",
        "media/tiles/010.jpg",
        "media/tiles/011.jpg",
        "media/tiles/012.jpg",
        "media/tiles/013.jpg",
        "media/tiles/014.jpg",
        "media/tiles/015.jpg",
        "media/tiles/016.jpg",
        "media/tiles/017.jpg",
        "media/tiles/018.jpg",
        "media/tiles/019.jpg",
        "media/tiles/020.jpg",
        "media/gzir/gzir007.jpg",
        "media/gzir/gzir002.jpg",
        "media/gzir/gzir003.jpg",
        "media/gzir/gzir004.jpg",
        "media/gzir/gzir005.jpg",
        "media/gzir/gzir006.jpg",
        "media/gzir/gzir001.jpg",
        "media/gzir/gzir008.jpg",
        "media/gzir/gzir009.jpg"};

#ifdef WIN32
    string iniFile = (char *)"config/configATR_May2020_win.json";
#elif OS_LINUX
#ifdef JETSON
    string iniFile = (char *)"config/configATR_June2020_linux_jetson.json";
#else
    string iniFile = (char *)"config/configATR_June2020_linux.json";
#endif
#endif

    // Mission
    MB_Mission mission = {
        MB_MissionType::ANALYZE_SAMPLE,         //mission1.missionType
        e_OD_TargetClass::UNKNOWN_CLASS,        //mission1.targetSubClass
        e_OD_TargetSubClass::UNKNOWN_SUB_CLASS, //mission1.targetSubClass
        e_OD_TargetColor::UNKNOWN_COLOR         //mission1.targetColor
    };
    // support data
    OD_SupportData supportData = {
        0, 0,                              //imageHeight//imageWidth
        e_OD_ColorImageType::RGB_IMG_PATH, //colorType;
        0,                                 //rangeInMeters
        70.0f,                             //fcameraAngle; //BE
        0,                                 //TEMP:cameraParams[10];//BE
        0                                  //TEMP: float	spare[3];
    };

    OD_InitParams initParamsSamples =
        {
            (char *)iniFile.c_str(),
            350, // max number of items to be returned
            supportData,
            mission};

    // Creation of ATR manager + new mission
    OD::ObjectDetectionManager *atrManagerSamples = OD::CreateObjectDetector(&initParamsSamples); //first mission
    OD::InitObjectDetection(atrManagerSamples, &initParamsSamples);

    // Cycles
    OD_CycleInput *ci = new OD_CycleInput();
    OD_CycleOutput *co = NewOD_CycleOutput(350); // allocate empty cycle output buffer

    OD_ErrorCode statusCycle;

    // Do it several times
    for (size_t sample = 0; sample < 2; sample++)
    {
        ci->ptr = (const unsigned char *)tiles[sample];
        statusCycle = OD::OperateObjectDetectionAPI(atrManagerSamples, ci, co);
        PrintCO(co);
    }

    //stress test
    int stressN = 5; // 100
    for (size_t i = 0; i < stressN; i++)
    {
        ci->ptr = (const unsigned char *)tiles[2];
        statusCycle = OD::OperateObjectDetectionAPI(atrManagerSamples, ci, co);
        PrintCO(co);
    }

    // clean
    delete ci;
    //delete co->ObjectsArr;
    delete co;

    return 0;
}
