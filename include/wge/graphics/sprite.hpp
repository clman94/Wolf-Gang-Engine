#pragma once

#include <optional>

#include <wge/util/json_helpers.hpp>
#include <wge/core/asset.hpp>
#include <wge/math/aabb.hpp>
#include <wge/graphics/image.hpp>

namespace wge::graphics
{

class sprite :
	public core::resource
{
public:
	using handle = core::resource_handle<sprite>;

public:
	struct frame_info
	{
		std::optional<math::vec2> anchor;
		std::optional<float> duration = 0;
	};

	static void to_json(json& pJson, const frame_info& pFrame)
	{
		pJson["duration"] = pFrame.duration;
		pJson["anchor"] = pFrame.anchor;
	}

	static void from_json(const json& pJson, frame_info& pFrame)
	{
		pFrame.duration = pJson["duration"];
		pFrame.anchor = pJson["anchor"];
	}

	virtual void load()
	{
		mImage.load_file(get_location().get_autonamed_file(".png").string());

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

	math::aabb get_frame_uv(std::size_t pFrame) const noexcept
	{
		math::aabb result;
		result.min = math::vec2{
			static_cast<float>(mSize.x + 1) * static_cast<float>(pFrame), 0 }
		+ math::vec2{ 1,  1 }; // +1 for Padding
		result.max = result.min + math::vec2{ mSize };
		result.min /= math::vec2{ mSize };
		result.max /= math::vec2{ mSize };
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

private:
	virtual json serialize_data() const override
	{
		json result;
		result["frame_duration"] = mFrame_duration;
		result["anchor"] = mAnchor;
		json jframes;
		for (auto& i : mFrames)
			jframes.push_back(i);
		result["frames"] = std::move(jframes);
		return result;
	}

	virtual void deserialize_data(const json& pJson) override
	{
		mFrame_duration = pJson["frame_duration"];
		mAnchor = pJson["anchor"];
		for (auto& i : pJson["frames"])
			mFrames.push_back(i.get<frame_info>());
	}

private:
	image mImage;
	std::vector<frame_info> mFrames;
	float mFrame_duration = 0;
	math::ivec2 mSize;
	math::vec2 mAnchor;
};

} // namespace wge::graphics
