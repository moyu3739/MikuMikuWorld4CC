#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <queue>

#define DEL(ptr) if (ptr != nullptr) {delete ptr; ptr = nullptr;}
#define DEL_THREAD(ptr) if (ptr != nullptr) {ptr->join(); delete ptr; ptr = nullptr;}


enum PlayMode{
    BACKGROUND,
    WINDOW
};


class TimeDrivenPlayer{
public:
    TimeDrivenPlayer() {}

    ~TimeDrivenPlayer(){
        CloseVideo();
    }

    void OpenVideo(const std::string& path);

    void CloseVideo();

    void HideVideo();

    void ShowVideo();

    void ShowVideoOnPlayMode(PlayMode play_mode);

    bool Running() const{
        return window_running || background_running;
    }

    bool WindowRunning() const{
        return window_running;
    }

    bool BackgroundRunning() const{
        return background_running;
    }

    bool Playable() const{
        return cap != nullptr;
    }

    const cv::Mat& GetFrame() const{
        return frame_mat;
    }

    void LockFrame(){
        frame_mtx.lock();
    }

    void UnlockFrame(){
        frame_mtx.unlock();
    }

    void Update(bool playing, double now){
		this->playing = playing;
        this->now = now;
    }

private:
    static void WindowPlay(TimeDrivenPlayer* player);

    static void MonitorWindowSize(TimeDrivenPlayer* player);

    static void ResizeFrame(const cv::Mat& src, cv::Mat& dst, int window_width, int window_height);

    static void Capture(TimeDrivenPlayer* player);

    static uint64_t HashMat(const cv::Mat& mat);

    void ShowVideoOnBackground();

    void ShowVideoOnWindow();

public:
    double fps;
    double interval;
    int frame_count;
    int video_width;
    int video_height;
    PlayMode play_mode = BACKGROUND;
    
private:
    std::atomic<bool> window_running = false;
    std::atomic<bool> background_running = false;

    std::atomic<int> window_width = 800;
	std::atomic<int> window_height = 600;
    
    std::atomic<bool> playing = false;
    std::atomic<double> now = 0.0;
    std::atomic<int> frame_id = -1; // `frame_mat` 的帧序号
    cv::Mat frame_mat;
    std::mutex frame_mtx;
    
    std::atomic<double> real_render_fps = 0.0;
    std::queue<double> history_render_frame;

    cv::VideoCapture* cap = nullptr;
	std::thread* thread_window_play = nullptr;
    std::thread* thread_monitor_window = nullptr;
    std::thread* thread_capture = nullptr;
};
