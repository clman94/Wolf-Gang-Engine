
#include <wge/editor/application.hpp>

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/core/scene.hpp>
#include <wge/physics/box_collider_component.hpp>
#include <wge/physics/physics_component.hpp>
#include <wge/physics/physics_world.hpp>
#include <wge/graphics/sprite_component.hpp>
#include <wge/core/asset_manager.hpp>
#include <wge/graphics/framebuffer.hpp>
#include <wge/filesystem/file_input_stream.hpp>
#include <wge/filesystem/exception.hpp>
#include <wge/graphics/renderer.hpp>
#include <wge/util/unique_names.hpp>
#include <wge/graphics/graphics.hpp>
#include <wge/math/transform.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/game_settings.hpp>
#include <wge/util/ipair.hpp>
#include <wge/core/object_resource.hpp>
#include <wge/core/scene_resource.hpp>
#include <wge/graphics/sprite.hpp>

#include "editor.hpp"
#include "script_editor.hpp"
#include "object_editor.hpp"
#include "sprite_editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"
#include "text_editor.hpp"
#include "icon_codepoints.hpp"
#include "asset_manager_window.hpp"
#include "widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/TextEditor.h>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <wge/graphics/glfw_backend.hpp>

#include <variant>
#include <functional>
#include <future>
#include <unordered_map>
#include <array>

namespace wge::physics
{

struct imgui_debug_draw : b2Draw
{
	virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
	{
		using namespace wge::editor;
		for (int32 i = 0; i < vertexCount; i++)
		{
			const b2Vec2& a = vertices[i];
			const b2Vec2& b = vertices[(i + 1) % vertexCount];
			visual_editor::draw_line({ a.x, a.y }, { b.x, b.y }, { color.r, color.g, color.b, color.a });
		}
	}

	virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
	{
		using namespace wge::editor;
		for (int32 i = 0; i < vertexCount; i++)
		{
			const b2Vec2& a = vertices[i];
			const b2Vec2& b = vertices[(i + 1) % vertexCount];
			visual_editor::draw_line({ a.x, a.y }, { b.x, b.y }, { color.r, color.g, color.b, color.a });
		}
	}

	virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
	{
		using namespace wge::editor;
		visual_editor::draw_circle({ center.x, center.y }, radius, { color.r, color.g, color.b, color.a }, true);
	}

	virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
	{
		using namespace wge::editor;
		visual_editor::draw_circle({ center.x, center.y }, radius, { color.r, color.g, color.b, color.a }, true);
	}
	virtual void DrawSegment(const b2Vec2& a, const b2Vec2& b, const b2Color& color) override
	{
		using namespace wge::editor;
		visual_editor::draw_line({ a.x, a.y }, { b.x, b.y }, { color.r, color.g, color.b, color.a });
	}
	virtual void DrawTransform(const b2Transform& xf) override
	{
		using namespace wge::editor;
		visual_editor::draw_circle({ xf.p.x, xf.p.y }, 5, { 1, 1, 1, 1 });
		visual_editor::draw_line({ xf.p.x, xf.p.y }, math::vec2{ 0, 0.1 }.rotate(xf.q.GetAngle()) + math::vec2{ xf.p.x, xf.p.y }, { 1, 1, 1, 1 });
	}

	virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override
	{
		using namespace wge::editor;
		visual_editor::draw_circle({ p.x, p.y }, size, { color.r, color.g, color.b, color.a });
	}
};

void physics_world::imgui_debug(float delta)
{
	using namespace wge::editor;
	if (mCollision_debug_enable)
	{
		imgui_debug_draw myimgui_debug_draw;
		myimgui_debug_draw.SetFlags(b2Draw::e_shapeBit);
		mWorld->SetDebugDraw(&myimgui_debug_draw);
		mWorld->DrawDebugData();
		mWorld->SetDebugDraw(nullptr);
	}
	if (mRaycast_debug_enabled)
	{
		const graphics::color nohit_color{ 1, 0, 0, 0.4f };
		const graphics::color hit_color{ 1, 1, 0, 0.4f };

		for (auto& i : mRaycast_debugs)
		{
			graphics::color color = (i.hit ? hit_color : nohit_color);
			color.a = i.timer;
			visual_editor::draw_line(i.from, i.to, color);
			if (i.hit)
				visual_editor::draw_circle(i.point, 3, hit_color, true);
			i.timer -= delta;
		}

		// Remove old casts
		auto iter = std::remove_if(mRaycast_debugs.begin(), mRaycast_debugs.end(), [](auto& i) { return i.timer <= 0; });
		mRaycast_debugs.erase(iter, mRaycast_debugs.end());
	}
}

} // namespace wge::physics

namespace wge::editor
{

struct spritesheet_data
{
	graphics::image image;
	math::ivec2 frame_size;
	std::size_t frame_count = 0;
};

// Takes a directory with an array of sprites and converts them into
// a spritesheet with proper padding.
// If path points to a single file, it fill create a single frame animation.
//
// The directory should look like this:
//   mysprite/
//   mysprite/mysprite1.png
//   mysprite/mysprite2.png
//   mysprite/mysprite3.png
//   ...etc...
// Assumptions:
// - All images are the exact same size.
// - All images are png.
// - There is at least one frame.
// Will throw if an image is unable to be read.
spritesheet_data create_spritesheet(const std::filesystem::path& pPath)
{
	const std::string sprite_name = pPath.filename().string();
	std::vector<graphics::image> frames;

	// If the path points to a directory, this assumes
	// that there are multiple sprite inside.
	if (std::filesystem::is_directory(pPath))
	{
		// Load a multiframe sprite.

		const auto make_frame_filepath = [&](int index)
		{
			return pPath / (sprite_name + std::to_string(index) + ".png");
		};

		// Load all the frames.
		int frame_index = 1;
		std::filesystem::path filepath = make_frame_filepath(frame_index);
		while (std::filesystem::exists(filepath))
		{
			frames.push_back(graphics::image{ filepath.string() });
			++frame_index;
			filepath = make_frame_filepath(frame_index);
		}
	}
	else
	{
		// Load a single frame sprite.
		frames.push_back(graphics::image{ pPath.string() });
	}

	// Sanity check.
	// Client must check if the directory contains at least one frame.
	assert(!frames.empty());

	// Calculate the size of the spritesheet.
	// *This assumes all the frames are the same size.
	const int padding = graphics::sprite::padding;
	const math::ivec2 spritesheet_size{
		(frames.front().get_width() + padding) * static_cast<int>(frames.size()) + padding,
		frames.front().get_height() + padding * 2
	};

	// Copy all the frames into the spritesheet.
	graphics::image spritesheet(spritesheet_size);
	math::ivec2 frame_position{ padding, padding };
	for (auto& i : frames)
	{
		spritesheet.splice(i, frame_position);
		frame_position += math::ivec2{ frames.front().get_width() + padding, 0 };
	}

	return { std::move(spritesheet), frames.front().get_size(), frames.size() };
}

// This exists because the C++17 file_time_clock does not
// have any facilities to allow us to serialize it (to_time_t, from_time_t).
template <typename Tto_tp, typename Tfrom_tp>
inline Tto_tp cast_clock(Tfrom_tp pTime_point)
{
	return std::chrono::time_point_cast<Tto_tp::duration>((pTime_point - Tfrom_tp::clock::now()) + Tto_tp::clock::now());
}

class import_manager
{
private:
	using time_point = std::chrono::system_clock::time_point;

	struct import_link
	{
		util::uuid asset;
		std::string import_name;
		time_point import_time;

		static json serialize(const import_link& pLink)
		{
			std::time_t time = time_point::clock::to_time_t(pLink.import_time);
			std::tm* tm = localtime(&time);
			std::stringstream time_buffer;
			time_buffer << std::put_time(tm, "%b %d %Y %H:%M:%S");
			return {
				{ "asset_id", pLink.asset },
				{ "import_name", pLink.import_name },
				{ "import_time", time_buffer.str() }
			};
		}
		static import_link deserialize(const json& pJson)
		{
			import_link link;
			link.asset = pJson["asset_id"];
			link.import_name = pJson["import_name"];
			
			std::tm tm{};
			std::stringstream ss(pJson["import_time"].get<std::string>());
			ss >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
			link.import_time = time_point::clock::from_time_t(mktime(&tm));
			
			return link;
		}
	};

public:
	void load_import_folder(const std::filesystem::path& pLocation)
	{
		mDirectory = pLocation;
		const std::filesystem::path imports_path = mDirectory / "imports.json";
		if (std::filesystem::exists(imports_path))
		{
			log::info("Parsing imports.json in \"{}\"", imports_path.string());
			deserialize(json::parse(std::ifstream(imports_path.c_str())));
		}
		parse_import_directory();
	}

	void save_import_config() const
	{
		std::ofstream((mDirectory / "imports.json").c_str(), std::ios::binary) << serialize().dump(2);
	}

	void register_link(const core::asset::ptr& pAsset, std::string_view pName)
	{
		// Find an existing link and update the import date.
		for (auto& i : mLinks)
		{
			if (i.asset == pAsset->get_id())
			{
				i.import_time = time_point::clock::now();
				save_import_config();
				return;
			}
		}

		// Create a new link if none where found.
		import_link link;
		link.import_name = pName;
		link.import_time = time_point::clock::now();
		link.asset = pAsset->get_id();
		mLinks.push_back(link);
		save_import_config();
	}

	void import_tileset(const std::filesystem::path& pFilepath, core::asset_manager& pAsset_mgr)
	{
		graphics::image sprite((mDirectory / pFilepath).string());

		// Create the new asset.
		auto tileset_asset = std::make_shared<core::asset>();
		tileset_asset->set_name(pFilepath.stem().string());
		tileset_asset->set_type("tileset");
		pAsset_mgr.store_asset(tileset_asset);

		// Save the image to the asset's location.
		bool success = sprite.save_png(tileset_asset->get_location()->get_autonamed_file(".png").string());
		assert(success);

		// Configure the resource.
		auto tileset_resource = pAsset_mgr.create_resource("tileset");
		tileset_resource->set_location(tileset_asset->get_location());
		tileset_resource->load();

		// Save the configuration.
		tileset_asset->set_resource(std::move(tileset_resource));
		tileset_asset->save();
		pAsset_mgr.add_asset(tileset_asset);
		register_link(tileset_asset, pFilepath.stem().string());
	}

	void import_sprite(const std::string& pPath, core::asset_manager& pAsset_mgr)
	{
		const std::string name = std::filesystem::path(pPath).stem().string();

		// Create the new asset.
		auto sprite_asset = std::make_shared<core::asset>();
		sprite_asset->set_name(name);
		sprite_asset->set_type("sprite");
		pAsset_mgr.store_asset(sprite_asset);

		// Save the new spritesheet to the asset's location.
		const spritesheet_data spritesheet = create_spritesheet(mDirectory / pPath);
		bool success = spritesheet.image.save_png(sprite_asset->get_location()->get_autonamed_file(".png").string());
		assert(success);

		// Configure the resource.
		auto sprite_resource = util::dynamic_unique_cast<graphics::sprite>(pAsset_mgr.create_resource("sprite"));
		sprite_resource->set_location(sprite_asset->get_location());
		sprite_resource->resize_animation(spritesheet.frame_count);
		sprite_resource->set_frame_size(spritesheet.frame_size);
		sprite_resource->load();

		// Save the configuration.
		sprite_asset->set_resource(std::move(sprite_resource));
		sprite_asset->save();
		pAsset_mgr.add_asset(sprite_asset);
		register_link(sprite_asset, pPath);
	}

	void reimport_sprite(const std::string& pPath, const core::asset_manager& pAsset_mgr)
	{
		import_link* link = get_link(pPath);
		assert(link != nullptr);

		core::asset::ptr asset = pAsset_mgr.get_asset(link->asset);
		assert(asset != nullptr);

		// Save the new spritesheet to the asset's location.
		const spritesheet_data spritesheet = create_spritesheet(mDirectory / pPath);
		bool success = spritesheet.image.save_png(asset->get_location()->get_autonamed_file(".png").string());
		assert(success);

		// Update the sprite info.
		auto sprite_resource = asset->get_resource<graphics::sprite>();
		sprite_resource->resize_animation(spritesheet.frame_count);
		sprite_resource->set_frame_size(spritesheet.frame_size);
		sprite_resource->load();

		register_link(asset, pPath);
	}

	static bool is_valid_animated_sprite(const std::filesystem::path& pDirectory)
	{
		// Check if it has at least 1 frame with the correct name.
		return std::filesystem::exists(pDirectory / (pDirectory.filename().string() + "1.png"));
	}

	void parse_import_directory()
	{
		mNot_imported.clear();
		mOutdated_names.clear();
		if (!std::filesystem::exists(mDirectory))
		{
			log::error("No import folder at \"{}\". Skiping import parsing.", mDirectory.string());
			return;
		}

		for (auto i : std::filesystem::directory_iterator(mDirectory))
		{
			if (i.is_directory() && is_valid_animated_sprite(i.path()) ||
				i.path().extension() == ".png")
			{
				const std::string name = i.path().filename().string();
				if (!has_imported(name))
					mNot_imported.push_back(name);

				else if (is_import_outdated(name, i))
					mOutdated_names.push_back(name);
			}
		}
	}

	util::span<const std::string> get_not_imported_names() const noexcept
	{
		return mNot_imported;
	}

	util::span<const std::string> get_outdated_names() const noexcept
	{
		return mOutdated_names;
	}

	bool is_import_outdated(std::string_view pName, const std::filesystem::directory_entry& pEntry) const noexcept
	{
		for (auto& i : mLinks)
		{
			if (i.import_name == pName)
			{
				if (pEntry.is_directory())
				{
					// Check all sub files in the directory.
					for (auto subfile : std::filesystem::directory_iterator(pEntry))
					{
						auto write_time = cast_clock<time_point>(subfile.last_write_time());
						if (i.import_time < write_time)
							return true;
					}
					return false;
				}
				else
				{
					auto write_time = cast_clock<time_point>(pEntry.last_write_time());
					// Check only the file.
					return i.import_time < write_time;
				}
			}
		}
		return false;
	}

	import_link* get_link(std::string_view pName) noexcept
	{
		for (auto& i : mLinks)
			if (i.import_name == pName)
				return &i;
		return nullptr;
	}

	bool has_imported(std::string_view pName) const noexcept
	{
		for (auto& i : mLinks)
			if (i.import_name == pName)
				return true;
		return false;
	}

private:
	std::filesystem::path mDirectory;
	std::vector<std::string> mNot_imported;
	std::vector<std::string> mOutdated_names;
	std::vector<import_link> mLinks;

	json serialize() const
	{
		json result;
		
		json jimports;
		for (auto& i : mLinks)
			jimports.push_back(import_link::serialize(i));
		result["imports"] = std::move(jimports);
		return result;
	}

	void deserialize(const json& pJson)
	{
		for (auto& i : pJson["imports"])
			mLinks.push_back(import_link::deserialize(i));
	}
};

class import_window
{
public:
	void on_gui(core::asset_manager& pAsset_mgr, import_manager& mManager)
	{
		bool imports_need_refresh = false;

		ImGui::Begin("Imports");

		if (ImGui::Button("Update List"))
		{
			log::info("Parsing import directory...");
			mManager.parse_import_directory();
			log::info("Found {} new imports", mManager.get_not_imported_names().size());
			log::info("Found {} outdated imports", mManager.get_outdated_names().size());
		}
		ImGui::BeginChild("ImportsPane", { 0, 0 }, true);
		ImGui::Columns(2);
		for (auto& i : mManager.get_outdated_names())
		{
			ImGui::PushID(&i);
			ImGui::TextUnformatted(i.c_str());
			ImGui::NextColumn();
			if (ImGui::Button("Update Sprite"))
			{
				mManager.reimport_sprite(i, pAsset_mgr);
			}
			ImGui::NextColumn();
			ImGui::PopID();
		}

		for (auto& i : mManager.get_not_imported_names())
		{
			ImGui::PushID(&i);
			ImGui::TextUnformatted(i.c_str());
			ImGui::NextColumn();
			if (ImGui::Button("Import Sprite"))
			{
				mManager.import_sprite(i, pAsset_mgr);
				imports_need_refresh = true;
			}
			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns();
		ImGui::EndChild();
		ImGui::End();

		if (imports_need_refresh)
		{
			mManager.parse_import_directory();
		}
	}
};


// Creates an imgui dockspace in the main window
inline void main_viewport_dock(ImGuiID pDock_id)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |=  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("_MainDockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(3);
	
	ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;//ImGuiDockNodeFlags_PassthruDockspace;
	ImGui::DockSpace(pDock_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	
	ImGui::End();
}

inline bool collapsing_arrow(const char* pStr_id, bool* pOpen = nullptr, bool pDefault_open = false)
{
	ImGui::PushID(pStr_id);

	// Use internal instead
	if (!pOpen)
		pOpen = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("IsOpen"), pDefault_open);;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
	if (ImGui::ArrowButton("Arrow", *pOpen ? ImGuiDir_Down : ImGuiDir_Right))
		*pOpen = !*pOpen; // Toggle open flag
	ImGui::PopStyleColor(3);

	ImGui::PopID();
	return *pOpen;
}





class tileset_editor :
	public asset_editor
{
public:
	tileset_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset)
	{}

	virtual void on_gui() override
	{
		auto tileset = get_asset()->get_resource<graphics::tileset>();

		int tile_size = tileset->tile_size.x;
		if (ImGui::InputInt("Tile Size", &tile_size))
		{
			tile_size = math::max(tile_size, 1);
			tileset->tile_size = { tile_size, tile_size };
			mark_asset_modified();
		}

		ImGui::BeginChild("TilesetEditor", { 0, 0 }, true);
		begin_image_editor("Tileset", tileset->get_texture());
		visual_editor::draw_grid({ 1, 1, 1, 1 }, static_cast<float>(tileset->tile_size.x));
		end_image_editor();
		ImGui::EndChild();
	}
};

class layer_previews
{
public:
	void set_graphics(graphics::graphics& pGraphics)
	{
		mGraphics = &pGraphics;
	}
	
	// Set the interval in which to rerender previews.
	// Default: 60 frames
	void set_render_interval(std::size_t pFrames)
	{
		assert(pFrames > 0);
		mRender_interval = pFrames;
	}

	void render_previews(core::scene& pScene, graphics::renderer& pRenderer, const math::ivec2& pSize)
	{
		assert(mGraphics);

		mFramebuffers.resize(pScene.get_layer_container().size());
		++mFrame_clock;
		if (mFrame_clock >= mRender_interval)
		{
			graphics::framebuffer::ptr last_fb = pRenderer.get_framebuffer();
			mFrame_clock = 0;
			std::size_t framebuffer_idx = 0;
			for (auto& i : pScene)
			{
				auto& framebuffer = mFramebuffers[framebuffer_idx];
				if (framebuffer == nullptr)
					framebuffer = mGraphics->get_graphics_backend()->create_framebuffer();
				if (framebuffer->get_size() != pSize)
					framebuffer->resize(pSize.x, pSize.y);

				framebuffer->clear();

				pRenderer.set_framebuffer(framebuffer);
				// If we are using the same renderer from the viewport,
				// this should use the same render view.
				pRenderer.render_layer(i, *mGraphics);
				++framebuffer_idx;
			}
			pRenderer.set_framebuffer(last_fb);
		}
	}

	graphics::framebuffer::ptr get_preview_framebuffer(std::size_t pLayer_index) const
	{
		if (pLayer_index >= mFramebuffers.size())
			return nullptr;
		return mFramebuffers[pLayer_index];
	}

private:
	std::size_t mRender_interval = 60;
	std::size_t mFrame_clock = 0;
	graphics::graphics* mGraphics = nullptr;
	std::vector<graphics::framebuffer::ptr> mFramebuffers;
};

struct editor_object_info
{
	// Precalculated local aabb of an object.
	math::aabb local_aabb;
};

class scene_editor_mode
{
public:
	virtual ~scene_editor_mode() {}
	virtual void on_overlay() {}
	virtual void on_detailed_properties() {}
	virtual void on_side_bar() {}
	virtual void on_tool_bar() {}
	virtual void on_bottom_bar() {}
	// Return true to select the current layer.
	virtual bool on_layer_display() { return false; }
	virtual math::aabb get_aabb() = 0;
};

class scene_editor_instance_mode :
	public scene_editor_mode
{
public:
	scene_editor_instance_mode(asset_editor& pMain_editor, core::layer& pLayer) :
		mMain_editor(&pMain_editor), mSelected_layer(&pLayer)
	{}

	virtual bool on_layer_display() override
	{
		bool select_this_layer = false;
		const auto _faded_color = ImGui::ScopedStyleColor(ImGuiCol_Text, { 0.6f, 0.6f, 0.6f, 1 });
		if (ImGui::TreeNode("Objects"))
		{
			for (auto obj : *mSelected_layer)
			{
				if (!obj.get_name().empty())
				{
					if (ImGui::Selectable(obj.get_name().c_str(), obj == mSelected_object))
					{
						mSelected_object = obj;
						select_this_layer = true;
					}
				}
			}
			ImGui::TreePop();
		}
		return select_this_layer;
	}

	virtual void on_side_bar() override
	{
		if (ImGui::CollapsingHeader("Instances", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float height = 200;
			ImGui::BeginChild("Instances", ImVec2(0, height), true);
			for (auto obj : *mSelected_layer)
			{
				ImGui::PushID(static_cast<int>(obj.get_id()));
				std::string display_name = fmt::format("{} [{} id:{}]", obj.get_name(), obj.get_asset()->get_name(), obj.get_id());
				if (ImGui::Selectable(display_name.c_str(), obj == mSelected_object))
					mSelected_object = obj;
				ImGui::PopID();
			}
			do_object_context_menu();
			ImGui::EndChild(); // Instances
			ImGui::HorizontalSplitter("InstancesSplitter", &height);
		}
	}

	virtual void on_detailed_properties() override
	{
		if (mSelected_layer && mSelected_object.is_valid() && ImGui::TreeNode("Selected Object"))
		{
			std::string name = mSelected_object.get_name();
			if (ImGui::InputText("Name", &name))
				mSelected_object.set_name(name);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				mSelected_object.set_name(scripting::make_valid_identifier(name));
				mMain_editor->mark_asset_modified();
			}
			if (ImGui::Button("Goto Asset"))
			{
				mMain_editor->get_context().open_editor(mSelected_object.get_asset());
			}
			ImGui::TextUnformatted("Transform");
			math::transform* transform = mSelected_object.get_component<math::transform>();
			ImGui::BeginGroup();
			ImGui::DragFloat2("Position", transform->position.components().data());
			math::degrees degrees = transform->rotation;
			if (ImGui::DragFloat("Rotation", degrees.components().data()))
				transform->rotation = degrees;
			ImGui::DragFloat2("Scale", transform->scale.components().data());
			ImGui::EndGroup();
			if (ImGui::IsItemDeactivatedAfterEdit())
				mMain_editor->mark_asset_modified();

			ImGui::Button("Creation Code");

			ImGui::TreePop();
		}
	}

	virtual void on_overlay() override
	{
		update_aabbs();

		// True when the selected object is being edited.
		// We don't want to select objects behind it on accident.
		bool is_currently_editing = false;
		if (mSelected_object.is_valid())
		{
			// Edit object transform.
			editor_object_info* info = mSelected_object.get_component<editor_object_info>();
			math::transform* transform = mSelected_object.get_component<math::transform>();
			visual_editor::box_edit box_edit(info->local_aabb, *transform);
			box_edit.resize(visual_editor::edit_type::transform);
			box_edit.drag(visual_editor::edit_type::transform);
			*transform = box_edit.get_transform();
			if (box_edit.is_dragging())
			{
				is_currently_editing = true;
				mMain_editor->mark_asset_modified();
			}

			// Draw center point.
			visual_editor::draw_circle(transform->position, 5, { 1, 1, 1, 0.6f }, 3.f);
		}

		// Draw boundaries of all the other objects.
		for (auto& [id, transform, info] : mSelected_layer->each<math::transform, editor_object_info>())
		{
			if (mSelected_object && mSelected_object.get_id() == id)
				continue;
			visual_editor::push_transform(transform);
			visual_editor::draw_rect(info.local_aabb, { 1, 1, 1, 0.7f });
			visual_editor::pop_transform();
		}
		
		if (!do_object_context_menu() &&
			!is_currently_editing && ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
		{
			// Generate a list of potential objects to be selected.
			std::vector<core::object> canidates;
			for (auto& [id, transform, info] : mSelected_layer->each<math::transform, editor_object_info>())
			{
				visual_editor::push_transform(transform);
				if (info.local_aabb.intersect(visual_editor::get_mouse_position()))
					canidates.push_back(mSelected_layer->get_object(id));
				visual_editor::pop_transform();
			}

			// If one of the canidates are already currectly selected,
			// we will use the object after the already selected one or loop back to the beginning.
			auto selected = std::find_if(canidates.begin(), canidates.end(), [this](core::object& obj) { return obj == mSelected_object; });
			if (selected != canidates.end())
			{
				++selected;
				// Loop back to the beginning.
				if (selected == canidates.end())
					selected = canidates.begin();
				mSelected_object = *selected;
			}
			// Select first object if nothing else is selected.
			else if (!canidates.empty())
				mSelected_object = canidates.front();
			// Deselect objects if the user clicked in an empty space.
			else
				mSelected_object = core::invalid_object;
		}
	}

	virtual math::aabb get_aabb() override
	{
		update_aabbs();
		math::aabb aabb;
		for (auto&& [id, editor_object_info, transform] : mSelected_layer->each<editor_object_info, math::transform>())
		{
			const math::aabb& local_aabb = editor_object_info.local_aabb;
			aabb.merge(transform.position);
			aabb.merge(transform.apply_to(local_aabb.min));
			aabb.merge(transform.apply_to(local_aabb.min + math::vec2{ local_aabb.max.x, 0 }));
			aabb.merge(transform.apply_to(local_aabb.min + local_aabb.max));
			aabb.merge(transform.apply_to(local_aabb.min + math::vec2{ 0, local_aabb.max.y }));
		}
		return aabb;
	}

private:
	bool do_object_context_menu()
	{
		if (ImGui::BeginPopupContextWindow("ObjectContextMenu"))
		{
			if (ImGui::MenuItem("Open Editor", 0, false, mSelected_object.is_valid() && mSelected_object.get_asset() != nullptr))
			{
				mMain_editor->get_context().open_editor(mSelected_object.get_asset());
			}
			if (ImGui::MenuItem("Delete", 0, false, mSelected_object.is_valid()))
			{
				mSelected_object.destroy();
				mSelected_object = core::invalid_object;
			}
			ImGui::EndPopup();
			return true;
		}
		return false;
	}

	void ensure_editor_info()
	{
		for (auto& obj : *mSelected_layer)
		{
			if (!obj.has_component<editor_object_info>())
				obj.add_component(editor_object_info{});
		}
	}

	static math::aabb get_aabb_from_object(core::object& pObj)
	{
		math::aabb aabb;
		if (auto sprite = pObj.get_component<graphics::sprite_component>())
		{
			aabb = sprite->get_local_aabb();
		}
		return aabb;
	}

	void update_aabbs()
	{
		ensure_editor_info();
		for (auto& [id, editor_object_info] : mSelected_layer->each<editor_object_info>())
		{
			editor_object_info.local_aabb = get_aabb_from_object(mSelected_layer->get_object(id));
		}
	}

	math::aabb mAabb;
	core::layer* mSelected_layer = nullptr;
	core::object mSelected_object;
	asset_editor* mMain_editor = nullptr;
};

class scene_editor_tilemap_mode :
	public scene_editor_mode
{
public:
	scene_editor_tilemap_mode(asset_editor& pMain_editor, core::layer& pLayer) :
		mMain_editor(&pMain_editor), mSelected_layer(&pLayer)
	{
		update_aabb();
	}

	virtual bool on_layer_display() override
	{
		ImGui::TextColored({ 0.6f, 0.6f, 0.6f, 1 }, "Tilemap");
		return false;
	}

	virtual void on_tool_bar() override
	{
		if (ImGui::ToolButton((const char*)ICON_FA_PENCIL, mTilemap_mode == tilemap_mode::draw))
			mTilemap_mode = tilemap_mode::draw;
		ImGui::DescriptiveToolTip("Paint Tool", "Drag To draw!");
		ImGui::SameLine();
		if (ImGui::ToolButton((const char*)ICON_FA_ERASER, mTilemap_mode == tilemap_mode::erase))
			mTilemap_mode = tilemap_mode::erase;
		ImGui::DescriptiveToolTip("Erase Tool", "Drag To Erase!");
		ImGui::SameLine();
		ImGui::Button((const char*)ICON_FA_OBJECT_GROUP);
		ImGui::DescriptiveToolTip("Select Tool", "Select a range!");
	}

	virtual void on_side_bar() override
	{
		core::tilemap_manipulator tilemap(*mSelected_layer);
		if (auto asset = asset_selector("Select_tileset", "tileset", mMain_editor->get_asset_manager(), tilemap.get_tileset().get_asset()))
		{
			tilemap.set_tileset(asset);
			tilemap.update_tile_uvs();
		}

		auto tileset = tilemap.get_tileset();
		if (tileset)
		{
			begin_image_editor("Tileset", tileset->get_texture());
			// Draw tile grid.
			visual_editor::draw_grid({ 1, 1, 1, 1 }, static_cast<float>(tilemap.get_tilesize().x));

			if (ImGui::IsItemClicked())
			{
				mTilemap_brush = math::ivec2(visual_editor::get_mouse_position() / math::vec2(tilemap.get_tilesize()));
			}

			// Draw brush selection.
			visual_editor::draw_rect(math::rect(math::vec2(mTilemap_brush * tilemap.get_tilesize()),
				math::vec2(tilemap.get_tilesize())), { 1, 1, 0, 1 });
			end_image_editor();
		}
	}

	virtual void on_overlay() override
	{
		core::tilemap_manipulator tilemap(*mSelected_layer);
		const math::ivec2 tile_position{ visual_editor::get_mouse_position().floor() };

		// Select color of hover overlay from the edit mode.
		graphics::color hover_overlay_color;
		switch (mTilemap_mode)
		{
		default:
		case tilemap_mode::draw: hover_overlay_color = { 1, 1, 0, 1 }; break;
		case tilemap_mode::erase:
			if (tilemap.find_tile(tile_position))
				hover_overlay_color = { 1, 0.4f, 0.4f, 1 };
			else
				hover_overlay_color = { 1, 0.7f, 0.7f, 1 };
			break;
		}
		// Draw the overlay
		if (ImGui::IsItemHovered())
			visual_editor::draw_rect(math::rect(math::vec2(tile_position), math::vec2(1, 1)), hover_overlay_color);

		if (ImGui::IsItemActive())
		{
			switch (mTilemap_mode)
			{
			default:
			case tilemap_mode::draw: tilemap.set_tile(tile_position, mTilemap_brush); break;
			case tilemap_mode::erase: tilemap.clear_tile(tile_position); break;
			}
			update_aabb();
			mMain_editor->mark_asset_modified();
		}
	}

	virtual math::aabb get_aabb() override
	{
		return mAabb;
	}

private:
	void update_aabb()
	{
		mAabb.min = mAabb.max = { 0, 0 };
		for (auto& [id, tile] : mSelected_layer->each<core::tile>())
		{
			mAabb.merge(math::aabb{ math::vec2{ tile.position },
				math::vec2{ tile.position } + math::vec2{ 1, 1 } });
		}
	}

private:
	enum class tilemap_mode
	{
		draw,
		erase,
	};
	tilemap_mode mTilemap_mode = tilemap_mode::draw;
	math::ivec2 mTilemap_brush;
	math::aabb mAabb;

	core::layer* mSelected_layer = nullptr;
	asset_editor* mMain_editor = nullptr;
};

class scene_editor :
	public asset_editor
{
public:

	// TODO: Implement a better and more generic messaging method at some point.
	using on_game_run_callback = std::function<void(const core::asset::ptr&)>;

	scene_editor(context& pContext, const core::asset::ptr& pAsset, const on_game_run_callback& pRun_callback) noexcept :
		asset_editor(pContext, pAsset),
		mOn_game_run_callback(pRun_callback)
	{
		log::info("Opening Scene Editor...");
		mLayer_previews.set_graphics(pContext.get_engine().get_graphics());

		log::info("Creating Scene Editor Framebuffer...");
		// Create a framebuffer for the scene to be rendered to.
		auto& graphics = pContext.get_engine().get_graphics();
		mViewport_framebuffer = graphics.get_graphics_backend()->create_framebuffer();
		mRenderer.set_framebuffer(mViewport_framebuffer);
		mRenderer.set_pixel_size(0.01f);

		mScene_resource = get_asset()->get_resource<core::scene_resource>();

		// Generate the layer.
		log::info("Generating Scene...");
		mScene_resource->generate_scene(mScene, pContext.get_engine().get_asset_manager());
		log::info("Layers: {}", mScene.get_layer_container().size());

		mViewport_camera.set_size({ 30, 30 });
	}

	virtual void on_gui() override
	{
		static float side_panel_width = 200;
		ImGui::BeginChild("SidePanelSettings", ImVec2(side_panel_width, 0));
		if (ImGui::CollapsingHeader("Scene"))
		{
			if (ImGui::Button("Send to viewport") && mOn_game_run_callback)
			{
				log::info("Sending scene to Player Viewport...");
				// Make sure the actual asset data is up to date before
				// we start the scene.
				update_asset_data();
				// Invoke the callback. I may consider using a better
				// messaging mechanism later.
				mOn_game_run_callback(get_asset());
			}

			if (ImGui::Button("Open Scene Script"))
			{

			}
		}
		if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float layers_height = 300;
			ImGui::BeginChild("Layers", ImVec2(0, layers_height), true);

			ImGui::BeginGroup();
			for (auto i = mScene.get_layer_container().begin();
				i != mScene.get_layer_container().end();
				++i)
			{
				ImGui::PushID(&*i);

				if (ImGui::Selectable("##LayerSelectable", mSelected_layer == &*i))
				{
					select_layer(*i);
				}
				ImGui::SameLine();
				auto preview = mLayer_previews.get_preview_framebuffer(std::distance(mScene.get_layer_container().begin(), i));
				if (preview)
				{
					ImGui::Image(preview, { 30, 30 });
					ImGui::SameLine();
				}

				ImGui::BeginGroup();
				ImGui::TextUnformatted(i->get_name().c_str());
				if (get_layer_editor(*i)->on_layer_display())
				{
					select_layer(*i);
				}
				ImGui::EndGroup();

				ImGui::PopID();
			}
			ImGui::EndGroup();
			if (mSelected_layer && ImGui::BeginPopupContextWindow())
			{
				std::string layer_name = mSelected_layer->get_name();
				if (ImGui::InputText("Name", &layer_name))
				{
					mSelected_layer->set_name(layer_name);
					mark_asset_modified();
				}
				if (ImGui::MenuItem("Delete"))
				{
					remove_selected_layer();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::EndChild();
			ImGui::HorizontalSplitter("LayersSplitter", &layers_height);
			if (ImGui::Button((const char*)ICON_FA_PLUS))
				ImGui::OpenPopup("AddLayerPopup");
			if (ImGui::BeginPopup("AddLayerPopup"))
			{
				if (ImGui::Selectable("Sprites"))
				{
					log::info("Adding Sprite Layer...");
					// Add layer specific components here.
					mScene.add_layer();
					mark_asset_modified();
				}
				if (ImGui::Selectable("Tilemap"))
				{
					log::info("Adding Tilemap Layer...");
					core::layer& layer = mScene.add_layer();
					core::tilemap_manipulator tilemap(layer);
					mark_asset_modified();
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			
			if (ImGui::Button((const char*)ICON_FA_TRASH))
				remove_selected_layer();
			ImGui::SameLine();
			if (ImGui::Button((const char*)ICON_FA_ARROW_UP))
				move_selected_layer_up();
			ImGui::SameLine();
			if (ImGui::Button((const char*)ICON_FA_ARROW_DOWN))
				move_selected_layer_down();
		}

		if (mCurrent_editor)
			mCurrent_editor->on_side_bar();

		if (ImGui::CollapsingHeader("Detailed Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginChild("PropertiesPane", { 0, 0 }, true);
			if (ImGui::TreeNode("Scene"))
			{
				ImGui::TextUnformatted("No properties available");
				ImGui::TreePop();
			}

			if (mSelected_layer && ImGui::TreeNode("Selected Layer"))
			{
				ImGui::TextUnformatted("No properties available");
				ImGui::TreePop();
			}

			if (mCurrent_editor)
				mCurrent_editor->on_detailed_properties();
			
			ImGui::EndChild();
		}

		ImGui::EndChild(); // SidePanelSettings

		ImGui::SameLine();
		ImGui::VerticalSplitter("SidePanelSettingsSplitter", &side_panel_width);


		ImGui::SameLine();
		ImGui::BeginGroup();

		if (mCurrent_editor)
			mCurrent_editor->on_tool_bar();

		ImGui::BeginChild("Scene", { 0, -ImGui::GetTextLineHeightWithSpacing() }, true, /*ImGuiWindowFlags_NoScrollbar |*/ ImGuiWindowFlags_MenuBar);
		
		const ImVec2 screen_mouse_pos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();
		show_viewport();


		ImGui::EndChild(); // Scene

		ImGui::TextUnformatted(fmt::format("Coord: {}", mRenderer.screen_to_world(screen_mouse_pos).to_string()).c_str());

		ImGui::EndGroup();
	}

	void update_asset_data()
	{
		auto resource = get_asset()->get_resource<core::scene_resource>();
		resource->update_data(mScene);
	}

	void move_selected_layer_up()
	{
		if (mSelected_layer)
		{
			auto& layers = mScene.get_layer_container();
			if (layers.size() <= 1)
				return;
			for (auto i = std::next(layers.begin()); i != layers.end(); i++)
			{
				if (mSelected_layer == &*i)
				{
					layers.splice(std::prev(i), layers, i);
					mark_asset_modified();
					return;
				}
			}
		}
	}

	void move_selected_layer_down()
	{
		if (mSelected_layer)
		{
			auto& layers = mScene.get_layer_container();
			if (layers.size() <= 1)
				return;
			for (auto i = layers.begin(); i != std::prev(layers.end()); i++)
			{
				if (mSelected_layer == &*i)
				{
					layers.splice(std::next(i, 2), layers, i);
					mark_asset_modified();
					return;
				}
			}
		}
	}

	void remove_selected_layer()
	{
		if (mSelected_layer)
		{
			mScene.remove_layer(*mSelected_layer);
			mSelected_layer = nullptr;
			mCurrent_editor = nullptr;
			mark_asset_modified();
		}
	}

	virtual void on_save() override
	{
		update_asset_data();
	}

	void show_viewport()
	{
		core::engine& engine = get_context().get_engine();

		static bool show_center_point = true;
		static bool is_grid_enabled = true;
		static graphics::color grid_color{ 1, 1, 1, 0.2f };

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Object", NULL, false, false);
				ImGui::Checkbox("Center Point", &show_center_point);
				ImGui::Separator();
				ImGui::MenuItem("Grid", NULL, false, false);
				ImGui::Checkbox("Enable Grid", &is_grid_enabled);
				ImGui::ColorEdit4("Grid Color", grid_color.components().data(), ImGuiColorEditFlags_NoInputs);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		
		ImVec2 region_size = ImGui::GetContentRegionAvail();
		float width = region_size.x - ImGui::GetStyle().ScrollbarSize;
		float height = region_size.y - ImGui::GetStyle().ScrollbarSize;

		ImGui::FillWithFramebuffer(mViewport_framebuffer);

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGui::ImageButton(mViewport_framebuffer, ImVec2(width, height));

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		// Middle mouse to drag
		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDragging(2, 0.1f))
			{
				mViewport_camera.move_focus(mRenderer.get_render_view_scale() * -math::vec2(ImGui::GetIO().MouseDelta));
			}

			if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
			{
				mViewport_camera.zoom(-ImGui::GetIO().MouseWheel * 0.25f);
			}
			else
			{
				math::vec2 scroll_amount = -math::vec2{ ImGui::GetIO().MouseWheelH, ImGui::GetIO().MouseWheel } * std::pow(2, mViewport_camera.get_zoom());
				mViewport_camera.move_focus(scroll_amount);
			}
		}

		// Update scene aabb info.
		update_scene_aabb();
		mRenderer.set_framebuffer(mViewport_framebuffer);
		//mViewport_camera.set_focus(math::clamp_components(mViewport_camera.get_focus(), mScene_aabb.min, mScene_aabb.max));
		mRenderer.set_view(mViewport_camera.get_view());

		draw_ruler(cursor);

		viewport_scrollbars();

		visual_editor::begin("_SceneEditor", { cursor.x, cursor.y }, mRenderer.get_render_view().min, 1.f / mRenderer.get_render_view_scale());
		{
			core::asset::ptr dropdropasset = asset_drag_drop_target("gameobject", get_asset_manager());
			if (dropdropasset && mSelected_layer)
			{
				auto obj = new_instance(dropdropasset);

				// Set the transform to the position it was dropped at.
				if (auto transform = obj.get_component<math::transform>())
					transform->position = visual_editor::get_mouse_position();
			}

			if (is_grid_enabled)
				visual_editor::draw_grid(grid_color, get_grid_step());

			if (mCurrent_editor)
				mCurrent_editor->on_overlay();
		}
		visual_editor::end();

		// Render the little layer previews.
		mLayer_previews.render_previews(mScene, mRenderer, { 20, 20 });

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		// Render all the layers.
		mRenderer.render_scene(mScene, engine.get_graphics());
	}

	// Generate a new instance from an object asset.
	core::object new_instance(const core::asset::ptr& pAsset)
	{
		assert(pAsset);
		core::engine& engine = get_context().get_engine();
		auto object_resource = pAsset->get_resource<core::object_resource>();

		// Generate the object
		core::object obj = mSelected_layer->add_object();
		object_resource->generate_object(obj, get_asset_manager());
		obj.set_asset(pAsset);

		mark_asset_modified();

		return obj;
	}

	scene_editor_mode* get_layer_editor(core::layer& pLayer)
	{
		auto& editor = mLayer_editors[&pLayer];
		if (!editor)
		{
			if (core::is_tilemap_layer(pLayer))
				editor = std::make_unique<scene_editor_tilemap_mode>(*this, pLayer);
			else
				editor = std::make_unique<scene_editor_instance_mode>(*this, pLayer);
		}
		return editor.get();
	}

	void select_layer(core::layer& pLayer)
	{
		mCurrent_editor = get_layer_editor(pLayer);
		mSelected_layer = &pLayer;
	}

	void update_scene_aabb()
	{
		mScene_aabb.min = mScene_aabb.max = { 0, 0 };
		for (auto& i : mScene)
			if (auto editor = get_layer_editor(i))
				mScene_aabb.merge(editor->get_aabb());
	}

private:
	float get_grid_step() const
	{
		// TODO: Doesn't work entirely as intended but its close enough.
		//   It SHOULD actually use the size of viewport to determine the step but
		//   I'm too stupid to figure it out.
		return std::pow(4, math::floor((mViewport_camera.get_zoom() + 1) * 2) / 2);
	}

	void draw_ruler(ImVec2 cursor)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();

		const ImU32 color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

		const float step = get_grid_step();
		const math::vec2 start{ (mRenderer.get_render_view().min / step).floor() * step };
		const math::vec2 end{ (mRenderer.get_render_view().max / step).floor() * step };
		for (float x = start.x; x <= end.x; x += step)
		{
			ImVec2 textpos = cursor + ImVec2(mRenderer.world_to_screen(math::vec2(x, 0)).x, 0);
			dl->AddText(textpos, color, fmt::to_string(x).c_str());
		}

		for (float y = start.y; y <= end.y; y += step)
		{
			ImVec2 textpos = cursor + ImVec2(0, mRenderer.world_to_screen(math::vec2(0, y)).y);
			dl->AddText(textpos, color, fmt::to_string(y).c_str());
		}
	}

	void viewport_scrollbars()
	{
		const math::vec2 scene_size = mScene_aabb.max - mScene_aabb.min;
		const math::vec2 view_size = mRenderer.get_render_view().max - mRenderer.get_render_view().min;
		if (scene_size.is_zero() || view_size.is_zero())
			return;
		const math::vec2 scrollbar_length = mViewport_camera.get_size() * (view_size / scene_size);
		math::vec2 scroll = mViewport_camera.get_focus() - mScene_aabb.min;

		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		const ImGuiStyle& style = g.Style;

		// Scrollbar sizes need to be set manually for things to work properly.
		window->ScrollbarSizes = { style.ScrollbarSize, style.ScrollbarSize };

		{
			const ImGuiID id = ImGui::GetWindowScrollbarID(window, ImGuiAxis_X);
			ImGui::KeepAliveID(id);

			// Calculate scrollbar bounding box
			ImRect bb = ImGui::GetWindowScrollbarRect(window, ImGuiAxis_X);
			ImGui::ScrollbarEx(bb, id, ImGuiAxis_X, &scroll.x, scrollbar_length.x, scene_size.x + scrollbar_length.x, 0);
		}
		{
			const ImGuiID id = ImGui::GetWindowScrollbarID(window, ImGuiAxis_Y);
			ImGui::KeepAliveID(id);

			// Calculate scrollbar bounding box
			ImRect bb = ImGui::GetWindowScrollbarRect(window, ImGuiAxis_Y);
			ImGui::ScrollbarEx(bb, id, ImGuiAxis_Y, &scroll.y, scrollbar_length.y, scene_size.y + scrollbar_length.y, 0);

			mViewport_camera.set_focus(scroll + mScene_aabb.min);
		}
	}

private:
	scene_editor_mode* mCurrent_editor = nullptr;
	std::map<core::layer*, std::unique_ptr<scene_editor_mode>> mLayer_editors;
	core::layer* mSelected_layer = nullptr;
	core::scene mScene;

	core::scene_resource* mScene_resource = nullptr;

	graphics::renderer mRenderer;
	graphics::framebuffer::ptr mViewport_framebuffer;
	graphics::camera mViewport_camera;
	math::aabb mScene_aabb;

	layer_previews mLayer_previews;

	on_game_run_callback mOn_game_run_callback;
};

static bool texture_asset_input(core::asset::ptr& pAsset, context& pContext, const core::asset_manager& pAsset_manager)
{
	std::string inputtext = pAsset ? pAsset_manager.get_asset_path(pAsset).string().c_str() : "None";
	ImGui::BeginGroup();
	if (pAsset)
	{
		const bool is_selected = ImGui::ImageButton(pAsset, { 100, 100 });
		ImGui::QuickToolTip("Open Sprite Editor");
		if (is_selected)
			pContext.open_editor(pAsset);
		ImGui::SameLine();
		ImGui::BeginGroup();
		auto res = pAsset->get_resource<graphics::sprite>();
		ImGui::Text("Size: %i, %i", res->get_frame_width(), res->get_frame_height());
		ImGui::EndGroup();
		ImGui::InputText("Sprite", &inputtext, ImGuiInputTextFlags_ReadOnly);
	}
	else
	{
		ImGui::Button("Drop a texture asset here", ImVec2(-1, 100));
	}
	ImGui::EndGroup();

	bool asset_dropped = false;

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("spriteAsset"))
		{
			const util::uuid& id = *(const util::uuid*)payload->Data;
			pAsset = pAsset_manager.get_asset(id);
			asset_dropped = true;
		}
		ImGui::EndDragDropTarget();
	}
	return asset_dropped;
}
class game_viewport
{
public:
	game_viewport(core::engine& pEngine) :
		mEngine(&pEngine)
	{}

	void open_scene(core::resource_handle<core::scene_resource> pScene)
	{
		if (!pScene)
		{
			log::error("No asset to generate scene with.");
			return;
		}
		log::info("Resetting script vm...");
		mEngine->get_script_engine().cleanup();

		log::info("Opening Scene in Player Viewport...");
		mEngine->get_physics().clear_all();
		mEngine->get_scene().clear();
		pScene->generate_scene(mEngine->get_scene(), mEngine->get_asset_manager());
		mIs_loaded = true;
		mScene = pScene;
		log::info("Ready");
	}

	void restart()
	{
		log::info("Restarting Scene...");
		open_scene(mScene);
	}

	void init_viewport()
	{
		log::info("Initializing Player Viewport Graphics....");

		auto& g = mEngine->get_graphics();
		mViewport_framebuffer = g.get_graphics_backend()->create_framebuffer();
		mViewport_framebuffer->resize(500, 500);
		mRenderer.set_framebuffer(mViewport_framebuffer);
		mRenderer.set_pixel_size(0.01f);
	}

	void register_input()
	{
		log::info("Registering Player Viewport Input...");

		auto& state = mEngine->get_script_engine().state;
		state["key_left"] = GLFW_KEY_LEFT;
		state["key_right"] = GLFW_KEY_RIGHT;
		state["key_up"] = GLFW_KEY_UP;
		state["key_down"] = GLFW_KEY_DOWN;
		state["key_a"] = GLFW_KEY_A;
		state["key_b"] = GLFW_KEY_B;
		state["key_c"] = GLFW_KEY_C;
		state["key_d"] = GLFW_KEY_D;
		state["key_e"] = GLFW_KEY_E;
		state["key_f"] = GLFW_KEY_F;
		state["key_g"] = GLFW_KEY_G;
		state["key_h"] = GLFW_KEY_H;
		state["key_i"] = GLFW_KEY_I;
		state["key_j"] = GLFW_KEY_J;
		state["key_k"] = GLFW_KEY_K;
		state["key_l"] = GLFW_KEY_L;
		state["key_m"] = GLFW_KEY_M;
		state["key_n"] = GLFW_KEY_N;
		state["key_o"] = GLFW_KEY_O;
		state["key_p"] = GLFW_KEY_P;
		state["key_q"] = GLFW_KEY_Q;
		state["key_r"] = GLFW_KEY_R;
		state["key_s"] = GLFW_KEY_S;
		state["key_t"] = GLFW_KEY_T;
		state["key_u"] = GLFW_KEY_U;
		state["key_v"] = GLFW_KEY_V;
		state["key_w"] = GLFW_KEY_W;
		state["key_x"] = GLFW_KEY_X;
		state["key_y"] = GLFW_KEY_Y;
		state["key_z"] = GLFW_KEY_Z;
		state["key_space"] = GLFW_KEY_SPACE;
		state["key_tab"] = GLFW_KEY_TAB;
		state["key_lshift"] = GLFW_KEY_LEFT_SHIFT;
		state["key_rshift"] = GLFW_KEY_RIGHT_SHIFT;
		state["key_lctrl"] = GLFW_KEY_LEFT_CONTROL;
		state["key_rctrl"] = GLFW_KEY_RIGHT_CONTROL;
		state["key_lalt"] = GLFW_KEY_LEFT_ALT;
		state["key_ralt"] = GLFW_KEY_RIGHT_ALT;

		state["mouse_button_left"] = GLFW_MOUSE_BUTTON_LEFT;
		state["mouse_button_middle"] = GLFW_MOUSE_BUTTON_MIDDLE;
		state["mouse_button_right"] = GLFW_MOUSE_BUTTON_RIGHT;

		state["is_key_down"] = [this](int pKey) -> bool
		{
			return mCan_take_input && ImGui::IsKeyDown(pKey);
		};
		state["is_key_pressed"] = [this](int pKey) -> bool
		{
			return mCan_take_input && ImGui::IsKeyPressed(pKey, false);
		};
		state["is_key_released"] = [this](int pKey) -> bool
		{
			return mCan_take_input && ImGui::IsKeyPressed(pKey, false);
		};

		state["is_mouse_pressed"] = [this](int pButton) -> bool
		{
			return mCan_take_input && ImGui::IsMouseClicked(pButton, false);
		};
		state["is_mouse_down"] = [this](int pButton) -> bool
		{
			return mCan_take_input && ImGui::IsMouseDown(pButton);
		};
		state["is_mouse_released"] = [this](int pButton) -> bool
		{
			return mCan_take_input && ImGui::IsMouseReleased(pButton);
		};
		
		state["mouse_world_delta"] = math::vec2(0, 0);
		state["mouse_world_position"] = math::vec2(0, 0);
		state["mouse_screen_delta"] = math::vec2(0, 0);
		state["mouse_screen_position"] = math::vec2(0, 0);
	}

	void step()
	{

		if (mIs_running)
		{
			mEngine->step();
		}

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		// Render all the layers.
		mRenderer.set_view(mEngine->get_default_camera().get_view());
		mRenderer.render_scene(mEngine->get_scene(), mEngine->get_graphics());
	}

	void on_gui()
	{
		if (ImGui::Begin("Game##GameViewport"))
		{
			// Play/pause button.
			if (ImGui::Button((const char*)(mIs_running ? ICON_FA_PAUSE u8" Pause" : ICON_FA_PLAY u8"Play")))
			{
				mIs_running = !mIs_running;
			}

			// Restart button.
			ImGui::SameLine();
			if (ImGui::Button((const char*)(ICON_FA_UNDO u8" Restart")))
			{
				restart();
			}
			ImGui::SameLine();
			if (auto new_asset = asset_selector("Scene", "scene", mEngine->get_asset_manager(), mScene.get_asset()))
			{
				open_scene(new_asset);
			}

			mCan_take_input = ImGui::IsWindowFocused();
			if (mIs_loaded)
			{
				ImGui::BeginChild("viewport", {0, 0}, true, ImGuiWindowFlags_MenuBar);
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Engine"))
					{
						if (ImGui::MenuItem("Reset erroneous objects"))
							reset_erroneous_objects();
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("View"))
					{
						bool collision_debug = mEngine->get_physics().get_collision_debug_enabled();
						if (ImGui::Checkbox("Show Collision", &collision_debug))
							mEngine->get_physics().set_collision_debug_enabled(collision_debug);
						bool raycast_debug = mEngine->get_physics().get_raycast_debug_enabled();
						if (ImGui::Checkbox("Show Raycasts", &raycast_debug))
							mEngine->get_physics().set_raycast_debug_enabled(raycast_debug);
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}
				mCan_take_input |= ImGui::IsWindowFocused();
				auto cursor = ImGui::GetCursorScreenPos();
				ImGui::Image(mViewport_framebuffer, ImGui::FillWithFramebuffer(mViewport_framebuffer));

				visual_editor::begin("_SceneEditor", cursor, mRenderer.get_render_view().min, 1.f / mRenderer.get_render_view_scale());
				
				mEngine->get_physics().imgui_debug(1.f / 60.f);

				visual_editor::end();

				// Update mouse inputs.
				auto& state = mEngine->get_script_engine().state;
				auto input = state.create_named_table("input");
				input["mouse_world_delta"] = math::vec2(ImGui::GetIO().MouseDelta) * mRenderer.get_render_view_scale();
				input["mouse_world_position"] = mRenderer.screen_to_world(ImGui::GetMousePos() - cursor);
				input["mouse_screen_delta"] = math::vec2(ImGui::GetIO().MouseDelta);
				input["mouse_screen_position"] = ImGui::GetMousePos() - cursor;

				ImGui::EndChild();

				step();
			}
			else
			{
				ImGui::TextUnformatted("No game to display... yet");
			}
		}
		ImGui::End();
	}

	void reset_erroneous_objects()
	{
		auto object_list = mEngine->get_script_engine().get_erroneous_objects();
		for (auto& id : object_list)
		{
			core::object obj = mEngine->get_scene().get_object(id);
			log::info("Reinitializing script for \"{}\" [{} id:{}]", obj.get_name(), obj.get_asset()->get_name(), obj.get_id());
			mEngine->get_script_engine().reset_object(obj);
		}
	}

private:
	bool mIs_running = false;
	bool mIs_loaded = false;
	bool mCan_take_input = false;
	graphics::framebuffer::ptr mViewport_framebuffer;
	graphics::renderer mRenderer;

	core::resource_handle<core::scene_resource> mScene;
	core::engine* mEngine;
};

class log_window
{
public:
	void on_gui()
	{
		const std::size_t log_limit = 256;

		if (ImGui::Begin("Log", NULL, ImGuiWindowFlags_HorizontalScrollbar))
		{
			auto log = log::get_log();

			// If the window is scrolled to the bottom, keep it at the bottom.
			// To prevent it from locking the users mousewheel input, it will only lock the scroll
			// when the log actually changes.
			bool lock_scroll_at_bottom = ImGui::GetScrollY() == ImGui::GetScrollMaxY() && last_log_size != log.size();

			ImGui::Columns(2, 0, false);
			ImGui::SetColumnWidth(0, 70);

			// Limit the amount of items that can be shown in log.
			// This makes it more convenient to scroll and there is less to draw.
			for (auto& i : log.last(log_limit))
			{
				switch (i.severity_level)
				{
				case log::level::info:
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
					ImGui::TextUnformatted("Info");
					break;
				case log::level::debug:
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 1, 1, 1 }); // Cyan-ish
					ImGui::TextUnformatted("Debug");
					break;
				case log::level::warning:
					ImGui::PushStyleColor(ImGuiCol_Text, { 1, 1, 0.5f, 1 }); // Yellow-ish
					ImGui::TextUnformatted("Warning");
					break;
				case log::level::error:
					ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.5f, 0.5f, 1 }); // Red
					ImGui::TextUnformatted("Error");
					break;
				}
				ImGui::NextColumn();

				// The actual message. Has the same color as the message type.
				ImGui::TextUnformatted(i.string.c_str());
				ImGui::PopStyleColor();

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			if (lock_scroll_at_bottom)
				ImGui::SetScrollHere();
			last_log_size = log.size();
		}
		ImGui::End();
	}

private:
	size_t last_log_size = 0;
};

class application
{
public:
	application()
	{
		mContext.register_editor<sprite_editor>("sprite");
		//mContext.register_editor<object_editor>("gameobject", mInspectors);
		mContext.register_editor<script_editor>("script");
		mContext.register_editor<scene_editor>("scene", mOn_game_run);
		mContext.register_editor<object_editor>("gameobject");
		mContext.register_editor<tileset_editor>("tileset");

		mOn_game_run.connect([this](const core::asset::ptr& pAsset) {
			mGame_viewport.open_scene(pAsset);
		});
	}

	int run()
	{
		init_graphics();
		init_imgui();
		mGame_viewport.init_viewport();
		mGame_viewport.register_input();

		load_project("project");

		mainloop();
		return 0;
	}

private:
	void load_editor_configuration()
	{
		log::info("Loading editor configuration.");
		std::ifstream file("./editor/config.json");
		if (!file)
		{
			log::info("Editor configuration does not exist. A new one will be created.");
			return;
		}
		
		try {
			json config = json::parse(file);
			mEngine.get_graphics().get_window_backend()->deserialize_settings(config);
		}
		catch (json::exception& e)
		{
			log::error("Json error: {}", e.what());
			log::error("Failed to parse editor configuration. A new one will be created.");
		}
	}

	void save_editor_configuration()
	{
		json config;
		mEngine.get_graphics().get_window_backend()->serialize_settings(config);

		std::ofstream configoutput("./editor/config.json");
		if (!configoutput)
		{
			log::error("Failed to open \"./editor/config.json\" for saving editor configuration.");
			return;
		}
		configoutput << config.dump(2);
		log::info("Editor configuration saved.");
	}

	void init_graphics()
	{
		auto& g = mEngine.get_graphics();
		// Only glfw and opengl is supported for editing
		g.initialize(graphics::window_backend_type::glfw, graphics::backend_type::opengl);

		load_editor_configuration();

		// Store the glfw backend for initializing imgui's glfw backend
		mGLFW_backend = std::dynamic_pointer_cast<graphics::glfw_window_backend>(g.get_window_backend());
	}

	void mainloop()
	{
		while (!glfwWindowShouldClose(mGLFW_backend->get_window()))
		{
			new_frame();

			mContext.set_default_dock_id(ImGui::GetID("_MainDockId"));
			main_viewport_dock(mContext.get_default_dock_id());

			if (ImGui::BeginMainMenuBar())
			{
				// Just an aesthetic
				ImGui::TextColored({ 0.5, 0.5, 0.5, 1 }, "WGE");

				if (ImGui::BeginMenu((const char*)(ICON_FA_HOME u8" Project")))
				{
					ImGui::MenuItem(" New");
					ImGui::MenuItem(" Open");
					ImGui::Separator();
					//ImGui::MenuItem("Save", "Ctrl+S", false, mContext.are_there_modified_assets());
					if (ImGui::MenuItem("Save All", "Ctrl+Alt+S", false, mContext.are_there_modified_assets() && mEngine.is_loaded()))
						mContext.save_all_assets();
					ImGui::Separator();
					if (ImGui::BeginMenu("Recent"))
					{
						ImGui::EndMenu();
					}
					ImGui::Separator();
					ImGui::MenuItem("Exit", "Alt+F4");
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Windows"))
				{
					if (ImGui::MenuItem("Close all editors"))
					{
						mContext.close_all_editors();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}

			mLog_window.on_gui();
			show_settings();
			mAsset_manager_window.on_gui();
			mContext.show_editor_guis();
			mGame_viewport.on_gui();
			mImport_window.on_gui(mContext.get_engine().get_asset_manager(), mImport_manager);
			show_debugger();

			end_frame();
		}
		shutdown();
	}

	void shutdown()
	{
		// Save configuration
		save_editor_configuration();

		// Cleanup ImGui
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void new_frame()
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void end_frame()
	{
		ImGui::Render();

		int display_w, display_h;
		glfwMakeContextCurrent(mGLFW_backend->get_window());
		glfwGetFramebufferSize(mGLFW_backend->get_window(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.15f, 0.15f, 0.15f, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		mGLFW_backend->refresh();
	}

	void init_imgui()
	{
		// Setup imgui, enable docking and dpi support.
		ImGui::CreateContext();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Viewports are disabled for now. Might make it an optional setting later.
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		ImGui::GetIO().ConfigDockingWithShift = true;
		ImGui_ImplGlfw_InitForOpenGL(mGLFW_backend->get_window(), true);
		ImGui_ImplOpenGL3_Init("#version 150");

		auto fonts = ImGui::GetIO().Fonts;

		// This will be our default font. It is quite a bit better than imguis builtin one.
		/*if (fonts->AddFontFromFileTTF("./editor/Roboto-Regular.ttf", 18) == NULL)
		{
			log::error("Could not load RobotoMono-Regular font. Using default.");
			fonts->AddFontDefault();
		}*/
		fonts->AddFontDefault();

		// Setup the icon font so we can fancy things up.
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 14;
		icons_config.GlyphMaxAdvanceX = 14;
		if (fonts->AddFontFromFileTTF("./editor/forkawesome-webfont.ttf", 13, &icons_config, icons_ranges) == NULL)
		{
			log::error("Could not load forkawesome-webfont.ttf font.");
			fonts->AddFontDefault();
		}

		// Used in the code editor.
		if (fonts->AddFontFromFileTTF("./editor/RobotoMono-Regular.ttf", 13) == NULL)
		{
			log::error("Could not load RobotoMono-Regular font. Using default.");
			fonts->AddFontDefault();
		}


		// Theme
		/*ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
		colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.77f, 0.77f, 0.77f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.64f, 0.64f, 0.64f, 0.40f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.77f, 0.77f, 0.77f, 0.40f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.64f, 0.64f, 0.64f, 0.40f);
		colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.54f, 0.54f, 0.54f, 0.78f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.431f, 0.431f, 0.431f, 0.500f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.54f, 0.54f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.38f, 0.38f, 0.38f, 0.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);*
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);*/
	}

	void create_project(const filesystem::path& pPath)
	{

	}

	void load_project(const filesystem::path& pPath)
	{
		mEngine.close_game();
		mEngine.load_game(pPath);

		mImport_manager.load_import_folder(pPath / "imports");
	}

private:
	void show_settings()
	{
		if (ImGui::Begin((const char*)(ICON_FA_COG u8" Settings")))
		{
			/*ImGui::BeginTabBar("SettingsTabBar");
			if (ImGui::BeginTabItem("Style"))
			{
				ImGui::ShowStyleEditor();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();*/
		}
		ImGui::End();
	}

	void show_debugger()
	{
		static std::set<std::string_view> builin_function_filter = {
			"set_position", "get_position", "created", "this", "this_layer",
			"destroy", "animation_play", "animation_stop", "set_sprite", "move",
			"is_valid" };
		auto& script_engine = mEngine.get_script_engine();
		if (ImGui::Begin("Debug Object Inspector"))
		{
			for (auto& layer : mEngine.get_scene())
			{
				if (ImGui::TreeNode(fmt::format("layer: {}", layer.get_name()).c_str()))
				{
					for (auto [id, env, info]: layer.each<scripting::event_state_component, core::object_info>())
					{
						if (env.environment.valid() &&
							ImGui::TreeNode(fmt::format("Name: {} Id: {}", info.name, id).c_str()))
						{
							ImGui::Columns(2);
							for (auto& var : env.environment)
							{
								std::string var_name = var.first.as<std::string>();
								if (builin_function_filter.find(var_name) != builin_function_filter.end())
									continue;
								std::string type_name = sol::type_name(script_engine.state, var.second.get_type());
								if (type_name == "function")
									continue;
								//std::string value = var.second.as<std::string>();
								var.second.push();
								std::size_t len = 0;
								const char* value_ptr = luaL_tolstring(script_engine.state, -1, &len);
								std::string value(value_ptr, len);
								lua_pop(script_engine.state, 1);
								var.second.pop();

								ImGui::PushID(&info);
								ImGui::Selectable(fmt::format("{} [{}]", var_name, type_name).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
								ImGui::PopID();
								ImGui::NextColumn();
								ImGui::TextUnformatted(value.c_str());
								ImGui::NextColumn();
							}
							ImGui::TreePop();
							ImGui::Columns(1);
						}
					}
					ImGui::TreePop();
				}
			}
		}

		ImGui::End();

		if (ImGui::Begin("Debug Error List"))
		{
			if (ImGui::TreeNodeEx("Compile Errors", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (script_engine.get_compile_errors().empty())
					ImGui::TextUnformatted("No compile errors");
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.5f, 0.5f, 1 });
				for (auto&& [id, error_info] : script_engine.get_compile_errors())
				{
					auto asset = mContext.get_engine().get_asset_manager().get_asset(id);
					auto path = mContext.get_engine().get_asset_manager().get_asset_path(asset);
					ImGui::Selectable(fmt::format("{} : {} : {}", path.string(), error_info.line, error_info.message).c_str());
				}
				ImGui::PopStyleColor();
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Runtime Errors", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (script_engine.get_runtime_errors().empty())
					ImGui::TextUnformatted("No runtime errors");
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.8f, 0.5f, 1 });
				for (auto&& [id, error_info] : script_engine.get_runtime_errors())
				{
					auto asset = mContext.get_engine().get_asset_manager().get_asset(error_info.asset_id);
					auto path = mContext.get_engine().get_asset_manager().get_asset_path(asset);
					auto obj = mEngine.get_scene().get_object(id);
					ImGui::Selectable(fmt::format("{} [{} id:{}] : {} : {} : {}",
						obj.get_name(), obj.get_asset()->get_name(), obj.get_id(),
						path.string(), error_info.line, error_info.message).c_str());
				}
				ImGui::PopStyleColor();
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Frozen objects", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (script_engine.get_erroneous_objects().empty())
					ImGui::TextUnformatted("No frozen objects");
				ImGui::PushStyleColor(ImGuiCol_Text, { 0.5f, 0.5f, 0.8f, 1 });
				for (auto id : script_engine.get_erroneous_objects())
				{
					auto obj = mEngine.get_scene().get_object(id);
					if (ImGui::Selectable(fmt::format("{} [{} id:{}]",
						obj.get_name(), obj.get_asset()->get_name(), obj.get_id()).c_str()))
					{
						script_engine.reset_object(obj);
						break;
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Reinitialize the object");
						ImGui::EndTooltip();
					}
				}
				ImGui::PopStyleColor();
				ImGui::TreePop();
			}
		}
		ImGui::End();

	}

private:
	// ImGui needs access to some glfw specific objects
	graphics::glfw_window_backend::ptr mGLFW_backend;

	util::signal<void(const core::asset::ptr&)> mOn_game_run;

	context mContext;

	log_window mLog_window;

	// Reference the game engine for convenience.
	core::engine& mEngine{ mContext.get_engine() };
	import_manager mImport_manager;
	import_window mImport_window;
	asset_manager_window mAsset_manager_window{ mContext, mEngine.get_asset_manager() };
	game_viewport mGame_viewport{ mEngine };

	bool mUpdate{ false };
};

int main(int argc, char ** argv)
{
	log::open_file("./editor/log.txt");
	application app;
	return app.run();
}

} // namespace wge::editor
