#include "TimeDrivenPlayer.h"


void TimeDrivenPlayer::OpenVideo(const std::string& path){
    // std::vector<int> params = {cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_ANY};
    // cap = new cv::VideoCapture(path, cv::CAP_FFMPEG, params);
    cap = new cv::VideoCapture(path);
    if (!cap->isOpened()) throw std::runtime_error("open video failed");

    fps = cap->get(cv::CAP_PROP_FPS);
    interval = 1.0 / fps;
    frame_count = cap->get(cv::CAP_PROP_FRAME_COUNT);
    video_width = cap->get(cv::CAP_PROP_FRAME_WIDTH);
    video_height = cap->get(cv::CAP_PROP_FRAME_HEIGHT);
    playing = false;
    real_render_fps = 0;
    frame_id = -1; // 设置一个不存在的帧序号，以便在下一次抓取时强制更新帧
}

void TimeDrivenPlayer::CloseVideo(){
    HideVideo();
    DEL(cap);
}

void TimeDrivenPlayer::HideVideo(){
    window_running = false;
    background_running = false;
    history_render_frame = std::queue<double>();
    DEL_THREAD(thread_capture);
    DEL_THREAD(thread_window_play);
    DEL_THREAD(thread_monitor_window);
}

void TimeDrivenPlayer::ShowVideo(){
    switch (play_mode){
        case BACKGROUND: ShowVideoOnBackground(); break;
        case WINDOW: ShowVideoOnWindow(); break;
    }
}

void TimeDrivenPlayer::ShowVideoOnPlayMode(PlayMode play_mode){
    this->play_mode = play_mode;
    ShowVideo();
}

void TimeDrivenPlayer::ShowVideoOnBackground(){
    bool previous_running = Running();
    background_running = true;
    window_running = false;

    DEL_THREAD(thread_window_play);
    DEL_THREAD(thread_monitor_window);

    if (!previous_running) { // 重启抓取线程
        DEL_THREAD(thread_capture);
        frame_id = -1; // 设置一个不存在的帧序号，以便在下一次抓取时强制更新帧
        thread_capture = new std::thread(Capture, this);
    }
}

void TimeDrivenPlayer::ShowVideoOnWindow(){
    bool previous_running = Running();
    window_running = true;
    background_running = false;

    DEL_THREAD(thread_window_play);
    DEL_THREAD(thread_monitor_window);

    if (!previous_running) { // 重启抓取线程
        DEL_THREAD(thread_capture);
        frame_id = -1; // 设置一个不存在的帧序号，以便在下一次抓取时强制更新帧
        thread_capture = new std::thread(Capture, this);
    }
    
    thread_window_play = new std::thread(WindowPlay, this);
    thread_monitor_window = new std::thread(MonitorWindowSize, this);
}

void TimeDrivenPlayer::ResizeFrame(const cv::Mat& src, cv::Mat& dst, int window_width, int window_height) {
    // 获取帧的宽高比
    double aspect_ratio = (double)src.cols / (double)src.rows;
	int new_width, new_height;

    // 根据窗口大小计算新的帧大小，同时保持宽高比例
    if (window_width / aspect_ratio <= window_height) {
        new_width = window_width;
        new_height = static_cast<int>(window_width / aspect_ratio);
    } else {
        new_width = static_cast<int>(window_height * aspect_ratio);
        new_height = window_height;
    }

    if (new_width == 0 || new_height == 0) {
        dst = cv::Mat();
        return;
    }

    // 创建一个黑色背景
    dst = cv::Mat(window_height, window_width, src.type(), cv::Scalar(0, 0, 0));

    // 调整帧大小
    cv:: Mat resized_frame;
    resize(src, resized_frame, cv::Size(new_width, new_height));

    // 将调整大小后的帧放置在背景中央
    int x_offset = (window_width - new_width) / 2;
    int y_offset = (window_height - new_height) / 2;
    resized_frame.copyTo(dst(cv::Rect(x_offset, y_offset, new_width, new_height)));
}

void TimeDrivenPlayer::WindowPlay(TimeDrivenPlayer* player){
    cv::namedWindow("Video Player", cv::WINDOW_NORMAL); // 允许调整窗口大小
    cv::setWindowProperty("Video Player", cv::WND_PROP_TOPMOST, 1); // 置顶窗口

    while (player->window_running) {
        double now = player->now;
        int target_frame_id = static_cast<int>((now + player->interval / 2) / player->interval);

        if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1) break;
        // 调整帧大小以适应窗口，同时保持宽高比例
        player->frame_mtx.lock();

        if (!player->frame_mat.empty()) {
            cv::Mat resized_frame;
            ResizeFrame(player->frame_mat, resized_frame, player->window_width, player->window_height);
            if (!resized_frame.empty()) imshow("Video Player", resized_frame);
        }

        player->frame_mtx.unlock();

        // 更新帧率
        if (player->history_render_frame.size() > 1)
            player->real_render_fps = (player->history_render_frame.size() - 1) / (player->history_render_frame.back() - player->history_render_frame.front());
        player->history_render_frame.push(now);
        if (player->history_render_frame.size() > player->fps / 4) player->history_render_frame.pop();

        std::string title;
        // 显示当前视频进度
        int now_int = static_cast<int>(now);
        title += std::to_string(now_int / 60) + ":" + std::to_string(now_int % 60 / 10) + std::to_string(now_int % 10);
        // 帧率
        if (player->playing)
            title += " (" + std::to_string(static_cast<int>(std::min(player->real_render_fps.load(), player->fps) + 0.5)) + " FPS)";
        cv::setWindowTitle("Video Player", title);

        int wait_time = static_cast<int>((target_frame_id * player->interval + player->interval / 2 - now) * 1000);
        cv::waitKey(player->playing ? std::max(1, wait_time / 2) : 100);
    }
    player->window_running = false;

    // 检查窗口是否需要 destroy
    if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) > 0)
        cv::destroyWindow("Video Player");
}

void TimeDrivenPlayer::Capture(TimeDrivenPlayer* player){
    int chase_limit = static_cast<int>(player->fps * 0.5);
    while (player->Running()) {
        double now = player->now;
        int target_frame_id = static_cast<int>((now + player->interval / 2) / player->interval);
        if (target_frame_id < 0) target_frame_id = 0;
        if (target_frame_id >= player->frame_count) target_frame_id = player->frame_count - 1;
        int offset = target_frame_id - player->frame_id;

        // 处于暂停状态
        if (!player->playing){
            if (offset != 0) { // 暂停状态下，只要不是当前帧，就跳转
                player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
                player->frame_mtx.lock();

                player->cap->read(player->frame_mat);

                player->frame_mtx.unlock();
                player->frame_id = target_frame_id;
            }
        }
		// 处于播放状态
		else {
			if (offset == 0) {}
			else if (offset == 1) {
                player->frame_mtx.lock();
                player->cap->read(player->frame_mat);
                player->frame_mtx.unlock();
				player->frame_id = target_frame_id;
			}
			else if (1 < offset && offset < chase_limit) { // 如果进度落后但不超过 chase_limit 帧，则快进
                player->frame_mtx.lock();
                player->cap->grab();
                player->frame_mtx.unlock();
				player->frame_id++;
			}
			else { // 如果进度落后超过 chase_limit 帧或超前，则跳转
				player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
                player->frame_mtx.lock();
				player->cap->read(player->frame_mat);
                player->frame_mtx.unlock();
				player->frame_id = target_frame_id;
			}
		}
    }
    player->window_running = false;
    player->background_running = false;
    
    // 清空帧内容
    player->frame_mtx.lock();
    player->frame_mat = cv::Mat();
    player->frame_mtx.unlock();
}

void TimeDrivenPlayer::MonitorWindowSize(TimeDrivenPlayer* player) {
    while ((cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1)); // 等待窗口创建

    while (player->window_running) {
        if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1) break;

        cv::Rect window_rect = cv::getWindowImageRect("Video Player");
        player->window_width = window_rect.width;
        player->window_height = window_rect.height;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    player->window_running = false;
}

