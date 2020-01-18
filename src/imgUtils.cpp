#include <utils/imgUtils.h>

using namespace cv;
using namespace std;


Mat rotate(Mat src, double angle)
{
    Mat dst;
    Point2f pt(src.cols/2., src.rows/2.);    
    Mat r = getRotationMatrix2D(pt, angle, 1.0);
    warpAffine(src, dst, r, Size(src.cols, src.rows));
    return dst;
}

std::vector<unsigned char> readBytesFromFile(const char* filename)
{
    std::vector<unsigned char> result;

    FILE* f = fopen(filename, "rb");

    fseek(f, 0, SEEK_END);  // Jump to the end of the file
    long length = ftell(f); // Get the current byte offset in the file
    rewind(f);              // Jump back to the beginning of the file

    result.resize(length);

    char* ptr = reinterpret_cast<char*>(&(result[0]));
    fread(ptr, length, 1, f); // Read in the entire file
    fclose(f); // Close the file

    return result;
}

bool convertYUV420toRGB(vector <unsigned char> raw, int width, int height, cv::Mat* outRGB)//,
                        // vector <unsigned char>* R = NULL, 
                        // vector <unsigned char>* G = NULL,
                        // vector <unsigned char>* B = NULL)
    {
    // vector <unsigned char> Y ; Y.reserve(width*height);
    // vector <unsigned char> U ; U.reserve(width*height);
    // vector <unsigned char> V ; V.reserve(width*height);
    //TODO: this is very slow
    cv:Mat ymat(height, width,CV_8UC1), umat(height, width,CV_8UC1), vmat(height, width,CV_8UC1);
   
    int t = 0;
    int skip = 1;
    for (int i=0;i<height;i++)
        for(int j = 0;j<width;j++)
        {
        // Y[t]=raw[t*2];
       
      
        ymat.at<uint8_t>(i,j)=raw[t*2];
        if(skip >0)
        {
        // U[2*t]=raw[t*2+1];
        // U[2*t+1]=U[2*t];
        // V[2*t]=raw[t*2+3];
        // V[2*t+1]=V[2*t];
        
        umat.at<uint8_t>(i,j)=raw[t*2 + 1];
        umat.at<uint8_t>(i,j+1)=raw[t*2 + 1];
        
        vmat.at<uint8_t>(i,j)=raw[t*2 + 3];
        vmat.at<uint8_t>(i,j+1)=raw[t*2 + 3];
        }
        skip = -skip;
        t = t + 1;

    }
    cv::imwrite("tryY.png",ymat);
    cv::imwrite("tryU.png",umat);
    cv::imwrite("tryV.png",vmat);
    
    cv::Mat yuv;

    std::vector<cv::Mat> yuv_channels = { ymat, umat, vmat };
    cv::merge(yuv_channels, yuv);

    // cv::Mat rgb(height, width,CV_8UC3);
    cv::cvtColor(yuv, *outRGB, cv::COLOR_YUV2RGB);
    cv::imwrite("RGB.tif", *outRGB);

return true;
}



bool convertYUV420toVector(vector <unsigned char> raw, int width, int height, vector <uint8_t>* outVector)
    {

    cv:Mat ymat(height, width,CV_8UC1), umat(height, width,CV_8UC1), vmat(height, width,CV_8UC1);
   
    int t = 0;
    int skip = 1;
    for (int i=0;i<height;i++)
        for(int j = 0;j<width;j++)
        {
        // Y[t]=raw[t*2];
       
      
        ymat.at<uint8_t>(i,j)=raw[t*2];
        if(skip >0)
        {
        umat.at<uint8_t>(i,j)=raw[t*2 + 1];
        umat.at<uint8_t>(i,j+1)=raw[t*2 + 1];
        
        vmat.at<uint8_t>(i,j)=raw[t*2 + 3];
        vmat.at<uint8_t>(i,j+1)=raw[t*2 + 3];
        }
        skip = -skip;
        t = t + 1;

    }
    cv::imwrite("tryY.png",ymat);
    cv::imwrite("tryU.png",umat);
    cv::imwrite("tryV.png",vmat);
    
    cv::Mat yuv;

    std::vector<cv::Mat> yuv_channels = { ymat, umat, vmat };
    cv::merge(yuv_channels, yuv);

    cv::Mat rgb(height, width,CV_8UC3);
    cv::cvtColor(yuv, rgb, cv::COLOR_YUV2RGB);
   
    outVector = new std::vector<uint8_t >();
    outVector->reserve(width*height*3);
    int a = outVector->size();
    outVector->assign(rgb.data, rgb.data + rgb.total() * rgb.channels());

return true;
}

bool convertCvMatToVector(cv::Mat* inBGR, std::vector<uint8_t>* outVec )
{
    int h = inBGR->rows;
    int w = inBGR->cols;
    int c = inBGR->channels();

    cv::Mat rgb(h, w, CV_8UC3);
    cv::cvtColor(*inBGR, rgb, cv::COLOR_BGR2RGB);

    outVec = new std::vector<uint8_t >();
    outVec->reserve(w*h*c);
    outVec->assign(rgb.data, rgb.data + rgb.total() * rgb.channels());

    return true;
}

void balance_white(cv::Mat mat) {
  double discard_ratio = 0.05;
  int hists[3][256];
  memset(hists, 0, 3*256*sizeof(int));

  for (int y = 0; y < mat.rows; ++y) {
    uchar* ptr = mat.ptr<uchar>(y);
    for (int x = 0; x < mat.cols; ++x) {
      for (int j = 0; j < 3; ++j) {
        hists[j][ptr[x * 3 + j]] += 1;
      }
    }
  }

  // cumulative hist
  int total = mat.cols*mat.rows;
  int vmin[3], vmax[3];
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 255; ++j) {
      hists[i][j + 1] += hists[i][j];
    }
    vmin[i] = 0;
    vmax[i] = 255;
    while (hists[i][vmin[i]] < discard_ratio * total)
      vmin[i] += 1;
    while (hists[i][vmax[i]] > (1 - discard_ratio) * total)
      vmax[i] -= 1;
    if (vmax[i] < 255 - 1)
      vmax[i] += 1;
  }


  for (int y = 0; y < mat.rows; ++y) {
    uchar* ptr = mat.ptr<uchar>(y);
    for (int x = 0; x < mat.cols; ++x) {
      for (int j = 0; j < 3; ++j) {
        int val = ptr[x * 3 + j];
        if (val < vmin[j])
          val = vmin[j];
        if (val > vmax[j])
          val = vmax[j];
        ptr[x * 3 + j] = static_cast<uchar>((val - vmin[j]) * 255.0 / (vmax[j] - vmin[j]));
      }
    }
  }
}

cv::Mat CreateTiledGzir(const char* imname, uint W, uint H)
{
    uint gap = 100;
    float whiteBalanceProb = 0.1;
    float rotAngleMax = 90;
    float rotAngleProb = 0.1;
    float flipLRprob = 0.1;
    float flipUDprob = 0.1;
    float resizeFactor[2]={0.5, 2.0};
    float resizeProb = 0.25;
    float histoEqProb = 0.1;


    uint gapX=gap, gapY = gap;
    bool flag = true;
    uint maxX = 0;
    uint maxY = 0;
    uint topX = 10;
    uint topY = 10;

    cv::Mat bigImg(H, W, CV_8UC3);
    cv::Mat im = cv::imread(imname, CV_LOAD_IMAGE_COLOR);
    list <float*> trueTargets;
    
    uint nextYline = 0;
   
    while (flag)
    {
      cv::Mat im0 = im.clone();
    
     
      // all possible augmentations based on probabilities on im 
      bool doWB = ((double) rand() / (RAND_MAX)) < whiteBalanceProb;
      bool doRot = ((double) rand() / (RAND_MAX)) < rotAngleProb;
      double randAngle = ((double) rand() / (RAND_MAX) - 0.5) * 2 * rotAngleMax;
      bool doFlipLR = ((double) rand() / (RAND_MAX)) < rotAngleProb;
      bool doFlipUD = ((double) rand() / (RAND_MAX)) < rotAngleProb;
      bool doResize = ((double) rand() / (RAND_MAX)) < resizeProb;
      double randResize = resizeFactor[0] + ((double) rand() / (RAND_MAX))*(resizeFactor[1] - resizeFactor[0] );
      

      if(doWB) 
         balance_white(im0);

      
      Scalar v( 100, 100, 100 ); // color for padding 

      int top = max(im0.cols - im0.rows, 0)/2 + 10;
      int lef = max(im0.rows - im0.cols, 0)/2 + 10;


      copyMakeBorder( im0, im0, top, top,  lef, lef, BORDER_CONSTANT, v );

       if(doFlipLR)
        {
          cv::flip(im0, im0, 1);
        }
        if(doFlipUD)
        {
          cv::flip(im0, im0, 0);
        }

    if(!doRot)
          randAngle = 0;

        Mat im1 = rotate(im0, randAngle);

        

       
        if(doResize)
        {
           cv::resize(im1, im1, cv::Size(int(im1.cols*randResize), int(im1.rows*randResize)));
        }


        if(topX+im1.cols>W - gapX)
            {
                topX = 10;
                topY = nextYline + gapY;
            }
        
        if(topY+im1.cols>H-gapY)
            break;
    
        
        int offX = rand()%gapX;
        int offY = rand()%gapY;

        im1.copyTo(bigImg(cv::Rect(topX + offX,topY + offY,im1.cols, im1.rows)));

        float* cXcY = new float[2];
         cXcY[0] = topX + offX + im1.cols*0.5;
         cXcY[1] = topY + offY + im1.rows*0.5;

        trueTargets.push_back(cXcY);

        nextYline = max(nextYline, topY + im1.rows);
        topX = topX + im1.cols + gapX;
       
    }
    cv::imwrite("bigTiled.tif", bigImg);
    //TODO: return trueTargets;
    return bigImg;
}