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
    Tensor* m_outTensorNumDetections;
    Tensor* m_outTensorScores;
    Tensor* m_outTensorBB;
    Tensor* m_outTensorClasses;
    Tensor* m_inpName ;

    
  public:
    cv::Mat m_keepImg;
    bool m_show;
    cv::Mat GetKeepImg(){return m_keepImg;}
  public:
    //constructors
	  mbInterfaceATR();
    ~mbInterfaceATR();

    bool LoadNewModel(const char* modelPath);
    int RunRGBimage(cv::Mat img);
    int RunRGBVector(const unsigned char *ptr, int height, int width);
    int RunRGBVector(std::vector<uint8_t > img_data, int height, int width);
    int RunRGBImgPath(const unsigned char *ptr);
    int RunRawImage(const unsigned char *ptr, int height, int width);
    int RunRawImageFast(const unsigned char *ptr, int height, int width, int colorType);
    int GetResultNumDetections();
    int GetResultClasses(int i);
    float GetResultScores(int i);
    std::vector<float>  GetResultBoxes();

    
};