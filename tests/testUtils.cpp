#include <utils/imgUtils.h>

#include <opencv2/opencv.hpp>
//#include <e:/Installs/opencv/sources/include/opencv2/opencv.hpp>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <algorithm>

using namespace std;
using namespace std::chrono;
using namespace cv;
//using namespace OD;

int main()
{
  int H = 4056;
  int W = 3040;

  cout << "Test utils" << endl;
  //emulate buffer
  cout << "readBytesFromFile raw" << endl;
  std::vector<unsigned char> rawVec = readBytesFromFile("media/00006160.raw");

  //
  //start timer
  auto start = high_resolution_clock::now();
  //

  cv::Mat *myRGB = new Mat(H, W, CV_8UC3);
  convertYUV420toRGB(rawVec, H, W, myRGB);
  // save JPG for debug

  //end timer
  auto stop = high_resolution_clock::now();
  //

  //calculate time difference
  auto duration = duration_cast<milliseconds>(stop - start);
  cout << duration.count() << endl;
  //

  //DEBUG
  cv::Mat bgr(H, W, CV_8UC3);
  cv::cvtColor(*myRGB, bgr, cv::COLOR_RGB2BGR);
  cv::imwrite("raw2im.tif", bgr);

  // RAW -> RGB vector to be used in inference
  std::vector<unsigned char> *myVector;
  convertYUV420toVector(rawVec, H, W, myVector);

  //Read TIF -> mat -> RGB vector to be used in  inference
  #ifdef OPENCV_MAJOR_4
  cv::Mat im = cv::imread("orig.tif", IMREAD_COLOR );//CV_LOAD_IMAGE_COLOR
  #else
  cv::Mat im = cv::imread("orig.tif", CV_LOAD_IMAGE_COLOR );//
  #endif
  std::vector<unsigned char> *myVector1;
  convertCvMatToVector(&im, myVector1);

  // READ TIF -> convert to buffer
  #ifdef OPENCV_MAJOR_4
  cv::Mat imrgb1 = cv::imread("00000018.tif", IMREAD_COLOR );//CV_LOAD_IMAGE_COLOR
  #else
  cv::Mat imrgb1 = cv::imread("00000018.tif", CV_LOAD_IMAGE_COLOR );//
  #endif
  unsigned char *array = new unsigned char(imrgb1.rows * imrgb1.cols * imrgb1.channels());
  if (imrgb1.isContinuous())
    array = imrgb1.data;
  else
  {
    {
      uchar **array = new uchar *[imrgb1.rows];
      for (int i = 0; i < imrgb1.rows; ++i)
        array[i] = new uchar[imrgb1.cols];

      for (int i = 0; i < imrgb1.rows; ++i)
        array[i] = imrgb1.ptr<uchar>(i);
    }
  }

  // buffer to tif and save
  cv::Mat imrgb2(imrgb1.rows, imrgb1.cols, CV_8UC3);
  imrgb2.data = array;
  int ch = imrgb2.channels();
  cv::imwrite("isrgb2.tif", imrgb2);

  return 0;
}