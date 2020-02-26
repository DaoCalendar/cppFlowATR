#include "cppflowCM/InterfaceCM.h"
#include <utils/imgUtils.h>
#include <utils/odUtils.h>
#include <iostream>

//constructors/destructors
mbInterfaceCM::mbInterfaceCM()
{
#ifdef TEST_MODE
    cout << "Construct mbInterfaceCM" << endl;
#endif //#ifdef TEST_MODE

    m_model = nullptr;
    m_inTensorPatches = nullptr;
    m_outTensorScores = nullptr;
}
mbInterfaceCM::~mbInterfaceCM()
{
#ifdef TEST_MODE
    cout << "Destruct mbInterfaceCM" << endl;
#endif //#ifdef TEST_MODE
    if (m_model != nullptr)
    {
        delete m_inTensorPatches;
        delete m_outTensorScores;
        delete m_model;
    }
}

bool mbInterfaceCM::LoadNewModel(const char *modelPath, const char *ckptPath, const char *intensor, const char *outtensor)
{
#ifdef TEST_MODE
    std::cout << " LoadNewModel begin" << std::endl;
#endif //#ifdef TEST_MODE

    if (m_model != nullptr)
    {
        delete m_model;
        delete m_inTensorPatches;
        delete m_outTensorScores;
    }

    m_model = new Model(modelPath, CreateSessionOptions(0.3));
    if (ckptPath != nullptr)
        m_model->restore(ckptPath);

    m_inTensorPatches = new Tensor(*m_model, intensor);
    m_outTensorScores = new Tensor(*m_model, outtensor);

    return true;
}

std::vector<float> mbInterfaceCM::RunRGBimage(cv::Mat img)
{
    cv::Mat img_resized;

    //resize
    cv::resize(img, img_resized, cv::Size(m_patchWidth, m_patchHeight));
    // Put image in vector
    std::vector<float> img_resized_data(m_patchWidth * m_patchHeight * 3);

    for (size_t i = 0; i < img_resized_data.size(); i = i + 1)
    {
        img_resized_data[i] = img_resized.data[i] / 255.0;
    }

    // Put vector in Tensor
    this->m_inTensorPatches->set_data(img_resized_data, {1, img_resized.rows, img_resized.cols, 3});
    //input->set_data(img_resized_data);
    this->m_model->run(m_inTensorPatches, m_outTensorScores);
    return (m_outTensorScores->get_data<float>());
}

std::vector<float> mbInterfaceCM::RunRGBImgPath(const unsigned char *ptr)
{
    cv::Mat img = cv::imread(string((const char *)ptr), CV_LOAD_IMAGE_COLOR);
    return (RunRGBimage(img));
}

void mbInterfaceCM::IdleRun()
{
    if (m_model == nullptr)
        return;

    int BS = 1;
    if (m_hardBatchSize > 1)
        BS = m_hardBatchSize;
    std::vector<float> inVec(BS * m_patchHeight * m_patchWidth * 3);

    // Put vector in Tensor
    this->m_inTensorPatches->set_data(inVec, {BS, m_patchHeight, m_patchWidth, 3});
    this->m_model->run(m_inTensorPatches, m_outTensorScores);
}

std::vector<float> mbInterfaceCM::GetResultScores(int i)
{
    //TODO: check if results do exist

    return (m_outTensorScores->get_data<float>());
}

std::vector<float> mbInterfaceCM::RunImgBB(cv::Mat img, OD::OD_BoundingBox bb)
{
    cv::Mat croppedRef, img_resized;
#ifdef TEST_MODE
    cv::Mat debugImg = img.clone();
#endif //#ifdef TEST_MODE

    //get sub-image

    cv::Rect myROI(bb.x1, bb.y1, bb.x2 - bb.x1, bb.y2 - bb.y1);

    croppedRef = img(myROI);

#ifdef TEST_MODE
    cv::rectangle(debugImg, myROI, cv::Scalar(0, 255, 0), 5);
    cv::imwrite("t1.png", img);
    cv::imwrite("t2.png", croppedRef);
#endif //#ifdef TEST_MODE

    //resize
    cv::resize(croppedRef, img_resized, cv::Size(m_patchWidth, m_patchHeight));

#ifdef TEST_MODE
    cv::imwrite("t1a.png", debugImg);
    cv::imwrite("t3.png", img_resized);
#endif //#ifdef TEST_MODE

    //apply
    // Put image in vector
    std::vector<float> img_resized_data(m_patchWidth * m_patchHeight * 3);

    for (size_t i = 0; i < img_resized_data.size(); i = i + 1)
    {
        img_resized_data[i] = img_resized.data[i] / 255.0;
    }

    // Put vector in Tensor
    this->m_inTensorPatches->set_data(img_resized_data, {1, img_resized.rows, img_resized.cols, 3});
    //input->set_data(img_resized_data);
    this->m_model->run(m_inTensorPatches, m_outTensorScores);
    return (m_outTensorScores->get_data<float>());
}

bool mbInterfaceCM::RunImgWithCycleOutput(cv::Mat img, OD::OD_CycleOutput *co, int startInd, int stopInd, bool copyResults)
{
#ifdef TEST_MODE
    cv::Mat debugImg = img.clone();
#endif //#ifdef TEST_MODE

    int N = co->numOfObjects; // N can be smaller or bigger than BS
    int origStopInd = stopInd;

    int BS = stopInd - startInd + 1; // requested batch size

    if (m_hardBatchSize)
        BS = m_batchSize; // force BS

    std::vector<float> inVec(BS * m_patchHeight * m_patchWidth * 3);

    int tempStopInd = stopInd;
    int ind = 0;
    while (1)
    {
        ind = 0;
        if (tempStopInd - startInd + 1 > BS)
            tempStopInd = BS - 1 + startInd;

        for (size_t i = startInd; i <= tempStopInd; i++)
        {
            if (i >= N)
            {
                //jic, not suppose to happen
                break;
            }
            cv::Mat croppedRef, img_resized;
            //crop
            OD::OD_BoundingBox bb = co->ObjectsArr[i].tarBoundingBox;
            cv::Rect myROI(bb.x1, bb.y1, bb.x2 - bb.x1, bb.y2 - bb.y1);

#ifdef TEST_MODE
            cv::rectangle(debugImg, myROI, cv::Scalar(0, 255, 0), 5);
#endif //#ifdef TEST_MODE

            croppedRef = img(myROI);
            //resize
            cv::resize(croppedRef, img_resized, cv::Size(m_patchWidth, m_patchHeight));

            // Put image in vector
            for (size_t i1 = 0; i1 < m_patchWidth * m_patchHeight * 3; i1++)
            {
                inVec[ind + i1] = img_resized.data[i1] / 255.0;
            }
            ind += m_patchWidth * m_patchHeight * 3;
        }
#ifdef TEST_MODE
        cv::imwrite("color_batch.png", debugImg);
#endif //#ifdef TEST_MODE \
    // Put vector in Tensor
        this->m_inTensorPatches->set_data(inVec, {BS, m_patchHeight, m_patchWidth, 3});

        m_model->run(m_inTensorPatches, m_outTensorScores);
        std::vector<float> allscores = m_outTensorScores->get_data<float>();
        if (copyResults)
        {
            ind = 0;
            for (size_t si = startInd; si <= tempStopInd; si++)
            {
                //subvector
                vector<float>::const_iterator first = allscores.begin() + ind * m_numColors;
                vector<float>::const_iterator last = allscores.begin() + (ind + 1) * m_numColors;
                vector<float> outRes(first, last);

                //get color
                //argmax
                uint color_id = std::distance(outRes.begin(), std::max_element(outRes.begin(), outRes.end()));

#ifdef TEST_MODE
                cout << "color id = " << color_id << endl;
                PrintColor(color_id);
                // score
                cout << "Net score: " << outRes[color_id] << endl;
#endif //#ifdef TEST_MODE \
    // copy res into co
                co->ObjectsArr[si].tarColor = TargetColor(color_id);
                co->ObjectsArr[si].tarColorScore = outRes[color_id];
                ind++;
            }
        }
        if (tempStopInd >= origStopInd)
            break;
        else
        {
            startInd = tempStopInd + 1;
            tempStopInd = origStopInd;
        }
    }
    return true;
}

OD::e_OD_TargetColor mbInterfaceCM::TargetColor(uint cid)
{
    switch (cid)
    {
    case 0:
#ifdef TEST_MODE
        cout << "Color: white" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::WHITE;
    case 1:
#ifdef TEST_MODE
        cout << "Color: black" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::BLACK;
    case 2:
#ifdef TEST_MODE
        cout << "Color: gray" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::GRAY;
    case 3:
#ifdef TEST_MODE
        cout << "Color: red" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::RED;
    case 4:
#ifdef TEST_MODE
        cout << "Color: green" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::GREEN;
    case 5:
#ifdef TEST_MODE
        cout << "Color: blue" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::BLUE;
    case 6:
#ifdef TEST_MODE
        cout << "Color: yellow" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::YELLOW;
    default:
#ifdef TEST_MODE
        cout << "Color: UNKNOWN_COLOR" << endl;
#endif //#ifdef TEST_MODE
        return OD::e_OD_TargetColor::UNKNOWN_COLOR;
    }
}
