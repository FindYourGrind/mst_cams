// cvMainProcessing.cpp : Defines the entry point for the console application.
//

#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <sstream> // stringstream
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "parkingConfig.hpp"
using namespace std;

#define CONTROL_PORT 10000
parkingConfig *config;
mutex configMutex;

vector<bool> lotStatus;
mutex lotMutex;


void processing(vector<char> buff, parkingConfig conf);
void controlInterface(int sock);
void controlInterfaceServer();
void reportInterface(parkingConfig conf);
float filling(cv::Mat& img, parkingConfig& conf, int zone);
void sendReport(int sock, vector<bool>& currentStatus, parkingConfig& conf);
int rotatedColor(cv::Mat& img, cv::RotatedRect& roi);
int parseAccept(void);
std::string return_current_time_and_date();

volatile bool done = false;

enum command_id{CMD_REBOOT, CMD_GET_CONFIG, CMD_SET_CONFIG, CMD_SAVE_CONFIG, CMD_LOAD_CONFIG};

void controlInterface(int sock){
	try{
		uint readed;
		while(!done){
			struct command_t{
				uint	header, command, opcode;
			} command;
			if(recv(sock, &command, sizeof(command), 0) <= 0)
				switch(errno){
					case 0:
						throw 2;
					break;
					case EAGAIN:
					continue;
					default:
						throw 1;
				}
			if(command.header != 0xAABBCCDD)
				throw 3;
			string conf;
			vector<char> buff;

			switch(command.command){
			case CMD_REBOOT:
				system("reboot");
			break;
			case CMD_LOAD_CONFIG:
				config->load();
			case CMD_GET_CONFIG:
				conf = config->toJSON();
				command.opcode = conf.size() + 1;
				if(send(sock, &command, sizeof(command), 0) < 0)
					throw 1;
				if(send(sock, conf.c_str(), conf.length() + 1, 0) < 0)
					throw 1;
			break;
			case CMD_SET_CONFIG:
				buff.resize(command.opcode);
				readed = 0;
				while(readed < command.opcode){
					int i = recv(sock, buff.data() + readed, command.opcode - readed, 0);
					if(i > 0)
						readed += i;
					else if (i == 0)
						throw 2;
					else
						switch(errno){
						case EAGAIN:
							continue;
						default:
							throw 5;
						}
				}
				configMutex.lock();
				try{
					config->fromJSON(string(buff.data(), buff.size()));
				}catch(...){
					configMutex.unlock();
					cerr << "Control interface error parsing JSON\n";
					throw 5;
				}
				configMutex.unlock();
				cout << "Recevied new config file" << config->toJSON(true) << endl;
			break;
			case CMD_SAVE_CONFIG:
				config->save();
			break;
			default:
				throw 4;
			}
		}
	}catch(const exception& e){
		cerr << "Unhandled exception " << e.what() << endl;
	}catch(int e){
		switch(e){
		case 1:
			perror("Control interface error");
		break;
		case 2:
			cerr << "Control socket disconnected gracefully\n";
		break;
		case 3:
			cerr << "Control interface header mismatch\n";
		break;
		case 4:
			cerr << "Control interface unknown command\n";
		break;
		case 5:
			cerr << "Control interface error getting settings\n";
		break;
		default:
			cerr << "Unhandled exception " << e << endl;
		}
	}
	close(sock);
}

int parseAccept(void){
	switch(errno){
	case EAGAIN:
	case EPROTO:
	case ENOPROTOOPT:
	case EHOSTDOWN:
	case ENONET:
	case EHOSTUNREACH:
	case EOPNOTSUPP:
	case ENETUNREACH:
		return 0;
		break;
	default:
		return 1;
	}
}

void sendReport(int sock, vector<bool>& currentStatus, parkingConfig& conf){
	static unsigned msg_id = 0;
	int i, sended = 0, len = 0;
	const char* buff;
        Json::Value root;
       	Json::FastWriter writer;
	string report;
        string request;

	try{
		root["dev_id"] = conf.uid;
		for(unsigned i = 0; i < currentStatus.size(); i++)
                        root["lot"][i] = (currentStatus[i]) ? 1 : 0;
                root["_POST"] = "BlaBlaBla";
		report = writer.write(root);

                request =  "POST /rest/v2/camera/ban HTTP/1.1\r\n";
		request += "Host: liko-t4p.rhcloud.com\r\n";
		request += "Connection: keep-alive\r\n";
		request += "Content-Type: application/json\r\n";
		request += "Content-Length: %u\r\n";
		request += "\r\n";
                request += report;
		buff = request.c_str();
                sprintf((char *) buff, (const char *) buff, report.length());
		len = request.length() + 1;
		while(sended < len){
			i = send(sock, buff + sended, len - sended, 0);
			if(i < 0){
				if(errno == EAGAIN)
					continue;
				else
					throw 3;
			}
			sended += i;
		}
		msg_id++;
	}catch(const exception& e){
		cerr << "Report interface error occured during report generation: " << e.what() << endl;
		throw 4;
	}
}

void reportInterface(parkingConfig conf){
	int cli = 0, i = 0;
	struct addrinfo hints, *servinfo, *p;
	vector<bool> lastSendStatus;
	while(!done){

	vector<bool> currentStatus;
	try{
		lotMutex.lock();
		currentStatus = lotStatus;
		lotMutex.unlock();
	}catch(...){
		lotMutex.unlock();
		throw 5;
	}
	if((currentStatus.size() != lastSendStatus.size()) | (currentStatus != lastSendStatus)){

	try {
		cout << "Report interface connect procedure started\n";
		memset(&hints, 0, sizeof hints);
		hints.ai_family		= AF_UNSPEC;
		hints.ai_socktype	= SOCK_STREAM;
		if ((i = getaddrinfo(conf.serverIP.c_str(), to_string(conf.serverPort).c_str(), &hints, &servinfo)) != 0)
			throw 1;

		// loop through all the results and connect to the first we can
		for(p = servinfo; p != NULL; p = p->ai_next)
		{
			try{
				cli = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
				if(cli == -1)
					throw 1;
				if(connect(cli, p->ai_addr, p->ai_addrlen) == -1)
					throw 2;
				break;
			}catch(int e){
				switch(e){
				case 1:
					cerr << "Report interface socket error: " << strerror(errno) << endl;
					break;
				case 2:
					cerr << "Report interface connect error: " << strerror(errno) << endl;
					close(cli);
					break;
				default:
					cerr << "Report interface connect procedure unhandled exception: " << e << endl;
				}
				continue;
			}
		}
		if (p == NULL)
			throw 2;
		cout << "Report interface connected successfully\n";

		sendReport(cli, currentStatus, conf);
		lastSendStatus = currentStatus;

	}catch(int e){
		switch(e){
		case 1:
			cerr << "Report interface error, next try in 1 munute: " << gai_strerror(i) << endl;
			sleep(60);
			break;
		case 2:
			cerr << "Report interface failed to connect server. Next try in 1 minute.\n";
			sleep(60);
			break;
		case 3:
			cerr << "Report interface error on socket write: " << strerror(errno) << endl;
			close(cli);
			break;
		case 4:
			cerr << "Report interface error during status reporting\n";
			close(cli);
			break;
		case 5:
			cerr << "Report interface error during shared object access\n";
			close(cli);
			break;

		default:
			cerr << "Report interface unhandled exception " << e << endl;
		}
	}
	close(cli);
        sleep(20);
    }
    }
}

void controlInterfaceServer(){
	int serv;
	struct sockaddr_in servsa;
	int enable = 1;
	try{
		memset(&servsa, 0, sizeof (servsa));
		servsa.sin_family = PF_INET;
		servsa.sin_port = htons(CONTROL_PORT);
		servsa.sin_addr.s_addr = htonl(INADDR_ANY);
		if((serv = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			throw 1;
		if (setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
			throw 1;
		if (bind(serv, (sockaddr*) &servsa, sizeof (servsa)) == -1)
			throw 1;
		if (listen(serv, 10) == -1)
			throw 1;
		while(!done) {
			struct sockaddr_in clisa;
			socklen_t socklen;
			int cli = accept(serv, (sockaddr*)&clisa, &socklen);
			if(cli == -1)
				if(parseAccept())
					throw 1;
			cout << inet_ntoa(servsa.sin_addr) << ':' <<  servsa.sin_port << " connected to control interface\n";
			thread(controlInterface, cli).detach();
		}
	}catch(const exception& e){
		cerr << "Unhandled exception " << e.what() << endl;
	}catch(int e){
		switch(e){
		case 1:
			perror("Control interface server error");
		break;
		default:
			cerr << "Unhandled exception " << e << endl;
		}
	}
	done = true;
	close(serv);
}

void drawROI(cv::Mat& image, cv::RotatedRect& roi, bool carPresent){
	cv::Point2f vertices[4];
	cv::Scalar color = (carPresent) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
	roi.points(vertices);
	for (int i = 0; i < 4; i++)
		line(image, vertices[i], vertices[(i+1)%4], color);
}

void processing(vector<char> buff, parkingConfig conf){
	vector<int> quality;
	stringstream ss;
	vector<bool> status;
	try{
		quality.push_back(CV_IMWRITE_JPEG_QUALITY);
		quality.push_back(conf.JPEGQuality);
		ss << "/dev/shm/" << std::this_thread::get_id() << ".jpg";

		cv::Mat frame = cv::imdecode(buff, 1);
		cv::Mat output;
		if(!conf.edgesOnly)
			frame.copyTo(output);

		cvtColor( frame, frame, CV_BGR2GRAY );
		blur( frame, frame, cv::Size(conf.blurSize, conf.blurSize) );
		Canny( frame, frame, conf.edge[0], conf.edge[1], conf.sobelSize );

		if(conf.edgesOnly)
			cvtColor( frame, output, CV_GRAY2BGR );

		for(uint num = 0; num < conf.ROI.size(); num++)
	        {
			bool carPresent = false;
			float value = filling(frame, conf, num);
			if(value > conf.treshold)
				carPresent = true;
			else carPresent = false;
			status.push_back(carPresent);
			//cout << num << ": " << carPresent << "\t" << value << endl;
			drawROI(output, conf.ROI[num], carPresent);
		}
		lotMutex.lock();
			lotStatus = status;
		lotMutex.unlock();
		if(conf.timestamp)
			cv::putText(output, return_current_time_and_date(), cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 2);
		imwrite(ss.str(), output, quality);
		system(("mv " + ss.str() + " /dev/shm/current.jpg").c_str());
	}catch(const exception& e){
		cerr << "Exception during image processing " << e.what() << endl;
	}

}

float filling(cv::Mat& img, parkingConfig& conf, int zone){
	unsigned int pixels = 0;
	cv::Rect bounding = conf.ROI[zone].boundingRect();
	cv::Mat cimg = img(bounding);
	cv::bitwise_and(cimg, conf.ROIMask[zone], cimg);

	for( int y = 0; y < cimg.rows; y++ )
		for( int x = 0; x < cimg.cols; x++ )
			pixels += cimg.at<uchar>(y, x);
	return ((pixels/255)*100.0)/conf.ROIArea[zone];
}

std::string return_current_time_and_date()
{
	std::stringstream ss;
	char buffer[32];
	time_t t = time(NULL);
	strftime(buffer,sizeof(buffer),"%d.%m.%Y %H:%M:%S", localtime(&t));
	ss << buffer;
	return ss.str();
}

int main(int argc, char* argv[])
{
	string configName = "config.json";
	if(argc >= 2)
		configName = argv[1];
	config = new parkingConfig(configName);
	uint maxThreads = config->maxThreads;
	thread server(controlInterfaceServer);
	thread client(reportInterface, *config);
	try{
		queue<thread> threads;
		while (!done){
			int len;
			cin.read((char*)&len, 4);
			vector<char> buff(len);
			cin.read(buff.data(), len);
			while(threads.size() >= maxThreads){
				threads.front().join();
				threads.pop();
			}
			configMutex.lock();
			maxThreads = config->maxThreads;
			threads.push(thread(processing, buff, *config));
			configMutex.unlock();
			if (cin.fail() || cin.bad() || cin.eof())
				done = true;
		}
		while(threads.size() >= 0){
			threads.front().join();
			threads.pop();
		}
	}
	catch (int e){
		switch (e){
		default:
		cerr << "Unhandled exception " << e << endl;
		}
	}
	catch (const exception& e){
		cerr << "Unhandled exception " << e.what() << endl;
	}
	server.join();
	return 0;
}


