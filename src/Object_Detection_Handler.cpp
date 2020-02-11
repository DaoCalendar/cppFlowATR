#include <cppflowATRInterface/Object_Detection_Handler.h>
#include <utils/imgUtils.h>
#include <utils/odUtils.h>

#include <iomanip>
#include <future>

using namespace OD;
using namespace std;

mbInterfaceCM *ObjectDetectionManagerHandler::m_mbCM = nullptr;

OD_CycleInput *NewCopyCycleInput(OD_CycleInput *tocopy, uint bufferSize)
{
    OD_CycleInput *newCopy = new OD_CycleInput();
    newCopy->ImgID_input = tocopy->ImgID_input;
    //copy buffer
    unsigned char *buffer = new unsigned char[bufferSize];
    memcpy(buffer, tocopy->ptr, bufferSize);
    newCopy->ptr = buffer;
    return newCopy;
}

OD_CycleInput *SafeNewCopyCycleInput(OD_CycleInput *tocopy, uint bufferSize)
{
    if (!tocopy)
        return nullptr;
    OD_CycleInput *newCopy = new OD_CycleInput();
    newCopy->ImgID_input = tocopy->ImgID_input;
    //copy buffer
    if (tocopy->ptr)
    {
        unsigned char *buffer = new unsigned char[bufferSize];
        memcpy(buffer, tocopy->ptr, bufferSize);
        newCopy->ptr = buffer;
    }
    else
    {
        newCopy->ptr = nullptr;
    }

    return newCopy;
}

void DeleteCycleInput(OD_CycleInput *todel)
{
    if (todel)
    {
        if (todel->ptr)
            delete todel->ptr;
        delete todel;
    }
}

OD_InitParams *ObjectDetectionManagerHandler::getParams() { return m_initParams; }

void ObjectDetectionManagerHandler::setParams(OD_InitParams *ip)
{
    m_initParams = ip;

    m_numPtrPixels = ip->supportData.imageHeight * ip->supportData.imageWidth;
    m_numImgPixels = ip->supportData.imageHeight * ip->supportData.imageWidth * 3;
    if (ip->supportData.colorType == e_OD_ColorImageType::YUV422)
        m_numPtrPixels = m_numPtrPixels * 2;
    else
        m_numPtrPixels = m_numPtrPixels * 3;
}

bool ObjectDetectionManagerHandler::IsBusy()
{
    if (m_result.valid())
    {
        bool temp = !(m_result.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        if (temp)
            cout << " IsBusy? Yes" << endl;
        else
        {
            cout << " IsBusy? No" << endl;
        }

        return !(m_result.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    }
    else
    {
        cout << " IsBusy? Not valid" << endl;
        return false;
    }
}

ObjectDetectionManagerHandler::ObjectDetectionManagerHandler()
{
    m_mbATR = nullptr;
    m_prevCycleInput = nullptr;
    m_nextCycleInput = nullptr;
    m_curCycleInput = nullptr;
}
ObjectDetectionManagerHandler::ObjectDetectionManagerHandler(OD_InitParams *ip) : ObjectDetectionManagerHandler()
{
    setParams(ip);
}

ObjectDetectionManagerHandler::~ObjectDetectionManagerHandler()
{
    WaitForThread();
    DeleteAllInnerCycleInputs();

    if (m_mbATR != nullptr)
        delete m_mbATR;
}

bool ObjectDetectionManagerHandler::WaitForThread()
{
    if (m_result.valid())
    {
        std::cout << "waiting...\n";
        m_result.wait();
        std::cout << "Done!\n";
    }
    else
    {
        cout << "Thread is still not valid" << endl;
    }

    return true;
}

bool ObjectDetectionManagerHandler::WaitUntilForThread(int sec)
{
    if (m_result.valid())
    {
        std::chrono::system_clock::time_point few_seconds_passed = std::chrono::system_clock::now() + std::chrono::seconds(sec);

        if (std::future_status::ready == m_result.wait_until(few_seconds_passed))
        {
            std::cout << "times_out "
                      << "\n";
        }
        else
        {
            std::cout << "did not complete!\n";
        }

        std::cout << "Done!\n";
    }
    else
    {
        cout << "Thread is still not valid" << endl;
    }

    return true;
}
void ObjectDetectionManagerHandler::DeleteAllInnerCycleInputs()
{
    if (m_nextCycleInput)
    {
        DeleteCycleInput(m_nextCycleInput);
        m_nextCycleInput = nullptr;
    }
    if (m_curCycleInput)
    {
        DeleteCycleInput(m_curCycleInput);
        m_curCycleInput = nullptr;
    }
    if (m_prevCycleInput)
    {
        DeleteCycleInput(m_prevCycleInput);
        m_prevCycleInput = nullptr;
    }
}

OD_ErrorCode ObjectDetectionManagerHandler::InitObjectDetection(OD_InitParams *odInitParams)
{
    //check if busy, if yes wait till the end and assign nullptr to next, current and prev
    WaitForThread();
    DeleteAllInnerCycleInputs();

    mbInterfaceATR *mbATR = nullptr;
    //initialization
    if (m_mbATR == nullptr)
    {
        mbATR = new mbInterfaceATR();
        cout << "Create new mbInterfaceATR in ObjectDetectionManagerHandler::InitObjectDetection" << endl;
        //TODO: decide which model to take (if to take or stay with old )
        mbATR->LoadNewModel(odInitParams->iniFilePath);
        m_mbATR = mbATR;
        cout << "Executed LoadNewModel in  InitObjectDetection" << endl;
    }
    if (odInitParams->supportData.colorType == e_OD_ColorImageType::RGB_IMG_PATH)
    {
        odInitParams->supportData.imageHeight = 2000;
        odInitParams->supportData.imageWidth = 4000;
    }
    //create CM
    bool initCMsuccess = true;
    m_withActiveCM = true; 

    if (m_mbCM == nullptr && m_withActiveCM)
    {
        initCMsuccess = InitCM(odInitParams->iniFilePath);
        m_withActiveCM = initCMsuccess; 
    }

    setParams(odInitParams);

    IdleRun();
    if (m_mbCM != nullptr && m_withActiveCM)
        m_mbCM->IdleRun();
    // TODO: check if really OK 
    return OD_ErrorCode::OD_OK;
}
OD_ErrorCode ObjectDetectionManagerHandler::PrepareOperateObjectDetection(OD_CycleInput *cycleInput)
{
    //keep OD_CycleInput copy
    cout << " PrepareOperateObjectDetection: Prepare to run on frame " << cycleInput->ImgID_input << endl;
    if (cycleInput && cycleInput->ptr) // input is valid
    {

        OD_CycleInput *tempCycleInput = nullptr;
        cout << "Replace old next cycle input" << endl;
        m_mutexOnNext.lock();
        //glob_mutexOnNext.lock();
        if (m_nextCycleInput) //my next not null
        {
            if (cycleInput->ImgID_input != m_nextCycleInput->ImgID_input)
            { // not same frame#
                //replace next
                tempCycleInput = m_nextCycleInput;

                m_nextCycleInput = NewCopyCycleInput(cycleInput, m_numPtrPixels);

                DeleteCycleInput(tempCycleInput); //delete old "next"
            }
            else // same next frame
            {
                cout << "PrepareOperateObjectDetection: attempt to call with same frame twice, skipping" << endl;
            }
        }
        else
        { // m_nextCycleInput is null, safe to create it
            m_nextCycleInput = NewCopyCycleInput(cycleInput, m_numPtrPixels);
        }
        m_mutexOnNext.unlock();
        //glob_mutexOnNext.unlock();
    }
    else
    {
        cout << "PrepareOperateObjectDetection:Input cycle is null, skip " << endl;
    }
    if (m_nextCycleInput && m_nextCycleInput->ptr)
        cout << "PrepareOperateObjectDetection:Next cycle is valid" << endl;
    else
        cout << "PrepareOperateObjectDetection:Next cycle is empty" << endl;

    return OD_ErrorCode::OD_OK;
}

void ObjectDetectionManagerHandler::IdleRun()
{
    cout << " ObjectDetectionManagerHandler::Idle Run (on neutral)" << endl;

    //TODO: is it continues in memory ? 
   // unsigned char *tempPtr = new unsigned char[m_numImgPixels];
    std::vector<uint8_t> *img_data = new std::vector<uint8_t>(m_numImgPixels);//try
    //create temp ptr
   // this->m_mbATR->RunRGBVector(tempPtr, this->m_initParams->supportData.imageHeight, this->m_initParams->supportData.imageWidth);
    
    //try
    this->m_mbATR->RunRGBVector(*img_data,this->m_initParams->supportData.imageHeight, this->m_initParams->supportData.imageWidth);
    //delete tempPtr;
    delete img_data;//try


}

OD_ErrorCode ObjectDetectionManagerHandler::OperateObjectDetection(OD_CycleOutput *odOut)
{

    cout << "^^^Locked" << endl;

    if (m_nextCycleInput && m_nextCycleInput->ptr)
        cout << "OperateObjectDetection:Next cycle is valid" << endl;
    else
        cout << "OperateObjectDetection:Next cycle is empty" << endl;

    if (m_curCycleInput && m_curCycleInput->ptr)
        cout << "OperateObjectDetection:Current cycle is valid" << endl;
    else
        cout << "OperateObjectDetection:Current cycle is empty" << endl;

    if (m_prevCycleInput && m_prevCycleInput->ptr)
        cout << "OperateObjectDetection:Previous cycle is valid" << endl;
    else
        cout << "OperateObjectDetection:Previous cycle is empty" << endl;

    if (m_curCycleInput) // never suppose to happen, jic
    {
        cout << "This was never  supposed to happen... m_curCycleInput is not empty... How? " << endl;
        DeleteCycleInput(m_curCycleInput);
        m_curCycleInput = nullptr;
        return OD_ErrorCode::OD_FAILURE;
    }
    // deep copy next into current, next is not null here
    m_mutexOnNext.lock();
    // glob_mutexOnNext.lock();
    m_curCycleInput = SafeNewCopyCycleInput(m_nextCycleInput, m_numPtrPixels); // safe take care of NULL
    DeleteCycleInput(m_nextCycleInput);                                        //just to allow recursive call of OperateObjectDetection
    m_nextCycleInput = nullptr;
    m_mutexOnNext.unlock();
    //glob_mutexOnNext.unlock();

    if (!(m_curCycleInput && m_curCycleInput->ptr)) // former next (current ) cycle or buffer is null
    {
        cout << "ObjectDetectionManagerHandler::OperateObjectDetection nothing todo" << endl;
        cout << "###UnLocked" << endl;
        DeleteCycleInput(m_curCycleInput);
        m_curCycleInput = nullptr;

        return OD_ErrorCode::OD_OK;
    }

    cout << " OperateObjectDetection: Run on frame " << m_curCycleInput->ImgID_input << endl;

    unsigned int fi = m_curCycleInput->ImgID_input;
    int h = m_initParams->supportData.imageHeight;
    int w = m_initParams->supportData.imageWidth;

    e_OD_ColorImageType colortype = m_initParams->supportData.colorType;

    if (colortype == e_OD_ColorImageType::YUV422) // if raw
        //this->m_mbATR->RunRawImage(m_curCycleInput->ptr, h, w);
        this->m_mbATR->RunRawImageFast(m_curCycleInput->ptr, h, w);
        
    else if (colortype == e_OD_ColorImageType::RGB) // if rgb
    {
        cout << " Internal Run on RGB buffer " << endl;
        this->m_mbATR->RunRGBVector(m_curCycleInput->ptr, h, w);
    }
    else if (colortype == e_OD_ColorImageType::RGB_IMG_PATH) //path
    {
        cout << " Internal Run on RGB_IMG_PATH " << endl;
        this->m_mbATR->RunRGBImgPath(m_curCycleInput->ptr);
    }
    else
    {
        return OD_ErrorCode::OD_ILEGAL_INPUT;
    }
    cout << " ObjectDetectionManagerHandler::OperateObjectDetection starts PopulateCycleOutput  " << endl;
    // save results
    this->PopulateCycleOutput(odOut);

    //CM
    if(odOut->numOfObjects>0 && m_withActiveCM)
        std::vector<float> vecScoresAll = m_mbCM->RunImgWithCycleOutput(m_mbATR->GetKeepImg(), odOut, 0, (odOut->numOfObjects -1), true);

    odOut->ImgID_output = fi;

    // copy current into prev
    cout << "ObjectDetectionManagerHandler::OperateObjectDetection m_prevCycleInput<=m_curCycleInput<=nullptr" << endl;
    OD_CycleInput *tempCI = m_prevCycleInput;
    m_prevCycleInput = m_curCycleInput; // transfere

    m_mutexOnPrev.lock();
    //glob_mutexOnPrev.lock();
    DeleteCycleInput(tempCI);
    m_mutexOnPrev.unlock();
    //glob_mutexOnPrev.unlock();

    m_curCycleInput = nullptr;

    // string outName = "outRes/out_res3_" + std::to_string(odOut->ImgID_output) + "_in.png";
    // this->SaveResultsATRimage(nullptr,odOut, (char*)outName.c_str(),false);

    cout << "###UnLocked" << endl;

    //Recoursive call
    if (m_nextCycleInput)
    {
        cout << "+++++++++++++++++++++++++++++++++Call OperateObjectDetection again automatically" << endl;
        OperateObjectDetection(odOut);
    }
    return OD_ErrorCode::OD_OK;
}

bool ObjectDetectionManagerHandler::SaveResultsATRimage(OD_CycleOutput *co, char *imgNam, bool show)
{
    OD_CycleInput *tempci = nullptr;
    m_mutexOnPrev.lock();
    //glob_mutexOnPrev.lock();
    if (!(m_prevCycleInput && m_prevCycleInput->ptr))
    {
        cout << "No m_prevCycleInput data, skipping" << endl;
        return false;
    }
    else
    {
        // deep copy m_prevCycleInput
        tempci = NewCopyCycleInput(m_prevCycleInput, this->m_numPtrPixels);
    }
    m_mutexOnPrev.unlock();
    //glob_mutexOnPrev.unlock();

    float drawThresh = 0; //if 0 draw all
    //TODO: make sure m_prevCycleInput->ImgID_input is like co->ImgID
    unsigned int fi = tempci->ImgID_input;
    unsigned int h = m_initParams->supportData.imageHeight;
    unsigned int w = m_initParams->supportData.imageWidth;

    e_OD_ColorImageType colortype = m_initParams->supportData.colorType;
    cv::Mat *myRGB = nullptr;
    unsigned char *buffer = (unsigned char *)(tempci->ptr);

    if (colortype == e_OD_ColorImageType::YUV422) // if raw
    {
        std::vector<uint8_t> img_data(h * w * 2);
        for (int i = 0; i < h * w * 2; i++) //TODO: without for loop
            img_data[i] = buffer[i];

        myRGB = new cv::Mat(h, w, CV_8UC3);
        convertYUV420toRGB(img_data, w, h, myRGB);
    }
    else if (colortype == e_OD_ColorImageType::RGB || colortype == e_OD_ColorImageType::RGB_IMG_PATH) // if rgb
    {
        myRGB = new cv::Mat(h, w, CV_8UC3);
        //myRGB->data = buffer;// NOT safe if we going to use buffer later
        std::copy(buffer, buffer + w * h * 3, myRGB->data);
        //std::copy(arrayOne, arrayOne+10, arrayTwo);
        cv::imwrite("debug_newImg1_bgr.tif", *myRGB);
    }
    else
    {
        DeleteCycleInput(tempci);
        return false;
    }

    std::cout << "***** num_detections " << co->numOfObjects << std::endl;
    for (int i = 0; i < co->numOfObjects; i++)
    {
        int classId = co->ObjectsArr[i].tarClass;
        OD::e_OD_TargetColor colorId = co->ObjectsArr[i].tarColor;
        float score = co->ObjectsArr[i].tarScore;
        OD_BoundingBox bbox_data = co->ObjectsArr[i].tarBoundingBox;

        std::vector<float> bbox = {bbox_data.x1, bbox_data.x2, bbox_data.y1, bbox_data.y2};

        if (score >= drawThresh)
        {
            cout << "add rectangle to drawing" << endl;
            float x = bbox_data.x1;
            float y = bbox_data.y1;
            float right = bbox_data.x2;
            float bottom = bbox_data.y2;

            cv::rectangle(*myRGB, {(int)x, (int)y}, {(int)right, (int)bottom}, {125, 255, 51}, 2);
            cv::Scalar tColor(124, 200, 10);
            tColor = GetColor2Draw(colorId);

            cv::putText(*myRGB, string("Label:") + std::to_string(classId) + ";" + std::to_string(int(score * 100)) + "%", cv::Point(x, y - 10), 1, 2, tColor, 3);
        }
    }
    cout << " Done reading targets" << endl;
    if (show)
    {
        cv::Mat imgS;
        cv::resize(*myRGB, imgS, cv::Size(1365, 720));
        cv::imshow("Image", imgS);

        char c = (char)cv::waitKey(25);

        // cv::waitKey(0);
    }
    cv::Mat bgr(h, w, CV_8UC3);
    cv::cvtColor(*myRGB, bgr, cv::COLOR_RGB2BGR);
    cv::imwrite(imgNam, bgr);
    cout << " Done saving image" << endl;
    if (myRGB != nullptr)
    {
        myRGB->release();
        delete myRGB;
    }
    cout << " Done cleaning image" << endl;
    DeleteCycleInput(tempci);
    return true;
}

int ObjectDetectionManagerHandler::PopulateCycleOutput(OD_CycleOutput *cycleOutput)
{
    float LOWER_SCORE_THRESHOLD = 0.1;
    cout << "ObjectDetectionManagerHandler::PopulateCycleOutput" << endl;

    OD_DetectionItem *odi = cycleOutput->ObjectsArr;

    int N = m_mbATR->GetResultNumDetections();

    cout << "PopulateCycleOutput: Num detections total " << N << endl;

    auto bbox_data = m_mbATR->GetResultBoxes();
    unsigned int w = this->m_initParams->supportData.imageWidth;
    unsigned int h = this->m_initParams->supportData.imageHeight;

    //cycleOutput->numOfObjects = cycleOutput->maxNumOfObjects;
    cycleOutput->numOfObjects = N;
    for (int i = 0; i < N; i++)
    {
        e_OD_TargetClass aa = e_OD_TargetClass(1);
        odi[i].tarClass = e_OD_TargetClass(m_mbATR->GetResultClasses(i));
        odi[i].tarScore = m_mbATR->GetResultScores(i);
        if (odi[i].tarScore < LOWER_SCORE_THRESHOLD)
        {
            cycleOutput->numOfObjects = i;
            cout << "Taking only " << cycleOutput->numOfObjects << endl;
            break;
        }

        odi[i].tarBoundingBox = {bbox_data[i * 4 + 1] * w, bbox_data[i * 4 + 3] * w, bbox_data[i * 4] * h, bbox_data[i * 4 + 2] * h};
    }

    return cycleOutput->numOfObjects;
}

OD_ErrorCode ObjectDetectionManagerHandler::OperateObjectDetectionOnTiledSample(OD_CycleInput *cycleInput, OD_CycleOutput *cycleOutput)
{
    cycleOutput->numOfObjects = 0;

    uint bigH = m_initParams->supportData.imageHeight;
    uint bigW = m_initParams->supportData.imageWidth;

    const char *imgName = (const char *)cycleInput->ptr;

    //create tiled image
    cv::Mat *bigIm = new cv::Mat(bigH, bigW, CV_8UC3);
    bigIm->setTo(Scalar(0, 0, 0));

    std::list<float *> *tarList = new list<float *>(0);

    CreateTiledImage(imgName, bigW, bigH, bigIm, tarList);

    // cv::imwrite("smallImg.tif",cv::imread(imgName));
    cv::imwrite("bigImg.tif",*bigIm);

    unsigned char *ptrTif = ParseCvMat(*bigIm); // has new inside
    //run operate part without sync stuff etc.
    cout << " Internal Run on RGB buffer " << endl;
    this->m_mbATR->RunRGBVector(ptrTif, bigH, bigW);

    OD_CycleOutput *tempCycleOutput = NewOD_CycleOutput(350);
    this->PopulateCycleOutput(tempCycleOutput);

    // color 
    if(m_withActiveCM && m_mbCM!=nullptr && tempCycleOutput->numOfObjects>0 )
        this->m_mbCM->RunImgWithCycleOutput(*bigIm,tempCycleOutput,0,tempCycleOutput->numOfObjects-1,true);


    //DEBUG
    m_prevCycleInput = new OD_CycleInput();
    m_prevCycleInput->ptr = ptrTif;

    SaveResultsATRimage(tempCycleOutput, (char *)"tiles1.png", false);

    // analyze results and populate output
    AnalyzeTiledSample(tempCycleOutput, tarList, cycleOutput);

    // clean
    bigIm->release();
    delete bigIm;
    std::list<float *>::iterator it;
    for (it = tarList->begin(); it != tarList->end(); ++it)
        delete (*it);
    delete tarList;
    delete ptrTif;
    delete tempCycleOutput->ObjectsArr;
    delete tempCycleOutput;

    //TODO: take care of nothing detected
    return OD_ErrorCode::OD_OK;
}
int ObjectDetectionManagerHandler::CleanWrongTileDetections(OD_CycleOutput *co1, std::list<float *> *tarList)
{
    int numRemoved = 0;
    float thresh = 0.01;
    int numTrueTargets = tarList->size();
    float objBB[4];

    for (size_t d = 0; d < co1->numOfObjects; d++)
    {
        //object bb
        objBB[0] = co1->ObjectsArr[d].tarBoundingBox.x1;
        objBB[2] = co1->ObjectsArr[d].tarBoundingBox.x2;
        objBB[1] = co1->ObjectsArr[d].tarBoundingBox.y1;
        objBB[3] = co1->ObjectsArr[d].tarBoundingBox.y2;
        bool foundTarget = false;

        std::list<float *>::iterator it;
        for (it = tarList->begin(); it != tarList->end(); ++it)
        {
            //target bb
            float *targetBB = *it;
            float iou = IoU(targetBB, objBB);
            if (iou > thresh) //found target
            {
                foundTarget = 1;
                break;
            }
        }
        if (!foundTarget)
        { //TODO: remove object
            co1->ObjectsArr[d].tarScore = 0;
            numRemoved++;
        }
    }
    return numRemoved;
}
void ObjectDetectionManagerHandler::AnalyzeTiledSample(OD_CycleOutput *co1, std::list<float *> *tarList, OD_CycleOutput *co2)
{
    int MAX_TILES_CONSIDER = 3;

    // make sure co1->ObjectsArr[i] is one of tarList[j] by IoU
    int nr = CleanWrongTileDetections(co1, tarList);
    cout << " CleanWrongTileDetections removed " << nr << " objects" << endl;
    co1->numOfObjects = co1->numOfObjects - nr;
    int co1NumOfObjectsWithSkips = co1->numOfObjects + nr;

    for (size_t i = 0; i < co1NumOfObjectsWithSkips; i++)
    {
        // cout << co1->ObjectsArr[i].tarClass << endl;
        // cout << co1->ObjectsArr[i].tarSubClass << endl;
        // cout << co1->ObjectsArr[i].tarColor << endl;
        // cout << co1->ObjectsArr[i].tarScore << endl;
        if (co1->ObjectsArr[i].tarScore < 0.2)
            continue;

        // if already exists increment score
        int targetSlot = co2->numOfObjects;
        for (size_t i1 = 0; i1 < co2->numOfObjects; i1++)
        {
            if (co1->ObjectsArr[i].tarClass == co2->ObjectsArr[i1].tarClass)
                if (co1->ObjectsArr[i].tarSubClass == co2->ObjectsArr[i1].tarSubClass)
                    if (co1->ObjectsArr[i].tarColor == co2->ObjectsArr[i1].tarColor)
                    {
                        targetSlot = i1;
                        break;
                    }
        }

        // if not add element co2->numOfObjects, co2->numOfObjects++
        if (targetSlot == co2->numOfObjects)
        {
            if (co2->maxNumOfObjects <= co2->numOfObjects) //jic
                continue;
            co2->numOfObjects = co2->numOfObjects + 1;
            co2->ObjectsArr[targetSlot].tarScore = 0;
        }
        co2->ObjectsArr[targetSlot].tarClass = co1->ObjectsArr[i].tarClass;
        co2->ObjectsArr[targetSlot].tarSubClass = co1->ObjectsArr[i].tarSubClass;
        co2->ObjectsArr[targetSlot].tarColor = co1->ObjectsArr[i].tarColor;
        co2->ObjectsArr[targetSlot].tarScore += 1.0 / (co1->numOfObjects + 0.000001);
    }

    // sort co2->ObjectsArr[i2] by score
    bubbleSort_OD_DetectionItem(co2->ObjectsArr, co2->numOfObjects);

    // trim num objects
    if (co2->numOfObjects > MAX_TILES_CONSIDER)
        co2->numOfObjects = MAX_TILES_CONSIDER;

    //re-normalize score
    float totalScores = 0;
    for (size_t i2 = 0; i2 < co2->numOfObjects; i2++)
        totalScores = totalScores + co2->ObjectsArr[i2].tarScore;
    for (size_t i2 = 0; i2 < co2->numOfObjects; i2++)
        co2->ObjectsArr[i2].tarScore = co2->ObjectsArr[i2].tarScore / (totalScores + 0.00001);

    //TODO: compute scores based on Binomial distribution
}

bool ObjectDetectionManagerHandler::InitCM(const char* iniFilePath)
{
    const char *modelPath = "graphs/output_graph.pb";
    const char *ckpt = nullptr;
    const char *inname = "conv2d_input";
    const char *outname = "dense_1/Softmax";

    //check file exist
    if(!file_exists_test(modelPath))
        return false;

    m_mbCM = new mbInterfaceCM();
    if (!m_mbCM->LoadNewModel(modelPath, ckpt, inname, outname))
    {
        std::cout << "ooops" << std::endl;
        return false;
    }
    return true;
}