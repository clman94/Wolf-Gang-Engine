#pragma once

#include <optional>

#include <wge/util/json_helpers.hpp>
#include <wge/core/asset.hpp>
#include <wge/math/aabb.hpp>
#include <wge/graphics/image.hpp>
#include <wge/graphics/texture.hpp>

namespace wge::graphics
{

class sprite :
	public core::resource
{
public:
	using handle = core::resource_handle<sprite>;

	static constexpr int padding = 1;

public:

	struct frame_info
	{
		std::optional<math::vec2> anchor;
		std::optional<float> duration;

		static json serialize(const frame_info& pInfo)
		{
			json result;
			result["duration"] = pInfo.duration;
			result["anchor"] = pInfo.anchor;
			return result;
		}

		static frame_info deserialize(const json& pJson)
		{
			frame_info info;
			info.duration = pJson["duration"].get<std::optional<float>>();
			info.anchor = pJson["anchor"].get<std::optional<math::vec2>>();
			return info;
		}
	};

	virtual void load()
	{
		auto image_filepath = get_location()->get_autonamed_file(".png").string();
		if (!mImage.load_file(image_filepath))
			throw std::runtime_error(
				fmt::format("Could not load image from \"{}\". {}",
					image_filepath, mImage.get_error()));
		mTexture.set_image(mImage);

		// mAabb_collision was default initialized so we must give it a useful value.
		if (mAabb_collision.min == mAabb_collision.max)
			set_aabb_collision_to_image_size();
	}

	void set_texture_implementation(texture_impl::ptr pImpl)
	{
		mTexture.set_implementation(pImpl);
	}

	std::size_t get_frame_count() const noexcept
	{
		return mFrames.size();
	}

	float get_frame_duration(std::size_t pFrame) const noexcept
	{
		assert(pFrame < mFrames.size());
		if (!mFrames.empty() && mFrames[pFrame].duration.has_value())
			return mFrames[pFrame].duration.value();
		else
			return mFrame_duration;
	}

	math::vec2 get_frame_anchor(std::size_t pFrame) const noexcept
	{
		assert(pFrame < mFrames.size());
		if (!mFrames.empty() && mFrames[pFrame].anchor.has_value())
			return mFrames[pFrame].anchor.value();
		else
			return mAnchor;
	}

	math::aabb get_frame_aabb(std::size_t pFrame) const noexcept
	{
		math::aabb result;
		result.min = math::vec2{
			static_cast<float>(mSize.x + padding) * static_cast<float>(pFrame), 0 }
		+ math::vec2{ padding,  padding };
		result.max = result.min + math::vec2{ mSize };
		return result;
	}

	math::aabb get_frame_uv(std::size_t pFrame) const noexcept
	{
		math::aabb result = get_frame_aabb(pFrame);
		result.min /= math::vec2{ mImage.get_size() };
		result.max /= math::vec2{ mImage.get_size() };
		return result;
	}

	frame_info& get_frame_info(std::size_t pFrame)
	{
		assert(pFrame < mFrames.size());
		return mFrames[pFrame];
	}

	const frame_info& get_frame_info(std::size_t pFrame) const
	{
		assert(pFrame < mFrames.size());
		return mFrames[pFrame];
	}

	const texture& get_texture() const noexcept
	{
		return mTexture;
	}

	void resize_animation(std::size_t pLength)
	{
		mFrames.resize(pLength);
	}

	void set_frame_size(const math::ivec2& pSize) noexcept
	{
		mSize = pSize;
	}

	math::ivec2 get_frame_size() const noexcept
	{
		return mSize;
	}

	int get_frame_width() const noexcept
	{
		return mSize.x;
	}

	int get_frame_height() const noexcept
	{
		return mSize.y;
	}

	math::vec2 get_default_anchor() const noexcept
	{
		return mAnchor;
	}

	void set_default_anchor(const math::vec2& pAnchor) noexcept
	{
		mAnchor = pAnchor;
	}

	float get_default_duration() const noexcept
	{
		return mFrame_duration;
	}

	void set_default_duration(float pSeconds) noexcept
	{
		mFrame_duration = pSeconds;
	}

	void set_loop(bool pLoop) noexcept
	{
		mLoop = pLoop;
	}

	bool get_loop() const noexcept
	{
		return mLoop;
	}

	void set_aabb_collision_to_image_size() noexcept
	{
		mAabb_collision = { math::vec2{ 0, 0 }, math::vec2{ mSize } };
	}

	void set_aabb_collision(const math::aabb& pAabb) noexcept
	{
		mAabb_collision = pAabb;
	}

	const math::aabb& get_aabb_collision() const noexcept
	{
		return mAabb_collision;
	}

private:
	virtual json serialize_data() const override
	{
		json result;
		result["frame_duration"] = mFrame_duration;
		result["frame_size"] = mSize;
		result["anchor"] = mAnchor;
		result["loop"] = mLoop;
		result["aabb_collision_box"] = mAabb_collision;
		json jframes;
		for (auto& i : mFrames)
			jframes.push_back(frame_info::serialize(i));
		result["frames"] = std::move(jframes);
		return result;
	}

	virtual void deserialize_data(const json& pJson) override
	{
		mFrame_duration = pJson["frame_duration"];
		mSize = pJson["frame_size"];
		mAnchor = pJson["anchor"];
		mLoop = pJson["loop"];
		mAabb_collision = util::json_get_or<math::aabb>(pJson,
			"aabb_collision_box", math::aabb{ math::fvec2{ 0, 0 }, math::fvec2{ mSize } });
		for (auto& i : pJson["frames"])
			mFrames.push_back(frame_info::deserialize(i));
	}

private:
	texture mTexture;
	image mImage;
	std::vector<frame_info> mFrames;
	float mFrame_duration = 0;
	math::ivec2 mSize;
	math::vec2 mAnchor;
	math::aabb mAabb_collision;
	bool mLoop = false;
};

class sprite_controller
{
public:
	sprite_controller() noexcept = default;
	sprite_controller(sprite::handle pHandle) noexcept :
		mSprite(pHandle)
	{}

	void set_sprite(sprite::handle pHandle) noexcept
	{
		mIs_new_frame = true;
		mSprite = pHandle;
		restart();
	}

	sprite::handle get_sprite() const noexcept
	{
		return mSprite;
	}

	void toggle() noexcept
	{
		mPlaying = !mPlaying;
	}

	void play() noexcept
	{
		mPlaying = true;
	}

	void pause() noexcept
	{
		mPlaying = false;
	}

	void stop() noexcept
	{
		mPlaying = false;
		restart();
	}

	void restart() noexcept
	{
		mTimer = 0;
		mFrame_index = 0;
		mIs_new_frame = true;
	}

	bool is_playing() const noexcept
	{
		return mPlaying;
	}

	bool is_paused() const noexcept
	{
		return !mPlaying;
	}

	void set_playing(bool pPlaying) noexcept
	{
		mPlaying = pPlaying;
	}

	std::size_t get_frame() const noexcept
	{
		return mFrame_index;
	}

	void set_frame(std::size_t pFrame)
	{
		if (pFrame >= mSprite->get_frame_count())
			mFrame_index = mSprite->get_frame_count() - 1;
		else
			mFrame_index = pFrame;
		mIs_new_frame = true;
	}

	bool is_first_frame() const noexcept
	{
		if (!mSprite)
			return false;
		return mFrame_index == 0;
	}

	bool is_last_frame() const noexcept
	{
		if (!mSprite)
			return false;
		return mSprite->get_frame_count() > 0 &&
			mFrame_index == mSprite->get_frame_count() - 1;
	}

	bool is_new_frame() const noexcept
	{
		return mIs_new_frame;
	}

	bool is_beginning_animation() const noexcept
	{
		return mIs_beginning_animation;
	}

	bool is_ending_animation() const noexcept
	{
		return mIs_ending_animation;
	}

	void update(float pDelta) noexcept
	{
		if (!mSprite)
			return;
		mIs_new_frame = false;
		mIs_beginning_animation = false;
		mIs_ending_animation = false;
		if (mPlaying)
		{
			if (is_first_frame() && mTimer == 0)
				mIs_beginning_animation = true;
			if (mTimer >= mSprite->get_frame_duration(mFrame_index))
			{
				mIs_new_frame = true;
				advance_frame();
			}
			mTimer += pDelta * mSpeed;
		}
	}

	void set_speed(float pSpeed) noexcept
	{
		mSpeed = pSpeed;
	}

	float get_speed() const noexcept
	{
		return mSpeed;
	}

private:
	void advance_frame() noexcept
	{
		assert(mPlaying);
		assert(mSprite);
		mTimer = 0;
		++mFrame_index;
		if (mFrame_index >= mSprite->get_frame_count())
		{
			if (mSprite->get_loop())
				mFrame_index = 0;
			else
			{
				--mFrame_index;
				mPlaying = false;
				mIs_ending_animation = true;
			}
		}
	}

private:
	sprite::handle mSprite;
	float mSpeed = 1.f;
	float mTimer = 0;
	std::size_t mFrame_index = 0;
	bool mPlaying = false;
	bool mIs_new_frame = true;
	bool mIs_beginning_animation = false;
	bool mIs_ending_animation = false;
};

} // namespace wge::graphics
