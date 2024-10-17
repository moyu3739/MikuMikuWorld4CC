#include "Background.h"
#include "Math.h"
#include "ResourceManager.h"
#include "Rendering/Renderer.h"
#include "Rendering/Framebuffer.h"
#include "Application.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld
{
	Background::Background()
	    : blur{ 0.0f }, brightness{ 0.4f }, width{ 0 }, height{ 0 }, dirty{ false },
		  mat_new_tex{ false }, resize_mode{ ResizeMode::MAX }
	{
		framebuffer = std::make_unique<Framebuffer>(1, 1);
	}

	void Background::load(const std::string& filename)
	{
		this->filename = filename;
		if (texture)
		{
			texture->dispose();
			texture = nullptr;
		}

		if (filename.empty() || !IO::File::exists(filename))
			return;

		texture = std::make_unique<Texture>(filename);
		framebuffer->resize(texture->getWidth(), texture->getHeight());

		dirty = true;
	}

	void Background::load(const cv::Mat& cv_mat) {
		if (mat_new_tex) {
			if (texture != nullptr) {
				texture->dispose();
				texture = nullptr;
				texture = std::make_unique<Texture>(cv_mat);
				framebuffer->resize(cv_mat.cols, cv_mat.rows);
			}
			mat_new_tex = false;
		}
		else texture->loadMat(cv_mat);

		// framebuffer->resize(texture->getWidth(), texture->getHeight());

		dirty = true;
	}

	void Background::load() {
		static const std::string defaultBackgroundPath = Application::getAppDir() + "res\\textures\\default.png";
		load(config.backgroundImage.empty() ? defaultBackgroundPath : config.backgroundImage);
	}

	void Background::resizeFrameBuffer(int w, int h) {
		if (framebuffer != nullptr) framebuffer->resize(w, h);
	}

	void Background::resizeByRatio(float& w, float& h, const Vector2& tgt, bool vertical)
	{
		if (vertical)
		{
			float ratio = tgt.y / h;
			h = tgt.y;
			w *= ratio;
		}
		else
		{
			float ratio = tgt.x / w;
			w = tgt.x;
			h *= ratio;
		}
	}

	void Background::resize(Vector2 target) {
		switch(resize_mode) {
			case ResizeMode::MIN: resizeMin(target); break;
			case ResizeMode::MAX: resizeMax(target); break;
		}
	}

	void Background::resizeMax(Vector2 target)
	{
		if (framebuffer == nullptr || texture == nullptr)
			return;

		float w = texture->getWidth();
		float h = texture->getHeight();
		float tgtAspect = target.x / target.y;

		if (tgtAspect > 1.0f)
			resizeByRatio(w, h, target, false);
		else if (tgtAspect < 1.0f)
			resizeByRatio(w, h, target, true);
		else
			w = h = target.x;

		// for non-square aspect ratios
		if (h < target.y)
			resizeByRatio(w, h, target, true);
		else if (w < target.x)
			resizeByRatio(w, h, target, false);

		width = w;
		height = h;
	}

	void Background::resizeMin(Vector2 target)
	{
		if (framebuffer == nullptr || texture == nullptr)
			return;

		float w = texture->getWidth();
		float h = texture->getHeight();
		float imgAspect = w / h;
    	float tgtAspect = target.x / target.y;

		if (imgAspect > tgtAspect) {
			resizeByRatio(w, h, target, false);
		}
		else if (imgAspect < tgtAspect) {
			resizeByRatio(w, h, target, true);
		}
		else {
			w = target.x;
			h = target.y;
		}

		width = w;
		height = h;
	}

	void Background::process(Renderer* renderer)
	{
		if (framebuffer == nullptr || texture == nullptr)
			return;

		int s = ResourceManager::getShader("basic2d");
		if (s == -1)
			return;

		int w = texture->getWidth();
		int h = texture->getHeight();

		if (w < 1 || h < 1)
			return;

		Shader* blur = ResourceManager::shaders[s];
		blur->use();
		blur->setMatrix4("projection", DirectX::XMMatrixOrthographicRH(w, -h, 0.001f, 100));

		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		// align background quad to canvas position
		Vector2 posR;
		Vector2 size(w, h);
		Color tint(brightness, brightness, brightness, 1.0f);

		renderer->drawSprite(posR, 0.0f, size, AnchorType::MiddleCenter, *texture, 0, tint);
		renderer->endBatch();
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		dirty = false;
	}

	void Background::getPosition(float canvas_pos_x, float canvas_pos_y,float canvas_size_x, float canvas_size_y,
								float& bg_pos_x, float& bg_pos_y) {
		switch(resize_mode) {
			case ResizeMode::MIN: {
				bg_pos_x = canvas_pos_x + (abs(width - canvas_size_x) / 2.0f);
				bg_pos_y = canvas_pos_y + (abs(height - canvas_size_y) / 2.0f);
				return;
			}
			case ResizeMode::MAX: {
				bg_pos_x = canvas_pos_x - (abs(width - canvas_size_x) / 2.0f);
				bg_pos_y = canvas_pos_y - (abs(height - canvas_size_y) / 2.0f);
				return;
			}
		}
	}

	std::string Background::getFilename() const { return filename; }

	int Background::getWidth() const { return width; }

	int Background::getHeight() const { return height; }

	int Background::getTextureID() const { return framebuffer->getTexture(); }

	float Background::getBlur() const { return blur; }

	void Background::setBlur(float b)
	{
		blur = b;
		dirty = true;
	}

	float Background::getBrightness() const { return brightness; }

	void Background::setBrightness(float b)
	{
		brightness = b;
		dirty = true;
	}

	bool Background::isDirty() const { return dirty; }

	void Background::dispose()
	{
		if (framebuffer)
		{
			framebuffer->dispose();
			framebuffer = nullptr;
		}

		if (texture)
		{
			texture->dispose();
			texture = nullptr;
		}
	}
}
