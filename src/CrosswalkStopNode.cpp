#include "crosswalk_stop/CrosswalkStopNode.h"

using namespace std;
using namespace cv;

CrosswalkStopNode::CrosswalkStopNode()
{
	nh_ = ros::NodeHandle("~");

	/* if NodeHangle("~"), then (write -> /lane_detector/write)	*/
	control_pub_ = nh_.advertise<ackermann_msgs::AckermannDriveStamped>("ackermann", 10);

	image_sub_ = nh_.subscribe("/usb_cam/image_raw", 1, &CrosswalkStopNode::imageCallback, this);

	getRosParamForUpdate();
}


CrosswalkStopNode::CrosswalkStopNode(String path)
	: test_video_path(path)
{}


void CrosswalkStopNode::imageCallback(const sensor_msgs::ImageConstPtr& image)
{

	try{
		parseRawimg(image, frame);
	} catch(const cv_bridge::Exception& e) {
		ROS_ERROR("cv_bridge exception: %s", e.what());
		return ;
	} catch(const std::runtime_error& e) {
		cerr << e.what() << endl;
	}

	crosswalk_start();
	steer_control_value_ = 0;


/* ////////////////original code////////////////////
	//getRosParamForUpdate();
	//cout << "crosswalk_stop_node" << endl;
	bool x = parkingstart();
	cout << "x : " << x << endl;
	if(!parkingstart()){
			cout << "do lane detecting" << endl;
			steer_control_value_ = laneDetecting();
	}
	else{
		cout << "parking" << endl;
		steer_control_value_ = 0;
	}
	//////////////////////////////////////////////// */
	cout << "throttle : " << throttle_ << "steer : " << steer_control_value_ << endl;

	ackermann_msgs::AckermannDriveStamped control_msg = makeControlMsg();

	control_pub_.publish(control_msg);
	
}


void CrosswalkStopNode::getRosParamForUpdate()
{
	nh_.getParam("throttle", throttle_);
}


ackermann_msgs::AckermannDriveStamped CrosswalkStopNode::makeControlMsg()
{
	ackermann_msgs::AckermannDriveStamped control_msg;
	//control_msg.drive.steering_angle = steer_control_value;
	control_msg.drive.steering_angle = steer_control_value_;
	control_msg.drive.speed = throttle_;
	return control_msg;
}


bool CrosswalkStopNode::crosswalk_start()
{
	int throttle;
	int ncols = frame.cols;
	int nrows = frame.rows;

	int64 t1 = getTickCount();
	frame_count++;

	resize(frame, frame, Size(ncols / resize_n, nrows / resize_n));
	lanedetector.filter_colors(frame, img_filtered);
	img_denoise = lanedetector.deNoise(img_filtered);
/*
	//indoor test
	bitwise_not(img_mask2,img_mask2); // test for black white invert
*/
	img_mask = crosswalk_stop.mask(img_filtered);
	imshow("original", frame);
	imshow("color_filter", img_filtered);
	imshow("img_filter", img_mask);

	cout << "crosswalk detect start" << endl;
	if(crosswalk_stop.detectstoppoint(img_mask, frame, 1, 2)){
		throttle_ = 0;
		if(!cwross_stop){
				cwross_stop = true;
			}
		return true;
	}
	crosswalk_stop.VisualizeCircle(frame, img_mask, 2);

	int64 t2 = getTickCount();
	double ms = (t2 - t1) * 1000 / getTickFrequency();
	sum += ms;
	avg = sum / (double)frame_count;
	//cout << "it took :  " << ms << "ms." << "average_time : " << avg << " frame per second (fps) : " << 1000 / avg << endl;
	waitKey(3);
	ROS_INFO("it took : %6.2f [ms].  average_time : %6.2f [ms].  frame per second (fps) : %6.2f [frame/s].  \n", ms, avg, 1000 / avg );

	return false;
}


void CrosswalkStopNode::parseRawimg(const sensor_msgs::ImageConstPtr& ros_img, cv::Mat& cv_img)
{
	cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(ros_img, sensor_msgs::image_encodings::BGR8);

	cv_img = cv_ptr->image;

	if (cv_img.empty()) {
		throw std::runtime_error("frame is empty!");
	}
}


bool CrosswalkStopNode::run_test()
{
	if(test_video_path.empty())
	{
		ROS_ERROR("Test is failed. video path is empty! you should set video path by constructor argument");
		return false;
	}

	VideoCapture cap;
	//cap.open("../../kasa.mp4");
	cap.open(test_video_path);

	if (!cap.isOpened())
	{
		ROS_ERROR("Test is failed. video is empty! you should check video path (constructor argument is correct)");
		return false;
	}

	while (1) {
		// Capture frame
		if (!cap.read(frame))
			break;


		int ncols = frame.cols;
		int nrows = frame.rows;


		int64 t1 = getTickCount();
		frame_count++;

			crosswalk_start();
			steer_control_value_ = 0;


		/* ////////////////original code////////////////////
			//getRosParamForUpdate();
			//cout << "crosswalk_stop_node" << endl;
			bool x = parkingstart();
			cout << "x : " << x << endl;
			if(!parkingstart()){
					cout << "do lane detecting" << endl;
					steer_control_value_ = laneDetecting();
			}
			else{
				cout << "parking" << endl;
				steer_control_value_ = 0;
			}
			////////////////////////////////////////////////*/
			cout << "throttle : " << throttle_ << "steer : " << steer_control_value_ << endl;

		waitKey(25);
		//cout << "it took :  " << ms << "ms." << "average_time : " << avg << " frame per second (fps) : " << 1000 / avg << endl;

	}

}
