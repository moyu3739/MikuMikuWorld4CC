#pragma once
#include "Rendering/Texture.h"
#include "Rendering/Framebuffer.h"
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>


enum ResizeMode {
	MIN,
	MAX
};

namespace MikuMikuWorld
{
	class Renderer;
	class Texture;
	struct Vector2;

	class Background
	{
	  private:
		std::string filename;
		std::unique_ptr<Texture> texture;
		std::unique_ptr<Framebuffer> framebuffer;

		float blur;
		float brightness;

		ResizeMode resize_mode;

		float width;
		float height;

		bool dirty;
		bool useJacketBg;

		bool mat_new_tex;

		void resizeByRatio(float& w, float& h, const Vector2& tgt, bool vertical);

	  public:
		Background();

		void matNewTex() {
			mat_new_tex = true;
		}

		ResizeMode getResizeMode() const {
			return resize_mode;
		}

		void setResizeMode(ResizeMode mode) {
			resize_mode = mode;
			dirty = true;
		}

		void load(const std::string& filename);
		void load(const cv::Mat& cv_mat);
		void load();
		void resize(Vector2 target);
		void resizeMin(Vector2 target);
		void resizeMax(Vector2 target);
		void process(Renderer* renderer);
		void dispose();

		void resizeFrameBuffer(int w, int h);

		std::string getFilename() const;

		void getPosition(float canvas_pos_x, float canvas_pos_y,float canvas_size_x, float canvas_size_y,
						   float& bg_pos_x, float& bg_pos_y);

		int getWidth() const;
		int getHeight() const;
		int getTextureID() const;

		float getBlur() const;
		void setBlur(float b);

		float getBrightness() const;
		void setBrightness(float b);

		bool isDirty() const;
	};
}
