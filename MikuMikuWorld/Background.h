#pragma once
#include "Rendering/Texture.h"
#include "Rendering/Framebuffer.h"
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

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

		float width;
		float height;

		bool dirty;
		bool useJacketBg;

		bool mat_load_new;

		void resizeByRatio(float& w, float& h, const Vector2& tgt, bool vertical);

	  public:
		Background();

		void newEmpty(int w, int h);

		void load(const std::string& filename);
		void load(const cv::Mat& cv_mat);
		void resize(Vector2 target);
		void process(Renderer* renderer);
		void dispose();

		std::string getFilename() const;

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
