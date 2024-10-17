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
#define ROUND(x) (static_cast<int>((x) + 0.5))


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

    bool FrameReady(unsigned int backend_id) const;

    void SetFrameHandled(unsigned int backend_id){
        frame_ready &= ~(1u << backend_id);
    }

private:
    static void WindowPlay(TimeDrivenPlayer* player);

    static void MonitorWindowSize(TimeDrivenPlayer* player);

    static void ResizeFrame(const cv::Mat& src, cv::Mat& dst, int window_width, int window_height);

    static void Capture(TimeDrivenPlayer* player);

    void ShowVideoOnBackground();

    void ShowVideoOnWindow();

    void SetFrameReady(){
        frame_ready = 0xFFFFFFFF;
    }

    // 更新渲染帧率
    void UpdateHistoryRenderFrame(){
        const int window_size = fps / 2;
        history_render_frame.push(now);
        if (history_render_frame.size() > window_size) history_render_frame.pop();
    }

    // 计算渲染帧率
    double RenderFPS(){
        if (history_render_frame.size() < 2) return 0;
        double fps = (history_render_frame.size() - 1) / (history_render_frame.back() - history_render_frame.front());
        if (fps < 0) return 0;
        else return fps;
    }

    // 更新取帧帧率
    void UpdateHistoryCaptureFrame(){
        const int window_size = fps / 2;
        history_capture_frame.push(now);
        if (history_capture_frame.size() > window_size) history_capture_frame.pop();
    }

    // 计算取帧帧率
    double CaptureFPS(){
        if (history_capture_frame.size() < 2) return 0;
        double fps = (history_capture_frame.size() - 1) / (history_capture_frame.back() - history_capture_frame.front());
        if (fps < 0) return 0;
        else return fps;
    }

public:
    double fps;
    double interval;
    int frame_count;
    int video_width;
    int video_height;
    PlayMode play_mode = BACKGROUND;
    bool fps_lock = false;
    
private:
    std::atomic<bool> window_running = false;
    std::atomic<bool> background_running = false;

    std::atomic<int> window_width = 800;
	std::atomic<int> window_height = 600;
    
    std::atomic<bool> playing = false;
    std::atomic<double> now = 0.0;
    std::atomic<int> frame_id = -1; // `frame_mat` 的帧序号

    std::atomic<unsigned int> frame_ready; // 用于标记帧是否已经被处理，第 i 位为 1 表示第 i 个后端已经处理了该帧
    cv::Mat frame_mat;
    std::mutex frame_mtx;
    
    std::queue<double> history_render_frame;
    std::queue<double> history_capture_frame;

    cv::VideoCapture* cap = nullptr;
	std::thread* thread_window_play = nullptr;
    std::thread* thread_monitor_window = nullptr;
    std::thread* thread_capture = nullptr;
};
