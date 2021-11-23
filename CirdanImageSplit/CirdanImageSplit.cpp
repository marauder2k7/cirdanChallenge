#include <iostream>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"
#include <rapidjson/writer.h>

using namespace cv;
using namespace std;
using namespace rapidjson;

namespace fs = std::filesystem;

std::string GetCurrentDir()
{
   char buffer[MAX_PATH];
   GetModuleFileNameA(NULL, buffer, MAX_PATH);
   std::string::size_type pos = std::string(buffer).find_last_of("\\/");

   return std::string(buffer).substr(0, pos);
}

void cvPartitionMat(const Mat& img, int xWidth, int yHeight, string imgName, string ext, string outPath, const Mat& viz)
{
   int sizeX = img.size().width;
   int xCount = xWidth;
   int sizeY = img.size().height;
   int yCount = yHeight;
   int cellWidth = sizeX / xCount;
   int cellHeight = sizeY / yCount;
   int cur = 0;
   int total = yCount * xCount;

   for (int y = 0; y < yCount; y++)
   {
      for (int x = 0; x < xCount; x++)
      {
         cur++;
         string fileName = to_string(y) + "_" + to_string(x);
         int ptx = x * cellWidth;
         int pty = y * cellHeight;
         Mat block = img(cv::Rect(ptx, pty, cellWidth, cellHeight)).clone();
         string fullPath = outPath + "\/" + fileName;
         cv::imwrite(fullPath + ".png", block);
         cv::rectangle(viz, cv::Rect(ptx, pty, cellWidth, cellHeight), Scalar(255, 0, 0));

         string partNum = to_string(cur) + " of " + to_string(total);

         string pathJson = fullPath + ".json";
         // start the writer document
         Document d;
         d.SetObject();

         // add members
         d.AddMember("OriginalFile", Value(imgName.c_str(),d.GetAllocator()), d.GetAllocator());
         d.AddMember("OriginalExtension", Value(ext.c_str(),d.GetAllocator()), d.GetAllocator());
         d.AddMember("PartitionName", Value(fileName.c_str(),d.GetAllocator()), d.GetAllocator());
         d.AddMember("Destination", Value(outPath.c_str(),d.GetAllocator()), d.GetAllocator());
         d.AddMember("Height", Value(cellHeight), d.GetAllocator());
         d.AddMember("Width", Value(cellWidth), d.GetAllocator());
         d.AddMember("Number", Value(partNum.c_str(), d.GetAllocator()), d.GetAllocator());

         // write it out to file
         FILE* fp = fopen(pathJson.c_str(), "wb");
         char writeBuffer[65536];
         FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
         Writer<FileWriteStream> writer(os);
         d.Accept(writer);

         StringBuffer buff;
         Writer<StringBuffer> jwriter(buff);
         d.Accept(jwriter);
         cout << buff.GetString() << endl;
         jwriter.Reset(buff);
         // close up.
         fclose(fp);
      }
   }

}

int main(int argc, char** argv)
{
   // initilalizers.
   string imageFile;
   int xWidth = 0;
   int yHeight = 0;

   // error handling need at least 1 arg.
   if (argc < 2)
   {
      cout << "Usage:" << endl;
      cout << "arg 1 image or text file" << endl;
      cout << "arg 2 rows" << endl;
      cout << "arg 3 columns" << endl;
      return -1;
   }

   // find extension of first arg the file input.
   int ext = 0;
   char* p = strrchr(argv[1], '.');
   if (strcmp(p, ".png") == 0)
   {
      // if png we need more than 1 arg to split the file.
      if (argc != 4)
      {
         cout << "Usage: " << argv[2] << " + " << argv[3] << " Should be split dimensions." << endl;
         return -1;
      }

      imageFile = argv[1];
      fs::path path(argv[1]);
      xWidth = stoi(argv[2]);
      yHeight = stoi(argv[3]);
      
      size_t pos = path.filename().string().find(".");
      string imgName = path.filename().string().substr(0, pos);
      string ext = path.extension().string();

      Mat img;
      img = imread(imageFile, IMREAD_COLOR);

      if (img.empty())
      {
         cout << "Could not open the image." << endl;
         return -1;
      }
      string outPath = "./" + imgName;

      fs::create_directory(outPath);
      Mat viz = img.clone();

      cvPartitionMat(img, xWidth, yHeight, imgName, ext, outPath, viz);

      cv::namedWindow("Visualisation Output", WINDOW_AUTOSIZE);
      cv::imshow("Visualisation Output", viz);
      waitKey(0);

   }
   else if(strcmp(p, ".txt") == 0)
   {
      // if text read each line in.
      ifstream inFile (argv[1]);
      if (!inFile.is_open())
      {
         cout << "file can't open" << endl;
         return -1;
      }

      while (inFile >> imageFile >> xWidth >> yHeight)
      {
         Mat img;
         string imgFileDir = GetCurrentDir();
         string imgFull = imgFileDir + "\/" + imageFile;
         img = imread(imgFull, IMREAD_COLOR);
         if (img.empty())
         {
            cout << "Could not open the image from the txt file." << endl;
            return -1;
         }

         // find image name for file directory.
         size_t pos = imageFile.find_last_of(".");
         string imgName = imageFile.substr(0, pos);
         string ext = imageFile.substr(pos);
         string outPath = "./" + imgName;
         fs::create_directory(outPath);
         Mat viz = img.clone();

         cvPartitionMat(img, xWidth, yHeight,imgName, ext, outPath, viz);

         cv::namedWindow("Visualisation Output", WINDOW_AUTOSIZE);
         cv::imshow("Visualisation Output", viz);
         waitKey(0);

      }
      inFile.close();

   }
   else
   {
      cerr << "Error file unknown." << endl;
      return -1;
   }

    return 0;
}
