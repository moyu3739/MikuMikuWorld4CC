#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>


class TimeDrivenPlayer{
public:
    TimeDrivenPlayer() {
        window_width = 800;
        window_height = 600;
    }

    ~TimeDrivenPlayer(){
        CloseVideo();
    }

    void OpenVideo(const std::string& path);

    void CloseVideo();

    void Run(){
        capture_thread = new std::thread(Capture, this, static_cast<int>(fps / 2));
        play_thread = new std::thread(Play, this);
        window_size_thread = new std::thread(MonitorWindowSize, this);
    }

    bool Running(){
        return running;
    }

    void Update(bool playing, double now){
		this->playing = playing;
        this->now = now;
    }

private:
    static void Play(TimeDrivenPlayer* player);

    static void MonitorWindowSize(TimeDrivenPlayer* player);

    static cv::Mat ResizeFrame(const cv::Mat& frame, int window_width, int window_height);

    static void Capture(TimeDrivenPlayer* player, int chase_limit);

public:
    double fps;
    double interval;

private:
    std::atomic<int> window_width = 800;
	std::atomic<int> window_height = 600;

    std::atomic<bool> running;
    std::atomic<bool> playing;
    std::mutex frame_mtx;

    std::atomic<double> real_capture_fps = 0.0;
    std::atomic<double> real_render_fps = 0.0;
    std::atomic<int> frame_id = -1; // 刚显示过的帧
    std::atomic<double> now = 0.0;
	std::queue<double> history_capture_frame;
    std::queue<double> history_render_frame;

    cv::Mat frame;
    cv::VideoCapture* cap = nullptr;
	std::thread* play_thread = nullptr;
    std::thread* window_size_thread = nullptr;
    std::thread* capture_thread = nullptr;
};
