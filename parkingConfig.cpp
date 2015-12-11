#include "parkingConfig.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

string parkingConfig::filePath = "";

parkingConfig::parkingConfig (const parkingConfig &obj) {
	uid = obj.uid;
	uid_device = obj.uid_device;
	timestamp = obj.timestamp;
	edgesOnly = obj.edgesOnly;
	showROIs = obj.showROIs;
	edge[0] = obj.edge[0];
	edge[1] = obj.edge[1];
	serverIP = obj.serverIP;
	serverPort = obj.serverPort;
	resolution[0] = obj.resolution[0];
	resolution[1] = obj.resolution[1];
	hflip = obj.hflip;
	vflip = obj.vflip;
	videoPort = obj.videoPort;
	framerate = obj.framerate;
	rotation = obj.rotation;
	maxThreads = obj.maxThreads;
	sobelSize = obj.sobelSize;
	blurSize = obj.blurSize;
	treshold = obj.treshold;
	ROI = obj.ROI;
	ROIMask = obj.ROIMask;
	ROIArea = obj.ROIArea;
	JPEGQuality = obj.JPEGQuality;
}

void parkingConfig::reConstructMask(void){
	ROIMask.clear();
	ROIArea.clear();
	for(unsigned int i = 0; i < ROI.size(); i++){
		unsigned int area = 0;
		cv::RotatedRect rect = ROI[i];
		cv::Rect bounding = rect.boundingRect();
		vector<cv::Point2f> contour(4);
		rect.center.x = bounding.width / 2;
		rect.center.y = bounding.height / 2;
		rect.points(contour.data());
		ROIMask.push_back(cv::Mat(bounding.height, bounding.width, CV_8UC1, cv::Scalar::all(0)));
		for(int y = 0; y < ROIMask[i].rows; y++)
			for(int x = 0; x < ROIMask[i].cols; x++)
				if(cv::pointPolygonTest(contour, cv::Point2f(x, y), false) >= 0){
					ROIMask[i].at<uchar>(y, x) = 255;
					area++;
				}
		ROIArea.push_back(area);
	}
}

parkingConfig::parkingConfig(std::string path)
{
	filePath = path;
	def();
	if(load() != 0)
		def();
	cout << "Current config" << toJSON(true) << endl;
}

void parkingConfig::fromJSON(Json::Value root){
	uid = root["UID"].asString();
	timestamp = root["timestamp"].asBool();
	edgesOnly = root["edgesOnly"].asBool();
	showROIs = root["showROIs"].asBool();
	edge[0] = root["edge"][0].asUInt();
	edge[1] = root["edge"][1].asUInt();
	serverIP = root["server"]["ip"].asString();
	serverPort = root["server"]["port"].asUInt();
	resolution[0] = root["camera"]["resolution"][0].asUInt();
	resolution[1] = root["camera"]["resolution"][1].asUInt();
	hflip = root["camera"]["hflip"].asBool();
	vflip = root["camera"]["vflip"].asBool();
	videoPort = root["camera"]["videoPort"].asBool();
	framerate = root["camera"]["framerate"].asFloat();
	rotation = root["camera"]["rotation"].asUInt();
	maxThreads = root["maxThreads"].asUInt();
	treshold = root["treshold"].asFloat();
	blurSize = root["blurSize"].asUInt();
	sobelSize = root["sobelSize"].asUInt();
	JPEGQuality = root["JPEGQuality"].asUInt();
	ROI.clear();
	for(uint i = 0; i < root["ROI"].size(); i++)
		ROI.push_back( cv::RotatedRect(
			cv::Point2f( root["ROI"][i]["center"][0].asFloat(), root["ROI"][i]["center"][1].asFloat()),
			cv::Size2f( root["ROI"][i]["size"][0].asFloat(), root["ROI"][i]["size"][1].asFloat()),
			root["ROI"][i]["angle"].asFloat()));
	reConstructMask();
}

void parkingConfig::fromJSON(string json){
	Json::Reader reader;
	Json::Value root;
	reader.parse(json, root);
	fromJSON(root);
}

int parkingConfig::load(){
	try{
		ifstream ifs (filePath , ifstream::in);
		if(!ifs.is_open())
			throw 1;
		Json::Value root;
		ifs >> root;
		ifs.close();
		fromJSON(root);
		return 0;
	} catch (int e){
                switch (e){
                case 1:
                        cerr << "Error opening config file " << filePath << endl;
                break;
                default:
                        cerr << "Unhandled exception during loading settings: " << e << endl;
                }
        }
        catch (exception e){
                cerr << "Unhandled exception during loading settings: " << e.what() << endl;
        }
	return 1;
}

int parkingConfig::save(){
	ofstream ofs (filePath , ofstream::out);
	try{
		if(!ofs.is_open())
			throw 1;
		ofs << toJSON(true);
		ofs.close();
		return 0;
	}catch (int e){
		switch (e){
		case 1:
			cerr << "Error opening config file for saving\n";
		break;
		default:
			cerr << "Unhandled exception, using default settings: " << e << endl;
		}
	}catch (exception e){
		cerr << "Unhandled exception, using default settings: " << e.what() << endl;
	}
	ofs.close();
	return 1;
}


string parkingConfig::toJSON(bool styled){
	Json::Value root;
	Json::FastWriter writer;
	Json::StyledWriter swriter;
	root["UID"] = uid;
	root["devUID"] = uid_device;
	root["timestamp"] = timestamp;
	root["edgesOnly"] = edgesOnly;
	root["treshold"] = treshold;
	root["showROIs"] = showROIs;
	root["edge"][0] = edge[0];
	root["edge"][1] = edge[1];
	root["maxThreads"] = maxThreads;
	root["server"]["ip"] = serverIP;
	root["server"]["port"] = serverPort;
	root["camera"]["resolution"][0] = resolution[0];
	root["camera"]["resolution"][1] = resolution[1];
	root["camera"]["hflip"] = hflip;
	root["camera"]["vflip"] = vflip;
	root["camera"]["videoPort"] = videoPort;
	root["camera"]["rotation"] = rotation;
	root["camera"]["framerate"] = framerate;
	root["blurSize"] = blurSize;
	root["sobelSize"] = sobelSize;
	root["JPEGQuality"] = JPEGQuality;
	for(uint num = 0; num < ROI.size(); num++)
	{
		root["ROI"][num]["center"][0] = ROI[num].center.x;
		root["ROI"][num]["center"][1] = ROI[num].center.y;
		root["ROI"][num]["size"][0] = ROI[num].size.width;
		root["ROI"][num]["size"][1] = ROI[num].size.height;
		root["ROI"][num]["angle"] = ROI[num].angle;
	}
	if(styled)
		return swriter.write(root);
	else
		return writer.write(root);
}

void parkingConfig::def(){
	timestamp = true;
	edgesOnly = false;
	treshold = 20.0;
	edge[0] = 100;
	edge[1] = 150;
	resolution[0] = 2592;
	resolution[1] = 1944;
	serverPort = 10010;
	serverIP = "137.117.242.220";
	rotation = 0;
	videoPort = vflip = hflip = false;
	framerate = 1;
	showROIs = true;
	maxThreads = 1;
	blurSize = 3;
	sobelSize = 3;
	JPEGQuality = 65;
	uid = uid_device = getUID();
}

string parkingConfig::getUID()
{
	string uid = "";
	string line;
	ifstream ifs ("/proc/cpuinfo" , ifstream::in);
	try{
		if(!ifs.is_open())
			throw 1;
		while(!ifs.eof()){
			getline(ifs, line);
			if(line.find("Serial") == string::npos)
				continue;
			uid = line.substr(line.find(":") + 2);
			break;
		}
	}catch(int e){
                switch (e){
                case 1:
                        cerr << "Error opening /proc/cpuinfo\n";
                break;
                default:
                        cerr << "Unhandled exception, during UID reading: " << e << endl;
                }

	}catch (exception e){
                cerr << "Unhandled exception during UID reading: " << e.what() << endl;
        }

	ifs.close();
	return uid;
}

parkingConfig::~parkingConfig()
{
}

