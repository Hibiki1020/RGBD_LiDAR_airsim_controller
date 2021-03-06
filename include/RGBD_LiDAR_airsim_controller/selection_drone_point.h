#ifndef __SELECTION_DRONE_POINT_H
#define __SELECTION_DRONE_POINT_H

#include <iostream>
#include "api/RpcLibClientBase.hpp"
#include <fstream>
#include <opencv2/opencv.hpp>
#include"cnpy.h"
#include <time.h>

class GetInferenceImage{
    private:
        //client
        msr::airlib::RpcLibClientBase _client;
		/*state*/
		msr::airlib::Pose _pose;
		msr::airlib::ImuBase::Output _imu;
        /*list*/
		std::vector<std::string> _list_camera;
		std::vector<msr::airlib::WorldSimApiBase::WeatherParameter> _list_weather;


        //Parameters
        const bool _random_weather = false;
        const int _wait_time_millisec = 300;

        /*parameter-pose*/
		const float _x_range = 25.0;	//Neighborhood: 200, SoccerField: 350 AirSimNH: 250
		const float _y_range = 25.0;	//Neighborhood: 200, SoccerField: 300 AirSimNH: 250
		const float _z_min = -2.5; //Usualy -1.6 Building99 -2.5
		const float _z_max = 1.9; //Usually -1.4 Builging99 1.0

        const float _roll_max = 0.01 * M_PI;
        const float _roll_min = -0.01 * M_PI;

        const float _pitch_max = 0.01* M_PI;
        const float _pitch_min = -0.01 * M_PI;

        const float _yaw_max = M_PI;
        const float _yaw_min = -1.0 * M_PI;

        const float resolution = M_PI/180.0 * 1.0;

        float tmp_roll = 0.0;
        float tmp_pitch = 0.0;
        float tmp_yaw = 0.0;

        //csv place data
        std::string place_csv_root_path = "/home/amsl/cpp/RGBD_LiDAR_airsim_controller/place_data/RGBD_LiDAR_Attitude_Estimation/Building99";
        std::string place_csv_name = "random_place2022_02_07_2.csv";
        std::vector< std::vector<std::string> > csv_data;
        size_t csv_data_size;

    public:
        GetInferenceImage();
        void client_initialization();
        void update_state();
        bool load_csv();
        std::vector<std::string> split(std::string& input, char delimiter);

        //Loop Process Function
        void spin();
        void random_weather();
        void random_place();
        void random_place_from_csv();
        int random_int(int min, int max);
        float check_deg(float deg);
        std::vector<float> string_to_float(std::vector<std::string> tmp_place_csv_data);
        void eular_to_quat(float r, float p, float y, Eigen::Quaternionf& q);
        int image_checker();

        ~GetInferenceImage();

};

#endif