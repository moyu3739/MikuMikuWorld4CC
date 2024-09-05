#include "TimeDrivenPlayer.h"


void TimeDrivenPlayer::OpenVideo(const std::string& path){
    cap = new cv::VideoCapture(path);
    if (!cap->isOpened()) throw std::runtime_error("open video failed");

    fps = cap->get(cv::CAP_PROP_FPS);
    interval = 1.0 / fps;
    running = true;
    playing = false;
    real_fps = 0;
    frame_id = -1;
    now = 0.0;
}

void TimeDrivenPlayer::CloseVideo(){
    std::cout << "CLOSE VIDEO" << std::endl;
    running = false;
    capture_thread->join();
    play_thread->join();
    window_size_thread->join();
    history_frame = std::queue<double>();
    if (cap != nullptr) {
        delete cap;
        cap = nullptr;
    }
    if (capture_thread != nullptr) {
        delete capture_thread;
        capture_thread = nullptr;
    }
    if (play_thread != nullptr) {
        delete play_thread;
        play_thread = nullptr;
    }
    if (window_size_thread != nullptr) {
        delete window_size_thread;
        window_size_thread = nullptr;
    }
}

cv::Mat TimeDrivenPlayer::ResizeFrame(const cv::Mat& frame, int window_width, int window_height) {
    // 获取帧的宽高比
    double aspect_ratio = (double)frame.cols / (double)frame.rows;
    int new_width, new_height;

    // 根据窗口大小计算新的帧大小，同时保持宽高比例
    if (window_width / aspect_ratio <= window_height) {
        new_width = window_width;
        new_height = static_cast<int>(window_width / aspect_ratio);
    } else {
        new_width = static_cast<int>(window_height * aspect_ratio);
        new_height = window_height;
    }

    if (new_width == 0 || new_height == 0) return cv::Mat();

    // 调整帧大小
    cv:: Mat resized_frame;
    resize(frame, resized_frame, cv::Size(new_width, new_height));

    // 创建一个黑色背景
    cv::Mat background(window_height, window_width, frame.type(), cv::Scalar(0, 0, 0));

    // 将调整大小后的帧放置在背景中央
    int x_offset = (window_width - new_width) / 2;
    int y_offset = (window_height - new_height) / 2;
    resized_frame.copyTo(background(cv::Rect(x_offset, y_offset, new_width, new_height)));

    return background;
}

void TimeDrivenPlayer::Play(TimeDrivenPlayer* player){
    cv::namedWindow("Video Player", cv::WINDOW_NORMAL); // 允许调整窗口大小

    while (player->running) {
        double now = player->now;
        int target_frame_id = static_cast<int>((now + player->interval / 2) / player->interval);
        int offset = target_frame_id - player->frame_id;

        if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1) break;
        // 调整帧大小以适应窗口，同时保持宽高比例
        if (!player->frame.empty()) {
            cv::Mat resized_frame = ResizeFrame(player->frame, player->window_width, player->window_height);
            if (!resized_frame.empty()) imshow("Video Player", resized_frame);
        }

        std::string title;

        int now_int = static_cast<int>(now);
        title += std::to_string(now_int / 60) + ":" + std::to_string(now_int % 60 / 10) + std::to_string(now_int % 10);

        if (player->playing)
            title += "  ( " +
                (player->real_fps > 1.0 ? std::to_string(static_cast<int>(player->real_fps + 0.5)) : "0")
                + " fps )";
        
        cv::setWindowTitle("Video Player", title);
		cv::waitKey(1);
    }
    player->running = false;

    // 检查窗口是否需要 destroy
    if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) > 0)
        cv::destroyWindow("Video Player");
}

void TimeDrivenPlayer::Capture(TimeDrivenPlayer* player, int chase_limit){
    // std::mutex mtx;

    while (player->running) {
        double now = player->now;
        int target_frame_id = static_cast<int>((now + player->interval / 2) / player->interval);
        int offset = target_frame_id - player->frame_id;

        // mtx.lock();

        // 处于暂停状态
        if (!player->playing){
            if (offset != 0) { // 暂停状态下，只要不是当前帧，就跳转
                player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
                if (!player->cap->read(player->frame)) {
                    std::cout << "视频播放完毕" << std::endl;
                    break;
                }
                player->frame_id = target_frame_id;
            }
        }
		// 处于播放状态
		else {
			if (offset == 0) {}
			else if (offset == 1) {
				if (!player->cap->read(player->frame)) {
					std::cout << "视频播放完毕" << std::endl;
					break;
				}
				player->frame_id = target_frame_id;
			}
			else if (1 < offset && offset < chase_limit) { // 如果进度落后但不超过 chase_limit 帧，则快进
				if (!player->cap->read(player->frame)) {
					std::cout << "视频播放完毕" << std::endl;
					break;
				}
				player->frame_id++;
			}
			else { // 如果进度落后超过 chase_limit 帧或超前，则跳转
				player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
				if (!player->cap->read(player->frame)) {
					std::cout << "视频播放完毕" << std::endl;
					break;
				}
				player->frame_id = target_frame_id;
			}
		}

        if (offset != 0){ // 渲染了新的视频帧，更新帧率信息
            if (player->history_frame.size() > 1){
                player->real_fps = (player->history_frame.size() - 1) / (player->history_frame.back() - player->history_frame.front());
            }
            player->history_frame.push(now);
            if (player->history_frame.size() > player->fps / 4) player->history_frame.pop();
        }

        // mtx.unlock();
    }
    player->running = false;
}

void TimeDrivenPlayer::MonitorWindowSize(TimeDrivenPlayer* player) {
    while ((cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1)); // 等待窗口创建

    while (player->running) {
        if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1) break;

        cv::Rect window_rect = cv::getWindowImageRect("Video Player");
        player->window_width = window_rect.width;
        player->window_height = window_rect.height;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    player->running = false;
}






// void TimeDrivenPlayer::_Play(TimeDrivenPlayer* player, int chase_limit){
//     cv::namedWindow("Video Player", cv::WINDOW_NORMAL); // 允许调整窗口大小

//     FreeTimer tm;
//     tm.Start();

//     while (player->running) {
//         double now = player->now;
//         int target_frame_id = static_cast<int>((now + player->interval / 2) / player->interval);
//         int offset = target_frame_id - player->frame_id;
// 		int wait_time;

//         double ckpt1 = tm.Read() * 1000;

//         // 处于暂停状态
//         if (!player->playing){
//             if (offset != 0) { // 暂停状态下，只要不是当前帧，就跳转
//                 player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
//                 if (!player->cap->read(player->frame)) {
//                     std::cout << "视频播放完毕" << std::endl;
//                     break;
//                 }
//                 player->frame_id = target_frame_id;
//             }
// 			wait_time = 1;
//         }
// 		// 处于播放状态
// 		else {
// 			if (offset == 0) {
// 				wait_time = 1;
// 			}
// 			else if (offset == 1) {
// 				if (!player->cap->read(player->frame)) {
// 					std::cout << "视频播放完毕" << std::endl;
// 					break;
// 				}
// 				player->frame_id = target_frame_id;
// 				wait_time = static_cast<int>((player->frame_id * player->interval + player->interval / 2 - now) * 1000);
// 			}
// 			else if (1 < offset && offset < chase_limit) { // 如果进度落后但不超过 chase_limit 帧，则快进
// 				if (!player->cap->read(player->frame)) {
// 					std::cout << "视频播放完毕" << std::endl;
// 					break;
// 				}
// 				player->frame_id++;
// 				wait_time = 1;
// 			}
// 			else { // 如果进度落后超过 chase_limit 帧或超前，则跳转
// 				player->cap->set(cv::CAP_PROP_POS_FRAMES, target_frame_id);
// 				if (!player->cap->read(player->frame)) {
// 					std::cout << "视频播放完毕" << std::endl;
// 					break;
// 				}
// 				player->frame_id = target_frame_id;
// 				wait_time = static_cast<int>((player->frame_id * player->interval + player->interval / 2 - now) * 1000);
// 			}
// 		}

//         double ckpt2 = tm.Read() * 1000;

//         std::string title = "Video Player";

//         int now_int = static_cast<int>(now);
//         title += " - " + std::to_string(now_int / 60) + ":" + std::to_string(now_int % 60);

//         if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) < 1) break;
//         // 调整帧大小以适应窗口，同时保持宽高比例
//         if (!player->frame.empty()) {
//             cv::Mat resized_frame = ResizeFrame(player->frame, player->window_width, player->window_height);
//             if (!resized_frame.empty()){
//                 imshow("Video Player", resized_frame);

//                 // 将帧率信息显示在窗口标题中
//                 player->history_frame.push(now);
//                 if (player->history_frame.size() > player->fps) player->history_frame.pop();
//                 double real_fps = player->fps / (now - player->history_frame.front());
//                 if (player->playing) title += " - " + std::to_string(static_cast<int>(real_fps + 0.5)) + " fps";
//             }
//         }

//         title += " - offset: " + std::to_string(offset);
//         title += " - frame: " + std::to_string(player->frame_id);

//         double ckpt3 = tm.Read() * 1000;
        
//         //cv::setWindowTitle("Video Player", title);

//         // cv::waitKey(max(1, wait_time));
// 		cv::waitKey(1);

// 		double ckpt4 = tm.Read() * 1000;

// 		title += " - cap: " + std::to_string(ckpt2 - ckpt1);
// 		title += " - show: " + std::to_string(ckpt3 - ckpt2);
// 		title += " - wait: " + std::to_string(ckpt4 - ckpt3);

// 		cv::setWindowTitle("Video Player", title);
//     }
//     player->running = false;

//     // 检查窗口是否需要 destroy
//     if (cv::getWindowProperty("Video Player", cv::WND_PROP_VISIBLE) > 0)
//         cv::destroyWindow("Video Player");
// }


