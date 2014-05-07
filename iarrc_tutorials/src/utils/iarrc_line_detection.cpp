#include <stdio.h>
#include <ros/ros.h>
#include <ros/subscriber.h>
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdlib.h>

std::string img_file;
int low_Threshold = 5;
int ratio = 4;
int sigma=0;
ros::Publisher img_pub;
sensor_msgs::Image rosimage;
int erosion_size=3;
int erosion_type=0;

using namespace std;
using namespace cv;

// ROS image callback
void ImageSaverCB(const sensor_msgs::Image::ConstPtr& msg) {
	
	cv_bridge::CvImagePtr cv_ptr;
	//cv_bridge::CvImagePtr cv_ptr_2;
	Mat edgeOp;	
	Mat blurImg;
	Mat grayscaleImg;
	Mat out;
	Mat edgeCopy;
	Mat edgeCopy2;
	int thresh = 100;
	int max_thresh = 255;
	vector <vector<Point>> contours;
	vector <Vec4i> hierarchy;
	RNG rng(10305);


	ROS_INFO("Received image with %s encoding", msg->encoding.c_str());

	// Convert ROS to OpenCV
	try {
		cv_ptr = cv_bridge::toCvCopy(msg, msg->encoding);
	} catch (cv_bridge::Exception& e) {
		ROS_ERROR("cv_bridge exception: %s", e.what());
		return;
	}
	int width = cv_ptr->image.cols;
	int height = cv_ptr->image.rows;
	
	//crop image
	Rect myRect = Rect(0,3*height/4,width,height/4);
	Mat tmp;
	Mat(cv_ptr->image,myRect).copyTo(tmp);
	Mat element = getStructuringElement( erosion_type,Size( 2*erosion_size + 1, 2*erosion_size+1 ),Point( erosion_size, erosion_size ) );

	GaussianBlur(tmp,blurImg,Size(9,9),sigma,sigma);
	cvtColor(blurImg,grayscaleImg,CV_BGR2GRAY);
	Canny(grayscaleImg,edgeOp,low_Threshold,low_Threshold*ratio,3);
	edgeOp.copyTo(edgeCopy);
	dilate(edgeCopy,edgeCopy,element);
	erode(edgeCopy,edgeCopy,element);
	edgeCopy.copyTo(edgeCopy2);
	addWeighted(edgeOp,0.5,grayscaleImg,0.5,0,out);	
	findContours(edgeCopy2,contours,hierarchy,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,Point(0,0));

	Mat drawing = Mat::zeros(edgeOp.size(),CV_8UC3);
	for(int i=0; i < contours.size();i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
		 drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
	}

	//ROS_INFO("Hi");

	// namedWindow("Contours",WINDOW_AUTOSIZE);
 // 	imshow("Contours",drawing);

	namedWindow("Edges",WINDOW_AUTOSIZE);
 	imshow("Edges",edgeOp);
	namedWindow("Eroded/Dilated",WINDOW_AUTOSIZE);
 	imshow("Eroded/Dilated",edgeCopy);
 	waitKey(0);
 	cv_ptr->image=drawing;
    cv_ptr->encoding="bgr8";
    //cv_ptr->encoding="mono8";
    cv_ptr->toImageMsg(rosimage);
    img_pub.publish(rosimage);

	
}

void help(std::ostream& ostr) {
	ostr << "Usage: iarrc_line_detection _img_topic:=<image-topic>" << std::endl;
}

int main(int argc, char* argv[]) {
    ros::init(argc, argv, "iarrc_line_detection");
	ros::NodeHandle nh;
    ros::NodeHandle nhp("~");

    // FIXME: Not expected behavior
    if(argc >= 2) {
	    help(std::cerr);
	    exit(1);
    }

    std::string img_topic;
    nhp.param(std::string("img_topic"), img_topic, std::string("/ps3_eye/image_raw"));
    nhp.param(std::string("img_file"), img_file, std::string("iarrc_image.png"));

    ROS_INFO("Image topic:= %s", img_topic.c_str());
    ROS_INFO("Image file:= %s", img_file.c_str());

    // Subscribe to ROS topic with callback
    ros::Subscriber img_saver_sub = nh.subscribe(img_topic, 1, ImageSaverCB);
    img_pub = nh.advertise<sensor_msgs::Image>("/image_lines", 1);//image publisher

    ROS_INFO("Hi");

	ROS_INFO("IARRC image saver node ready.");
	ros::spin();
	ROS_INFO("Shutting down IARRC image saver node.");
    return 0;
}
