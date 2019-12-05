#pragma once

#include <opencv2/opencv.hpp>
#include <cppflow/Tensor.h>
#include <cppflow/Model.h>
#include <string>

class mbInterfaceATR
{
  protected:
	bool active;
    Model* m_model;
    Tensor* m_outNames1;
    Tensor* m_outNames2;
    Tensor* m_outNames3;
    Tensor* m_outNames4;
    Tensor * m_inpName ;

  public:
    
    bool m_show;
  public:
	mbInterfaceATR();
    bool LoadNewModel(const char* modelPath);
    int RunRGBimage(cv::Mat img);
    int GetResultNumDetections();
    int GetResultClasses(int i);
    float GetResultScores(int i);
    std::vector<float>  GetResultBoxes();
};