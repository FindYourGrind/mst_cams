#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <json/json.h>

class parkingConfig
{
public:
	parkingConfig(std::string path);
	parkingConfig (const parkingConfig &obj);
	~parkingConfig();

	int save();
	int load();
	void def();
	static std::string getUID();
	std::string toJSON(bool styled=false);
	void fromJSON(Json::Value root);
	void fromJSON(std::string json);
	std::string serverIP;
	std::vector<cv::RotatedRect> ROI;
	std::vector<cv::Mat> ROIMask;
	std::vector<unsigned int> ROIArea;
	bool timestamp;
	bool edgesOnly;
	bool showROIs;
	unsigned char blurSize;
	unsigned char sobelSize;
	unsigned char maxThreads;
	unsigned char JPEGQuality;
	unsigned char edge[2];
	unsigned short rotation;
	unsigned short serverPort;
	std::string uid;
	float treshold;
private:
	void reConstructMask(void);
	static std::string filePath;
	std::string uid_device;
	unsigned short resolution[2];
	float framerate;
	bool hflip;
	bool vflip;
	bool videoPort;

};


