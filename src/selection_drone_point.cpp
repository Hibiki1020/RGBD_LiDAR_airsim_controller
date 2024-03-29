#include "RGBD_LiDAR_airsim_controller/selection_drone_point.h"


GetInferenceImage::GetInferenceImage(){
    std::cout << "Get Inference Image" << std::endl;

    //client initialization
    client_initialization();

    /*camera list*/
	_list_camera = {
		"camera_rgb",
        "camera_depth",
	};


    load_csv();

    std::cout << "Initialization Done" << std::endl;
}

GetInferenceImage::~GetInferenceImage(){}

void GetInferenceImage::client_initialization(){
    //connect
    _client.confirmConnection();

    //reset
    std::cout << "Reset" << std::endl;
    _client.reset();

    //pose
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    update_state();

    /*weather*/
	if(_random_weather)	_client.simEnableWeather(true);
	_list_weather = {
		msr::airlib::WorldSimApiBase::WeatherParameter::Rain,
		msr::airlib::WorldSimApiBase::WeatherParameter::Roadwetness,
		msr::airlib::WorldSimApiBase::WeatherParameter::Snow,
		msr::airlib::WorldSimApiBase::WeatherParameter::RoadSnow,
		msr::airlib::WorldSimApiBase::WeatherParameter::MapleLeaf,
		msr::airlib::WorldSimApiBase::WeatherParameter::RoadLeaf,
		msr::airlib::WorldSimApiBase::WeatherParameter::Dust,
		msr::airlib::WorldSimApiBase::WeatherParameter::Fog
		// msr::airlib::WorldSimApiBase::WeatherParameter::Enabled
	};
	/*time*/
	// _client.simSetTimeOfDay(true, "2020-01-01 17:00:00", false, 0.01, 1.0, true);
}

void GetInferenceImage::update_state(){
    /*pose*/
	_pose = _client.simGetVehiclePose();

    /*
	std::cout << "Pose: " << std::endl;
	std::cout << " Position: "	//Eigen::Vector3f
		<< _pose.position.x() << ", "
		<< _pose.position.y() << ", "
		<< _pose.position.z() << std::endl;
	std::cout << " Orientation: "	//Eigen::Quaternionf
		<< _pose.orientation.w() << ", "
		<< _pose.orientation.x() << ", "
		<< _pose.orientation.y() << ", "
		<< _pose.orientation.z() << std::endl;
    */

	/*imu*/
	_imu = _client.getImuData();

    /*
	std::cout << "IMU: " << std::endl;
	std::cout << " linear_acceleration: "	//Eigen::Vector3f
		<< _imu.linear_acceleration.x() << ", "
		<< _imu.linear_acceleration.y() << ", "
		<< _imu.linear_acceleration.z() << std::endl;
    */
}


std::vector<std::string> GetInferenceImage::split(std::string& input, char delimiter)
{
    std::istringstream stream(input);
    std::string field;
    std::vector<std::string> result;
    while (getline(stream, field, delimiter)) {
        result.push_back(field);
    }
    return result;
}

bool GetInferenceImage::load_csv(){
    bool checker = true;
    std::string place_csv_path = place_csv_root_path + place_csv_name;

    std::ifstream csv_file(place_csv_path);
    if(!csv_file){
        std::cout << "CSV File " << place_csv_path << " not found. Exit..." << std::endl;
        checker = false;
    }
    else{
        std::string line;
        while( getline(csv_file, line)){
            std::vector<std::string> tmp_data = split(line, ',');
            csv_data.push_back(tmp_data);
        }

        csv_data_size = csv_data.size();
    }
    return checker;
}

void GetInferenceImage::spin(){
    std::cout << "Start Sampling" << std::endl;

    bool continue_func = true;

    int i = 0;
    int accepted_num = 0;

    std::string csv_path = place_csv_root_path + "/" + place_csv_name;
    std::ofstream final_csvfile(csv_path, std::ios::app);

    while(continue_func==true){
        std::cout << "----SAMPLE " << i << " -----" << std::endl;
        if(_random_weather) random_weather();
        
        random_place();
        //random_place_from_csv();
        update_state();

        int save_image_checker = image_checker();

        if(save_image_checker==0){
            std::cout << "Save Place" << std::endl;
            std::vector<float> tmp_place{_pose.position.x(), _pose.position.y(), _pose.position.z(), _pose.orientation.x(), _pose.orientation.y(), _pose.orientation.z(), _pose.orientation.w()};

            accepted_num += 1;
            std::cout << "Accepted Place: " << accepted_num << std::endl;
            
            std::string tmp_x = std::to_string(tmp_place[0]);
            std::string tmp_y = std::to_string(tmp_place[1]);
            std::string tmp_z = std::to_string(tmp_place[2]);
    
            std::string tmp_qx = std::to_string(tmp_place[3]);
            std::string tmp_qy = std::to_string(tmp_place[4]);
            std::string tmp_qz = std::to_string(tmp_place[5]);
            std::string tmp_qw = std::to_string(tmp_place[6]);

            final_csvfile << tmp_x << ","
                << tmp_y << ","
                << tmp_z << ","
                << tmp_qx << ","
                << tmp_qy << ","
                << tmp_qz << ","
                << tmp_qw << std::endl;

        }
        else if(save_image_checker==1){
            std::cout << "Not Appropriate Position and Reset Position" << std::endl;
        }
        else{
            std::cout << "Place Sampling Done. Save place as .csv file" << std::endl;
            continue_func = false;
        }
        
        _client.simPause(false); //位置更新に必須
        
        i++;
    }

    final_csvfile.close();

}

void GetInferenceImage::eular_to_quat(float r, float p, float y, Eigen::Quaternionf& q)
{
	q = Eigen::AngleAxisf(y, Eigen::Vector3f::UnitZ())
		* Eigen::AngleAxisf(p, Eigen::Vector3f::UnitY())
		* Eigen::AngleAxisf(r, Eigen::Vector3f::UnitX());
}

void GetInferenceImage::random_weather(){
    /*reset*/
	for(size_t i=0; i<_list_weather.size(); ++i)	_client.simSetWeatherParameter(_list_weather[i], 0.0);
	/*random*/
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<> uid_index(0, _list_weather.size()-1);
	std::uniform_real_distribution<> urd_val(0.0, 1.0); //天候の程度を表す？
	for(int i=1; i<=uid_index(mt); ++i){
		int weather_index = uid_index(mt);
		double weather_val = urd_val(mt);
		_client.simSetWeatherParameter(_list_weather[weather_index], weather_val);
	}
}

int GetInferenceImage::random_int(int min, int max){
    int num = 0;
    std::mt19937 mt{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max);

    num = dist(mt);
    return num;
}

float GetInferenceImage::check_deg(float deg){
    float result = 0;
    if(deg > M_PI){
        float tmp = deg - M_PI;
        result = -1.0 * M_PI + tmp;
        std::cout << "upper" << std::endl;
    }
    else if( deg < -1.0 * M_PI){
        float tmp = deg + M_PI;
        result = M_PI + tmp;
        std::cout << "lower" << std::endl;
    }
    else{
        result = deg;
    }

    return result;
}

std::vector<float> GetInferenceImage::string_to_float(std::vector<std::string> tmp_place_csv_data){
    std::vector<float> farray;

    for(size_t i=0; i < tmp_place_csv_data.size(); ++i){
        std::string tmp_string = tmp_place_csv_data[i];
        float tmp_f = std::stof(tmp_string);

        farray.push_back(tmp_f);
    }

    return farray;
}

void GetInferenceImage::random_place_from_csv(){
    int place_index = random_int(0, csv_data.size());

    std::vector<std::string> tmp_place_csv_data = csv_data[place_index];
    std::vector<float> place_data = string_to_float(tmp_place_csv_data);


    std::random_device rd;
	std::mt19937 mt(rd());

    std::uniform_real_distribution<> urd_roll(_roll_min, _roll_max);
    std::uniform_real_distribution<> urd_pitch( _pitch_min, _pitch_max);
    std::uniform_real_distribution<> urd_yaw(_yaw_min, _yaw_max);

    Eigen::Vector3f position(place_data[0], place_data[1], place_data[2]);

    //float roll = check_deg( urd_roll(mt) );
    //float pitch = check_deg( urd_pitch(mt) );
    float roll = 0.0;
    float pitch = 0.0;
    float yaw = check_deg( urd_yaw(mt) );

    tmp_roll = roll;
    tmp_pitch = pitch;
    tmp_yaw = yaw;

    Eigen::Quaternionf orientation;
    eular_to_quat(roll, pitch, yaw, orientation);
    msr::airlib::Pose goal = msr::airlib::Pose(position, orientation);

    std::cout << "Move to: " << std::endl
		<< " XYZ[m]: " 
			<< goal.position.x() << ", "
			<< goal.position.y() << ", "
			<< goal.position.z() << std::endl
		<< " RPY[deg]: "
			<< roll/M_PI*180.0 << ", "
			<< pitch/M_PI*180.0 << ", "
			<< yaw/M_PI*180.0 << std::endl
		<< " Quat: "
			<< goal.orientation.w() << ", "
			<< goal.orientation.x() << ", "
			<< goal.orientation.y() << ", "
			<< goal.orientation.z() << std::endl;

    /*teleport*/
	_client.simSetVehiclePose(goal, true);
	std::this_thread::sleep_for(std::chrono::milliseconds(_wait_time_millisec));
}

void GetInferenceImage::random_place(){
    /*random*/
	std::random_device rd;

	std::mt19937 mt(rd());

	std::uniform_real_distribution<> urd_x(-_x_range, _x_range);
	std::uniform_real_distribution<> urd_y(-_y_range, _y_range);
	std::uniform_real_distribution<> urd_z(_z_min, _z_max);

    std::uniform_real_distribution<> urd_roll(-_roll_min, _roll_max);
    std::uniform_real_distribution<> urd_pitch( _pitch_min, _pitch_max);
    std::uniform_real_distribution<> urd_yaw(_yaw_min, _yaw_max);

    Eigen::Vector3f position( urd_x(mt), urd_y(mt), urd_z(mt));
    //float roll = urd_roll(mt);
    //float pitch = urd_yaw(mt);

    float roll = 0.0;
    float pitch = 0.0;
    float yaw = urd_yaw(mt);

    tmp_roll = roll;
    tmp_pitch = pitch;
    tmp_yaw = yaw;

    Eigen::Quaternionf orientation;
    eular_to_quat(roll, pitch, yaw, orientation);
    msr::airlib::Pose goal = msr::airlib::Pose(position, orientation);

    std::cout << "Move to: " << std::endl
		<< " XYZ[m]: " 
			<< goal.position.x() << ", "
			<< goal.position.y() << ", "
			<< goal.position.z() << std::endl
		<< " RPY[deg]: "
			<< roll/M_PI*180.0 << ", "
			<< pitch/M_PI*180.0 << ", "
			<< yaw/M_PI*180.0 << std::endl
		<< " Quat: "
			<< goal.orientation.w() << ", "
			<< goal.orientation.x() << ", "
			<< goal.orientation.y() << ", "
			<< goal.orientation.z() << std::endl;

    /*teleport*/
	_client.simSetVehiclePose(goal, true);
	std::this_thread::sleep_for(std::chrono::milliseconds(_wait_time_millisec));
    _client.simPause(true);
}

int GetInferenceImage::image_checker(){
    
    std::cout << "Checking Image" << std::endl;
    
    int result = 0;
    std::vector<msr::airlib::ImageCaptureBase::ImageRequest> list_request(_list_camera.size()-1);

    for(size_t i=0; i<_list_camera.size()-1; ++i){
		list_request[i] = msr::airlib::ImageCaptureBase::ImageRequest(_list_camera[0], msr::airlib::ImageCaptureBase::ImageType::Scene, false, false);
	}

    std::vector<msr::airlib::ImageCaptureBase::ImageResponse> list_response = _client.simGetImages(list_request);
    
    cv::Mat img_cv = cv::Mat(list_response[0].height, list_response[0].width, CV_8UC3);
    for(int row=0; row<list_response[0].height; ++row){
		for(int col=0; col<list_response[0].width; ++col){
			img_cv.at<cv::Vec3b>(row, col)[0] = list_response[0].image_data_uint8[3*row*list_response[0].width + 3*col + 0];
			img_cv.at<cv::Vec3b>(row, col)[1] = list_response[0].image_data_uint8[3*row*list_response[0].width + 3*col + 1];
			img_cv.at<cv::Vec3b>(row, col)[2] = list_response[0].image_data_uint8[3*row*list_response[0].width + 3*col + 2];
		}
    }
    
    std::cout << "Do you want to save this place's data?" << std::endl
        << "Answer Yes[y] or No[n] or Quit[q]." << std::endl;

    cv::imshow("Place Photo", img_cv);
    int answer = cv::waitKey(0);

    if(answer == 0x79 /* y */){
        result = 0;
        std::cout << "Yes" << std::endl;
    }
    else if(answer == 0x6E /* n */){
        result = 1;
        std::cout << "No" << std::endl;
    }
    else if(answer == 0x71 /* q */){
        result = 2;
        std::cout << "Quit" << std::endl;
    }
    else{
        result = 1;
        std::cout << "No" << std::endl;
    }

    cv::destroyAllWindows();

    return result;
}

int main(){
    GetInferenceImage get_inference_image;
    get_inference_image.spin();

    return 0;
}