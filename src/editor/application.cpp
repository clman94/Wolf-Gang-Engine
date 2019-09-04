
#include <wge/editor/application.hpp>

#include <wge/logging/log.hpp>
#include <wge/filesystem/path.hpp>
#include <wge/core/scene.hpp>
#include <wge/core/transform_component.hpp>
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
#include <Wge/util/ipair.hpp>

#include "editor.hpp"
#include "history.hpp"
#include "context.hpp"
#include "component_inspector.hpp"
#include "imgui_editor_tools.hpp"
#include "imgui_ext.hpp"
#include "text_editor.hpp"
#include "icon_codepoints.hpp"

#include <imgui/imgui_internal.h>

// GL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <wge/graphics/glfw_backend.hpp>

#include <variant>
#include <functional>
#include <future>
#include <unordered_map>
#include <array>

namespace wge::core
{

class object_resource :
	public resource
{
public:
	enum class event_type
	{
		create,
		update,
		count,
	};
	static constexpr std::array<const char*, 2> event_display_name = {
		ICON_FA_PENCIL " Create",
		ICON_FA_STEP_FORWARD " Update"
	};
	static constexpr std::array<const char*, 2> event_typenames = { "create", "update" };

	// Asset id for the default sprite.
	util::uuid display_sprite;

	// Lists the ids for the scripts assets for each event type.
	std::array<util::uuid, static_cast<unsigned>(event_type::count)> events;

public:
	virtual void save() override {}

	void generate_object(core::game_object& pObj, const asset_manager& pAsset_mgr)
	{
		pObj.add_component<core::transform_component>();

		if (display_sprite.is_valid())
		{
			auto sprite = pObj.add_component<graphics::sprite_component>();
			sprite->set_texture(pAsset_mgr.get_asset(display_sprite));
		}

		if (!events.empty())
		{
			pObj.add_component<scripting::event_state_component>();

			for (const auto& [type, asset_id] : util::ipair{ events })
			{
				scripting::event_component* comp = nullptr;
				switch (static_cast<event_type>(type))
				{
				case event_type::create:
					comp = pObj.add_component<scripting::event_components::on_create>();
					break;
				case event_type::update:
					comp = pObj.add_component<scripting::event_components::on_update>();
					break;
				}
				assert(comp);
				if (auto asset = pAsset_mgr.get_asset(asset_id))
					comp->source_script = asset;
				else
					log::error() << "Could not load event script; Invalid id" << log::endm;
			}
		}
	}

	virtual json serialize_data() const override
	{
		json j;
		json& jevents = j["events"];
		for (const auto& [type, asset_id] : util::ipair{ events })
		{
			json jevent;
			const char* event_name = event_typenames[type];
			jevent["type"] = event_name;
			jevent["id"] = asset_id;
			jevents.push_back(jevent);
		}
		j["sprite"] = display_sprite;

		return j;
	}

	virtual void deserialize_data(const json& pJson) override
	{
		const json& jevents = pJson["events"];
		for (const auto& i : jevents)
		{
			// Find the index for this event name.
			for (auto[index, name] : util::ipair{ event_typenames })
			{
				if (name == i["type"])
				{
					// Assign the id to that index.
					events[index] = i["id"];
					break;
				}
			}
		}

		display_sprite = pJson["sprite"];
	}
};

class scene_resource :
	public resource
{
public:
	struct object_instance
	{
		// Unique name of instance.
		std::string name;
		math::transform transform;
		util::uuid id;
		util::uuid asset_id;
	};
	 
	std::vector<object_instance> instances;

public:
	virtual json serialize_data() const override
	{
		json j;
		for (const object_instance& i : instances)
		{
			json j_inst;
			j_inst["name"] = i.name;
			j_inst["transform"] = i.transform;
			j_inst["id"] = i.id;
			j_inst["asset_id"] = i.asset_id;
			j["instances"].push_back(j_inst);
		}
		return j;
	}

	virtual void deserialize_data(const json& pJson) override
	{
		for (const json& i : pJson["instances"])
		{
			object_instance inst;
			inst.name = i["name"];
			inst.transform = i["transform"];
			inst.id = i["id"];
			inst.asset_id = i["asset_id"];
			instances.emplace_back(std::move(inst));
		}
	}

	static core::game_object generate_instance(core::layer& pLayer, const core::asset_manager& pAsset_mgr, const object_instance& pData)
	{
		auto asset = pAsset_mgr.get_asset(pData.asset_id);
		auto object_resource = asset->get_resource<core::object_resource>();

		// Generate the object
		core::game_object obj = pLayer.add_object(pData.name);
		obj.set_instance_id(pData.id);
		object_resource->generate_object(obj, pAsset_mgr);
		obj.set_asset(asset);

		if (auto transform = obj.get_component<core::transform_component>())
			transform->set_transform(pData.transform);

		return obj;
	}

	void generate_layer(core::layer& pLayer, const core::asset_manager& pAsset_mgr) const
	{
		auto renderer = pLayer.add_system<graphics::renderer>();
		renderer->set_pixel_size(0.01f);
		pLayer.add_system<scripting::script_system>();
		pLayer.add_system<physics::physics_world>();
		for (auto& i : instances)
			generate_instance(pLayer, pAsset_mgr, i);
	}

	void generate_scene(core::scene& pScene, const core::asset_manager& pAsset_mgr) const
	{
		auto layer = pScene.add_layer("Game");
		generate_layer(*layer, pAsset_mgr);
	}
};

class texture_importer :
	public importer
{
public:
	virtual asset::ptr import(asset_manager& pAsset_mgr, const filesystem::path& pSystem_path) override
	{
		auto tex = std::make_shared<asset>();
		tex->set_name(pSystem_path.stem());
		tex->set_type("texture");
		
		// Create a new subdirectory for the asset's storage.
		auto directory = pAsset_mgr.get_root_directory() / pAsset_mgr.generate_asset_directory_name(tex);
		system_fs::create_directory(directory);
		tex->set_file_path(directory / (tex->get_name() + ".wga"));

		// Copy the texture file.
		auto dest = directory / (tex->get_name() + ".png");
		try {
			system_fs::copy_file(pSystem_path, dest);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			log::error() << "Failed to copy texture from " << pSystem_path.string() << log::endm;
			log::error() << "Failed with exception " << std::quoted(e.what()) << log::endm;
			return{};
		}

		// Create the new resource
		auto res = pAsset_mgr.create_resource("texture");
		res->load(directory, tex->get_name());
		tex->set_resource(std::move(res));

		// Save the configuration
		tex->save();

		pAsset_mgr.add_asset(tex);

		return tex;
	}
};

} // namespace wge::core

namespace wge::editor
{

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

inline void GLAPIENTRY opengl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam);

class object_editor :
	public asset_editor
{
public:
	object_editor(context& pContext, const core::asset::ptr& pAsset, component_inspector& pInspectors) :
		asset_editor(pContext, pAsset),
		mInspectors(&pInspectors)
	{
		mSandbox = core::layer::create(pContext.get_engine().get_factory());
		mObject = mSandbox->add_object();
		mObject.deserialize(
			pContext.get_engine().get_asset_manager(),
			pAsset->get_metadata());
	}

	virtual void on_gui() override
	{
		assert(mInspectors);
		std::string name = mObject.get_name();
		if (ImGui::InputText("Name", &name))
			mObject.set_name(name);
		if (ImGui::IsItemDeactivatedAfterEdit())
			mark_asset_modified();

		ImGui::Separator();

		for (std::size_t i = 0; i < mObject.get_component_count(); i++)
		{
			core::component* comp = mObject.get_component_at(i);
			ImGui::PushID(comp);

			bool enabled = comp->is_enabled();
			if (ImGui::Checkbox("##Enabled", &enabled))
				comp->set_enabled(enabled);
			if (ImGui::IsItemDeactivatedAfterEdit())
				mark_asset_modified();

			ImGui::SameLine();

			if (ImGui::TreeNodeEx(comp->get_component_name().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				mInspectors->on_gui(comp);
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		ImGui::Separator();

		if (ImGui::BeginCombo("##Add Component", "Add Component", ImGuiComboFlags_NoArrowButton))
		{
			if (ImGui::Selectable("Transform 2D"))
				mObject.add_component<core::transform_component>();
			ImGui::DescriptiveToolTip("Give your object space",
				"This transform represents the position, rotation, and scale of this object in 2d space. Use with sprite and other 2d graphics for the best effect.");

			if (ImGui::Selectable("Physics"))
				mObject.add_component<physics::physics_component>();
			ImGui::DescriptiveToolTip("Let's get physical",
				"This component allows this object to interact with physics in your world.");

			if (ImGui::Selectable("Box Collider"))
				mObject.add_component<physics::box_collider_component>();
			ImGui::DescriptiveToolTip("Give this object a collidable box",
				"Works best with a physics component.");

			if (ImGui::Selectable("Sprite"))
				mObject.add_component<graphics::sprite_component>();
			ImGui::QuickToolTip("Give this object a sprite to render");

			if (ImGui::Selectable("Event State"))
				mObject.add_component<scripting::event_state_component>();
			ImGui::DescriptiveToolTip("Let's get interactive!",
				"Start adding events to turn this stale component pasta into something spectacularly interactive!");
			
			if (ImGui::BeginMenu("Events"))
			{
				if (ImGui::Selectable("Create"))
					mObject.add_component<scripting::event_components::on_create>();
				ImGui::QuickToolTip("Do someting when this object is instantiated");

				if (ImGui::Selectable("Update"))
					mObject.add_component<scripting::event_components::on_update>();
				ImGui::QuickToolTip("Do something every frame");

				ImGui::EndMenu();
			}

			ImGui::EndCombo();
		}
	}

private:
	core::game_object mObject;
	core::layer::uptr mSandbox;
	component_inspector* mInspectors;
};

class sprite_editor :
	public asset_editor
{
public:
	sprite_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset)
	{}

	void on_gui()
	{
		ImGui::BeginChild("AtlasInfo", ImVec2(mAtlas_info_width, 0));
		atlas_info_pane();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::VerticalSplitter("AtlasInfoSplitter", &mAtlas_info_width);

		ImGui::SameLine();
		ImGui::BeginChild("SpriteEditorViewport", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		auto texture = get_asset()->get_resource<graphics::texture>();

		float* zoom = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("_Zoom"), 0);

		const ImVec2 last_cursor = ImGui::GetCursorPos();
		ImGui::BeginGroup();

		const float scale = std::powf(2, *zoom);
		const ImVec2 image_size = texture->get_size() * scale;

		// Top and left padding
		ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
		ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
		ImGui::SameLine();

		// Store the cursor so we can position things on top of the image
		const ImVec2 image_position = ImGui::GetCursorScreenPos();

		// A checker board will help us "see" the alpha channel of the image
		ImGui::DrawAlphaCheckerBoard(image_position, image_size);

		ImGui::Image(get_asset(), image_size);

		// Right and bottom padding
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2, image_size.y));
		ImGui::Dummy(ImVec2(image_size.x + ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
		ImGui::EndGroup();

		// Draw grid
		if (*zoom > 2)
		{
			ImGui::DrawGridLines(image_position,
				ImVec2(image_position.x + image_size.x, image_position.y + image_size.y),
				{ 0, 1, 1, 0.2f }, scale);
		}

		// Overlap with an invisible button to recieve input
		ImGui::SetCursorPos(last_cursor);
		ImGui::InvisibleButton("_Input", ImVec2(image_size.x + ImGui::GetWindowWidth(), image_size.y + ImGui::GetWindowHeight()));

		visual_editor::begin("_SomeEditor", { image_position.x, image_position.y }, { 0, 0 }, { scale, scale });

		// Draw the rectangles for the frames
		for (const auto& i : texture->get_raw_atlas())
			visual_editor::draw_rect(i.frame_rect, { 0, 1, 1, 0.5f });

		const bool was_dragging = visual_editor::is_dragging();

		// Get the pointer to the selected animation
		graphics::animation* selected_animation = texture->get_animation(mSelected_animation_id);

		// Modify selected
		if (selected_animation)
		{
			visual_editor::begin_snap({ 1, 1 });

			// Edit the selection
			visual_editor::box_edit box_edit(selected_animation->frame_rect);
			box_edit.resize(visual_editor::edit_type::rect);
			box_edit.drag(visual_editor::edit_type::rect);
			selected_animation->frame_rect = box_edit.get_rect();

			// Limit the minimum size to +1 pixel so the user isn't using 0 or negitive numbers
			selected_animation->frame_rect.size = math::max(selected_animation->frame_rect.size, math::vec2(1, 1));

			// Notify a change in the asset
			if (box_edit.is_dragging() && ImGui::IsMouseReleased(0))
				mark_asset_modified();

			visual_editor::end_snap();
		}

		// Select a new one
		if (!was_dragging && ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
		{
			// Find all overlapping frames that the mouse is hovering
			std::vector<graphics::animation*> mOverlapping;
			for (auto& i : texture->get_raw_atlas())
				if (i.frame_rect.intersects(visual_editor::get_mouse_position()))
					mOverlapping.push_back(&i);

			if (!mOverlapping.empty())
			{
				// Check if the currently selected animation is being selected again
				// and cycle through the overlapping animations each click.
				auto iter = std::find(mOverlapping.begin(), mOverlapping.end(), selected_animation);
				if (iter == mOverlapping.end() || iter + 1 == mOverlapping.end())
					selected_animation = mOverlapping.front(); // Start/loop to front
				else
					selected_animation = *(iter + 1); // Next item
				mSelected_animation_id = selected_animation->id;
			}
		}

		visual_editor::end();

		if (ImGui::IsItemHovered())
		{
			// Zoom with ctrl and mousewheel
			if (ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0)
			{
				*zoom += ImGui::GetIO().MouseWheel;
				const float new_scale = std::powf(2, *zoom);
				const float ratio_changed = new_scale / scale;
				ImGui::SetScrollX(ImGui::GetScrollX() * ratio_changed);
				ImGui::SetScrollY(ImGui::GetScrollY() * ratio_changed);
			}

			// Hold middle mouse button to scroll
			ImGui::DragScroll(2);
		}

		ImGui::EndChild();
	}

	static void preview_image(const char* pStr_id, const graphics::texture::handle& pTexture, const math::vec2& pSize, const math::rect& pFrame_rect)
	{
		if (pSize.x <= 0 || pSize.y <= 0)
			return;

		// Scale the size of the image to preserve the aspect ratio but still fit in the
		// specified area.
		const float aspect_ratio = pFrame_rect.size.x / pFrame_rect.size.y;
		math::vec2 scaled_size =
		{
			math::min(pSize.y * aspect_ratio, pSize.x),
			math::min(pSize.x / aspect_ratio, pSize.y)
		};

		// Center the position
		const math::vec2 center_offset = pSize / 2 - scaled_size / 2;
		const math::vec2 pos = math::vec2(ImGui::GetCursorScreenPos()) + center_offset;

		// Draw the checkered background
		ImGui::DrawAlphaCheckerBoard(pos, scaled_size, 10);

		// Convert to UV coord
		math::aabb uv(pFrame_rect);
		uv.min /= pTexture->get_size();
		uv.max /= pTexture->get_size();

		// Draw the image
		const auto impl = std::dynamic_pointer_cast<graphics::opengl_texture_impl>(pTexture->get_implementation());
		auto dl = ImGui::GetWindowDrawList();
		dl->AddImage((void*)impl->get_gl_texture(), pos, pos + scaled_size, uv.min, uv.max);

		// Add an invisible button so we can interact with this image
		ImGui::InvisibleButton(pStr_id, pSize);
	}

	void atlas_info_pane()
	{
		const auto open_sprite_editor = []()
		{
			ImGui::Begin("Sprite Editor");
			ImGui::SetWindowFocus();
			ImGui::End();
		};

		graphics::texture::handle texture = get_asset();

		if (ImGui::CollapsingHeader("Atlas", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float atlas_list_height = 200;
			// Atlas list
			ImGui::BeginChild("_AtlasList", { 0, atlas_list_height }, true);
			ImGui::Columns(2, "_Previews", false);
			ImGui::SetColumnWidth(0, 75 + ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().ItemSpacing.x);
			for (auto& i : texture->get_raw_atlas())
			{
				if (ImGui::Selectable(("###" + i.name).c_str(), mSelected_animation_id == i.id, ImGuiSelectableFlags_SpanAllColumns, { 0, 75 }))
					mSelected_animation_id = i.id;

				// Double click will focus the sprite editor
				if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
					open_sprite_editor();
				ImGui::SameLine();

				preview_image("SmallPreviewImage", texture, { 75, 75 }, i.frame_rect);

				ImGui::NextColumn();

				// Entry name
				ImGui::Text(i.name.c_str());
				ImGui::NextColumn();
			}
			ImGui::Columns();
			ImGui::EndChild();

			ImGui::HorizontalSplitter("AtlasListSplitter", &atlas_list_height);

			if (ImGui::Button("Add"))
			{
				graphics::animation& animation = texture->get_raw_atlas().emplace_back();
				animation.frame_rect = math::rect({ 0, 0 }, texture->get_size());
				animation.name = make_unique_animation_name(texture, "NewEntry");
				animation.id = util::generate_uuid();
				mark_asset_modified();
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				auto iter = std::find_if(
					texture->get_raw_atlas().begin(),
					texture->get_raw_atlas().end(),
					[&](auto& i) { return i.id == mSelected_animation_id; });
				if (iter != texture->get_raw_atlas().end())
				{
					// Set the next animation after this one as selected
					if (iter + 1 != texture->get_raw_atlas().end())
						mSelected_animation_id = (iter + 1)->id;

					// Remove it
					texture->get_raw_atlas().erase(iter);
				}
				mark_asset_modified();
			}
		}

		if (graphics::animation* selected_animation = texture->get_animation(mSelected_animation_id))
		{
			ImGui::PushID("_AnimationSettings");

			if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static float preview_image_height = 200;
				ImGui::BeginChild("PreviewImageChild", { 0, preview_image_height }, true, ImGuiWindowFlags_NoInputs);
				preview_image("LargePreviewImage", texture, ImGui::GetWindowContentRegionSize(), selected_animation->frame_rect);
				ImGui::EndChild();
				ImGui::HorizontalSplitter("PreviewImageSplitter", &preview_image_height);
				preview_image_height = math::max(preview_image_height, 30.f);

				ImGui::Button("Play");
				static int a = 1;
				const std::string format = "%d/" + std::to_string(selected_animation->frames);
				ImGui::SliderInt("Frame", &a, 1, selected_animation->frames, format.c_str());
			}
			if (ImGui::CollapsingHeader("Basic", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::InputText("Name", &selected_animation->name);
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					std::string temp = std::move(selected_animation->name);
					selected_animation->name = make_unique_animation_name(texture, temp);
					mark_asset_modified();
				}
				ImGui::DragFloat2("Position", selected_animation->frame_rect.position.components); check_if_edited();
				ImGui::DragFloat2("Size", selected_animation->frame_rect.size.components); check_if_edited();
			}
			if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int frame_count = static_cast<int>(selected_animation->frames);
				if (ImGui::InputInt("Frame Count", &frame_count))
				{
					// Limit the minimun to 1
					selected_animation->frames = math::max<std::size_t>(static_cast<std::size_t>(frame_count), 1);
					mark_asset_modified();
				}
				ImGui::InputFloat("Interval", &selected_animation->interval, 0.01f, 0.1f, "%.3f Seconds"); check_if_edited();
			}
			ImGui::PopID();
		}
	}

private:
	static std::string make_unique_animation_name(const graphics::texture::handle& pTexture, const std::string& pName)
	{
		return util::create_unique_name(pName,
			pTexture->get_raw_atlas().begin(), pTexture->get_raw_atlas().end(),
			[](auto& i) -> const std::string& { return i.name; });
	}

	void check_if_edited()
	{
		if (ImGui::IsItemDeactivatedAfterEdit())
			mark_asset_modified();
	}

private:
	util::uuid mSelected_animation_id;
	float mAtlas_info_width = 200;
};

class script_editor :
	public asset_editor
{
public:
	script_editor(context& pContext, const core::asset::ptr& pAsset) :
		asset_editor(pContext, pAsset)
	{}

	virtual void on_gui() override
	{
		auto source = get_asset()->get_resource<scripting::script>();

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
		if (ImGui::CodeEditor("CodeEditor", source->source))
			mark_asset_modified();
		ImGui::PopFont();
	}
};

class asset_manager_window
{
public:
	asset_manager_window(context& pContext, core::asset_manager& pAsset_manager) :
		mContext(pContext),
		mAsset_manager(pAsset_manager)
	{}

	void on_gui()
	{
		if (ImGui::Begin(ICON_FA_FOLDER " Assets", 0, ImGuiWindowFlags_MenuBar))
		{
			static core::asset::ptr current_folder;
			const filesystem::path current_path = mAsset_manager.get_asset_path(current_folder);

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("New..."))
				{
					if (ImGui::MenuItem("Folder"))
					{
						mAsset_manager.create_folder(current_path / "new_folder");
					}

					if (ImGui::MenuItem("Object"))
					{
						auto asset = std::make_shared<core::asset>();
						asset->set_name("New_Object");
						asset->set_parent(current_folder);
						asset->set_type("gameobject");
						asset->set_resource(std::make_unique<core::object_resource>());
						mAsset_manager.store_asset(asset);
						asset->save();
						mAsset_manager.add_asset(asset);
					}

					if (ImGui::MenuItem("Scene"))
					{
						auto asset = std::make_shared<core::asset>();
						asset->set_name("New_Scene");
						asset->set_parent(current_folder);
						asset->set_type("scene");
						asset->set_resource(std::make_unique<core::scene_resource>());
						mAsset_manager.store_asset(asset);
						asset->save();
						mAsset_manager.add_asset(asset);
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Update Asset Directory"))
				{
					mAsset_manager.update_directory_structure();
				}
				ImGui::EndMenuBar();
			}

			static float directory_tree_width = 200;

			ImGui::BeginChild("DirectoryTree", { directory_tree_width, 0 }, true);
			show_asset_directory_tree(current_folder);
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::VerticalSplitter("DirectoryTreeSplitter", &directory_tree_width);

			ImGui::SameLine();
			ImGui::BeginGroup();

			ImGui::PushID("PathList");
			for (auto[i, segment] : util::ipair{ current_path })
			{
				const bool last_item = i == current_path.size() - 1;
				if (ImGui::Button(segment.c_str()))
				{
					// TODO: Clicking the last item will open a popup to select a different directory..?
					auto new_path = current_path;
					new_path.erase(new_path.begin() + i, new_path.end());
					current_folder = mAsset_manager.get_asset(new_path);
					break;
				}
				ImGui::SameLine();
			}
			ImGui::NewLine();

			ImGui::PopID();

			const math::vec2 file_preview_size = { 100, 100 };

			ImGui::BeginChild("FileList", { 0, 0 }, true);
			mAsset_manager.for_each_child(current_folder, [&](auto& i)
			{
				// Skip folders
				if (i->get_type() != "folder")
				{
					asset_tile(i, file_preview_size);

					// If there isn't any room left in this line, create a new one.
					if (ImGui::GetContentRegionAvailWidth() < file_preview_size.x)
						ImGui::NewLine();
				}
			});
			ImGui::EndChild();

			ImGui::EndGroup();
		}
		ImGui::End();
	}

private:
	void show_asset_directory_tree(core::asset::ptr& pCurrent_folder, const core::asset::ptr& pAsset = {})
	{
		const bool is_root = !pAsset;
		bool open = false;

		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		const char* name = is_root ? "Assets" : pAsset->get_name().c_str();
		if (pAsset && !mAsset_manager.has_subfolders(pAsset))
			ImGui::TreeNodeEx(name, flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		else
			open = ImGui::TreeNodeEx(name, flags);

		// Set the directory
		if (ImGui::IsItemClicked())
		{
			if (is_root)
				pCurrent_folder = core::asset::ptr{};
			else
				pCurrent_folder = pAsset;
		}

		if (open)
		{
			// Show subdirectories
			mAsset_manager.for_each_child(pAsset, [&](auto& i)
			{
				if (i->get_type() == "folder")
					show_asset_directory_tree(pCurrent_folder, i);
			});

			ImGui::TreePop();
		}
	}

	void asset_tile(const core::asset::ptr& pAsset, const math::vec2& pSize)
	{
		ImGui::PushID(&*pAsset);

		const bool is_asset_selected = mSelected_asset == pAsset;

		ImGui::BeginGroup();

		// Draw preview
		if (pAsset->get_type() == "texture")
			ImGui::ImageButton(pAsset,
				pSize - math::vec2(ImGui::GetStyle().FramePadding) * 2);
		else
			ImGui::Button("No preview", pSize);

		ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), pAsset->get_type().c_str());

		// Draw text
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + pSize.x);
		ImGui::Text(pAsset->get_name().c_str());
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();
		ImGui::SameLine();

		// Allow asset to be dragged
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload((pAsset->get_type() + "Asset").c_str(), &pAsset->get_id(), sizeof(util::uuid));
			ImGui::Text("Asset: %s", mAsset_manager.get_asset_path(pAsset).string().c_str());
			ImGui::EndDragDropSource();
		}

		// Select the asset when clicked
		if (ImGui::IsItemClicked())
			mSelected_asset = pAsset;
		if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
			mContext.open_editor(mSelected_asset);

		auto dl = ImGui::GetWindowDrawList();

		// Calculate item aabb that includes the item spacing
		math::vec2 item_min = math::vec2(ImGui::GetItemRectMin())
			- math::vec2(ImGui::GetStyle().ItemSpacing) / 2;
		math::vec2 item_max = math::vec2(ImGui::GetItemRectMax())
			+ math::vec2(ImGui::GetStyle().ItemSpacing) / 2;

		// Draw the background
		if (ImGui::IsItemHovered())
		{
			dl->AddRectFilled(item_min, item_max,
				ImGui::GetColorU32(ImGuiCol_ButtonHovered), ImGui::GetStyle().FrameRounding);
		}
		else if (is_asset_selected)
		{
			dl->AddRectFilled(item_min, item_max,
				ImGui::GetColorU32(ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding);
		}

		ImGui::PopID();
	}

private:
	context& mContext;
	core::asset_manager& mAsset_manager;
	core::asset::ptr mSelected_asset;
};

class scene_editor :
	public asset_editor
{
public:
	// TODO: Implement a better and more generic messaging method at some point.
	using on_game_run_callback = std::function<void(const core::asset::ptr&)>;

	scene_editor(context& pContext, const core::asset::ptr& pAsset, const on_game_run_callback& pRun_callback) noexcept :
		asset_editor(pContext, pAsset),
		mScene(pContext.get_engine().get_factory()),
		mOn_game_run_callback(pRun_callback)
	{
		// Create a framebuffer for the scene to be rendered to.
		auto& graphics = pContext.get_engine().get_graphics();
		mViewport_framebuffer = graphics.get_graphics_backend()->create_framebuffer();

		mScene_resource = get_asset()->get_resource<core::scene_resource>();

		// Generate the layer.
		auto layer = mScene.add_layer();
		mScene_resource->generate_layer(*layer, pContext.get_engine().get_asset_manager());
	}

	virtual void on_gui() override
	{
		ImGui::BeginChild("SidePanelSettings", ImVec2(200, 0));
		if (ImGui::Button("Run") && mOn_game_run_callback)
		{
			// Make sure the actual asset data is up to date before
			// we start the scene.
			update_asset_data();
			// Invoke the callback. I may consider using a better
			// messaging mechanism later.
			mOn_game_run_callback(get_asset());
		}
		ImGui::TextUnformatted("Layers");
		ImGui::BeginChild("Layers", ImVec2(0, 300), true);
		for (const auto& i : mScene.get_layer_container())
		{
			ImGui::PushID(i.get());

			if (ImGui::Selectable(i->get_name().c_str(), mSelected_layer == i.get()))
				mSelected_layer = i.get();
			ImGui::PopID();
		}
		ImGui::EndChild();

		ImGui::TextUnformatted("Instances");
		ImGui::BeginChild("Instances", ImVec2(0, 0), true);
		if (mSelected_layer)
		{
			for (std::size_t i = 0; i < mSelected_layer->get_object_count(); i++)
			{
				ImGui::PushID(i);
				core::game_object obj = mSelected_layer->get_object(i);
				if (ImGui::Selectable(obj.get_name().c_str(), obj.get_instance_id() == mSelected_object_id))
					mSelected_object_id = obj.get_instance_id();
				ImGui::PopID();
			}
		}
		ImGui::EndChild(); // Instances

		ImGui::EndChild(); // SidePanelSettings

		ImGui::SameLine();

		static float viewport_width = -100;
		ImGui::BeginChild("Scene", ImVec2(viewport_width, 0), true);

		show_viewport();

		ImGui::EndChild(); // Scene
		ImGui::SameLine();
		ImGui::VerticalSplitter("ViewportInspectorSplitter", &viewport_width);
		ImGui::SameLine();
		ImGui::BeginChild("InspectorSidePanel", ImVec2(-viewport_width, 0));
		if (mSelected_layer && mSelected_object_id.is_valid())
		{
			ImGui::TextUnformatted("Selected Object Settings");
			auto obj = mSelected_layer->get_object(mSelected_object_id);
			std::string name = obj.get_name();
			if (ImGui::InputText("Name", &name))
				obj.set_name(name);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				obj.set_name(scripting::make_valid_identifier(name));
				mark_asset_modified();
			}
		}
		ImGui::EndChild();
	}

	void update_asset_data()
	{
		auto resource = get_asset()->get_resource<core::scene_resource>();
		resource->instances.clear();

		auto layer = mScene.get_layer(0);
		for (std::size_t i = 0; i < layer->get_object_count(); i++)
		{
			core::game_object obj = layer->get_object(i);
			core::scene_resource::object_instance& inst = resource->instances.emplace_back();
			inst.asset_id = obj.get_asset()->get_id();
			inst.id = obj.get_instance_id();
			inst.name = obj.get_name();
			if (auto transform = obj.get_component<core::transform_component>())
				inst.transform = transform->get_transform();
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
		static graphics::color grid_color{ 1, 1, 1, 0.7f };
		static bool show_collision = false;

		/*if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Object", NULL, false, false);
				ImGui::Checkbox("Center Point", &show_center_point);
				ImGui::Separator();
				ImGui::MenuItem("Grid", NULL, false, false);
				ImGui::Checkbox("Enable Grid", &is_grid_enabled);
				ImGui::ColorEdit4("Grid Color", grid_color.components, ImGuiColorEditFlags_NoInputs);

				ImGui::Separator();
				ImGui::MenuItem("Physics", NULL, false, false);
				ImGui::Checkbox("Collision", &show_collision);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}*/

		float width = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2 - ImGui::GetStyle().ScrollbarSize;
		float height = ImGui::GetWindowHeight() - ImGui::GetCursorPos().y - ImGui::GetStyle().WindowPadding.y - ImGui::GetStyle().ScrollbarSize;
		float scroll_x_max = width * 2;
		float scroll_y_max = height * 2;

		if (mViewport_framebuffer->get_width() != width
			|| mViewport_framebuffer->get_height() != height)
			mViewport_framebuffer->resize(static_cast<int>(width), static_cast<int>(height));

		ImGui::BeginFixedScrollRegion({ width, height }, { scroll_x_max, scroll_y_max });

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
			ImGui::DragScroll(2, 1);

		mViewport_offset = (math::vec2(ImGui::GetScrollX(), ImGui::GetScrollY()) / mViewport_scale);

		visual_editor::begin("_SceneEditor", { cursor.x, cursor.y }, mViewport_offset, mViewport_scale);
		{
			if (ImGui::BeginDragDropTarget() && mSelected_layer)
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobjectAsset"))
				{
					// Retrieve the object asset from the payload.
					const util::uuid& id = *(const util::uuid*)payload->Data;
					auto obj = new_instance(id);

					// Set the transform to the position it was dropped at.
					if (auto transform = obj.get_component<core::transform_component>())
						transform->set_position(visual_editor::get_mouse_position());
				}
				ImGui::EndDragDropTarget();
			}

			if (is_grid_enabled)
				visual_editor::draw_grid(grid_color, 1);

			for (auto& layer : mScene.get_layer_container())
			{
				graphics::renderer* renderer = layer->get_system<graphics::renderer>();
				if (!renderer)
					continue;
				// Make sure we are working with the viewports framebuffer
				renderer->set_framebuffer(mViewport_framebuffer);

				if (show_collision)
				{
					auto& box_collider_container = layer->get_component_container<physics::box_collider_component>();
					for (auto& i : box_collider_container)
					{
						auto transform = layer->get_object(i.get_object_id()).get_component<core::transform_component>();
						if (!transform)
							continue;
						visual_editor::push_transform(transform->get_transform());
						math::transform box_transform;
						box_transform.position = i.get_offset();
						box_transform.rotation = i.get_rotation();
						visual_editor::push_transform(box_transform);
						visual_editor::draw_rect(math::rect({ 0, 0 }, i.get_size()), { 0, 1, 0, 0.8f });
						visual_editor::pop_transform(2);
					}
				}

				for (std::size_t i = 0; i < layer->get_object_count(); i++)
				{
					auto obj = layer->get_object(i);
					auto transform = obj.get_component<core::transform_component>();
					if (!transform)
						continue;

					const bool is_object_selected = obj.get_instance_id() == mSelected_object_id;

					math::aabb aabb;
					if (create_aabb_from_object(obj, aabb))
					{
						if (is_object_selected)
						{
							visual_editor::box_edit box_edit(aabb, transform->get_transform());
							box_edit.resize(visual_editor::edit_type::transform);
							box_edit.drag(visual_editor::edit_type::transform);
							transform->set_transform(box_edit.get_transform());
							if (box_edit.is_dragging())
							{
								mark_asset_modified();
							}
						}

						visual_editor::push_transform(transform->get_transform());
						if (aabb.intersect(visual_editor::get_mouse_position()))
						{
							if (ImGui::IsItemClicked())
								mSelected_object_id = obj.get_instance_id();
							// TODO: Show info about this object instance
							visual_editor::draw_rect(aabb, { 1, 1, 1, 1 });
						}
						visual_editor::pop_transform();
					}

					// Draw center point
					if (is_object_selected && show_center_point)
					{
						visual_editor::draw_circle(transform->get_position(), 5, { 1, 1, 1, 0.6f }, 3.f);
					}
				}
			}
		}
		visual_editor::end();

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		// Render all layers.
		mScene.for_each_system<graphics::renderer>([&](core::layer&, graphics::renderer& pRenderer)
		{
			pRenderer.set_framebuffer(mViewport_framebuffer);
			pRenderer.set_render_view_to_framebuffer(mViewport_offset, 1 / mViewport_scale);
			pRenderer.render(engine.get_graphics());
		});
		ImGui::EndFixedScrollRegion();
	}

	core::game_object new_instance(util::uuid pAsset)
	{
		core::engine& engine = get_context().get_engine();
		auto asset = engine.get_asset_manager().get_asset(pAsset);
		auto object_resource = asset->get_resource<core::object_resource>();

		// Generate the object
		core::game_object obj = mSelected_layer->add_object();
		object_resource->generate_object(obj, get_asset_manager());
		obj.set_asset(asset);

		mark_asset_modified();

		return obj;
	}

	core::game_object generate_instance(const core::scene_resource::object_instance& pData)
	{
		if (!mSelected_layer)
		{
			log::error() << "No selected layer to add object instance"; 
			return{};
		}

		core::engine& engine = get_context().get_engine();
		auto asset = engine.get_asset_manager().get_asset(pData.asset_id);
		auto object_resource = asset->get_resource<core::object_resource>();

		// Generate the object
		core::game_object obj = mSelected_layer->add_object();
		obj.set_instance_id(pData.id);
		obj.set_name(pData.name);
		object_resource->generate_object(obj, get_asset_manager());
		obj.set_asset(asset);

		if (auto transform = obj.get_component<core::transform_component>())
			transform->set_transform(pData.transform);

		return obj;
	}

	static bool create_aabb_from_object(core::game_object& pObj, math::aabb& pAABB)
	{
		bool has_aabb = false;
		for (std::size_t comp_idx = 0; comp_idx < pObj.get_component_count(); comp_idx++)
		{
			auto comp = pObj.get_component_at(comp_idx);
			if (comp->has_aabb())
			{
				if (!has_aabb)
				{
					pAABB = comp->get_local_aabb();
					has_aabb = true;
				}
				else
					pAABB.merge(comp->get_local_aabb());
			}
		}
		return has_aabb;
	}

private:
	core::layer* mSelected_layer = nullptr;
	util::uuid mSelected_object_id;
	core::scene mScene;

	core::scene_resource* mScene_resource = nullptr;

	graphics::framebuffer::ptr mViewport_framebuffer;
	math::vec2 mViewport_offset;
	math::vec2 mViewport_scale{ 100, 100 };

	on_game_run_callback mOn_game_run_callback;
};

class drop_import_handler
{
public:
	drop_import_handler(core::asset_manager& pAsset_manager) :
		mAsset_manager(&pAsset_manager)
	{}

	~drop_import_handler()
	{
		mCallback_connection.disconnect();
	}

	void register_callback_to(decltype(graphics::window_backend::on_file_drop)& pCallback)
	{
		mCallback_connection = pCallback.connect([&](int pCount, const char** pPaths)
		{
			for (int i = 0; i < pCount; i++)
			{
				filesystem::path path = pPaths[i];
				if (path.extension() == ".png")
					mPaths.push_back(std::move(path));
			}
		});
	}

	void on_gui()
	{
		if (!mPaths.empty())
			ImGui::OpenPopup("Import Assets");

		if (ImGui::BeginPopupModal("Import Assets", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::BeginChild("AssetsToImport", ImVec2(400, 300), true);

			for (auto& i : mPaths)
				ImGui::Selectable(i.string().c_str());

			ImGui::EndChild();
			if (ImGui::Button("Import"))
			{
				core::texture_importer importer;
				for (auto& i : mPaths)
				{
					importer.import(*mAsset_manager, i);
				}
				mPaths.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				mPaths.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

private:
	core::asset_manager* mAsset_manager;
	std::vector<filesystem::path> mPaths;
	util::connection mCallback_connection;
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
		auto res = pAsset->get_resource<graphics::texture>();
		ImGui::Text("Size: %i, %i", res->get_width(), res->get_height());
		ImGui::Text("Animations: %u", res->get_raw_atlas().size());
		ImGui::EndGroup();
		ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
	}
	else
	{
		ImGui::Button("Drop a texture asset here", ImVec2(-1, 100));
	}
	ImGui::EndGroup();

	bool asset_dropped = false;

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("textureAsset"))
		{
			const util::uuid& id = *(const util::uuid*)payload->Data;
			pAsset = pAsset_manager.get_asset(id);
			asset_dropped = true;
		}
		ImGui::EndDragDropTarget();
	}
	return asset_dropped;
}

class eventful_sprite_editor :
	public asset_editor
{
public:
	eventful_sprite_editor(context& pContext, const core::asset::ptr& pAsset) noexcept :
		asset_editor(pContext, pAsset)
	{}

	virtual void on_gui() override
	{
		mSCript_editor_dock_id = ImGui::GetID("EditorDock");

		ImGui::BeginChild("LeftPanel", ImVec2(300, 0));

		std::string mut_name = get_asset()->get_name();
		if (ImGui::InputText("Name", &mut_name))
		{
			get_asset()->set_name(mut_name);
			mark_asset_modified();
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			get_asset()->set_name(scripting::make_valid_identifier(get_asset()->get_name()));
		}

		auto generator = get_asset()->get_resource<core::object_resource>();

		display_sprite_input(generator);
		display_event_list(generator);

		ImGui::EndChild();

		ImGui::SameLine();

		ImGuiDockFamily dock_family(mSCript_editor_dock_id);
		// Event script editors are given a dedicated dockspace where they spawn. This helps
		// remove clutter windows popping up everywhere.
		ImGui::DockSpace(mSCript_editor_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_None, &dock_family);
	}

	virtual void on_close() override
	{
		auto generator = get_asset()->get_resource<core::object_resource>();
		for (auto& i : generator->events)
			if (i.is_valid())
				get_context().close_editor(i);
	}

private:
	void display_sprite_input(core::object_resource* pGenerator)
	{
		core::asset::ptr sprite = get_asset_manager().get_asset(pGenerator->display_sprite);
		if (texture_asset_input(sprite, get_context(), get_asset_manager()))
		{
			pGenerator->display_sprite = sprite->get_id();
			mark_asset_modified();
		}
	}

	void display_event_list(core::object_resource* pGenerator)
	{
		ImGui::Text("Events:");
		ImGui::BeginChild("Events", ImVec2(0, 0), true);
		for (auto[type, asset_id] : util::ipair{ pGenerator->events })
		{
			const char* event_name = pGenerator->event_display_name[type];
			const bool editor_already_open = get_context().is_editor_open_for(asset_id);
			if (ImGui::Selectable(
				event_name,
				editor_already_open, ImGuiSelectableFlags_AllowDoubleClick)
				&& !editor_already_open && ImGui::IsMouseDoubleClicked(0))
			{
				bool first_time = false;
				core::asset::ptr asset;
				if (asset_id.is_valid())
				{
					asset = get_asset_manager().get_asset(asset_id);
				}
				else
				{
					asset = create_event_script(pGenerator->event_typenames[type]);
					asset_id = asset->get_id();
					mark_asset_modified();
					first_time = true;
				}
				auto editor = get_context().open_editor(asset, mSCript_editor_dock_id);
				editor->set_dock_family_id(mSCript_editor_dock_id);
			}
		}
		ImGui::EndChild();
	}

private:
	core::asset::ptr create_event_script(const char* pName)
	{
		core::asset_manager& asset_manager = get_context().get_engine().get_asset_manager();

		auto asset = std::make_shared<core::asset>();
		asset->set_name(pName);
		asset->set_parent(get_asset());
		asset->set_type("script");
		asset->set_resource(asset_manager.create_resource("script"));
		asset_manager.store_asset(asset);

		auto script = asset->get_resource<scripting::script>();
		script->save(asset->get_file_path().parent(), pName);
		asset->save();

		asset_manager.add_asset(asset);

		return asset;
	}

	ImGuiID mSCript_editor_dock_id;
};

class game_viewport
{
public:
	game_viewport(core::engine& pEngine) :
		mEngine(&pEngine)
	{}

	void open_scene(const core::asset::ptr& pAsset)
	{
		mEngine->get_scene().clear();
		if (auto resource = pAsset->get_resource<core::scene_resource>())
		{
			resource->generate_scene(mEngine->get_scene(), mEngine->get_asset_manager());
			mIs_running = true;
			mIs_loaded = true;
		}
		else
		{
			log::error() << "Asset is not a scene." << log::endm;
		}
	}

	void init_viewport()
	{
		auto& g = mEngine->get_graphics();
		mViewport_framebuffer = g.get_graphics_backend()->create_framebuffer();
		mViewport_framebuffer->resize(500, 500);
	}

	void step()
	{
		auto& scene = mEngine->get_scene();

		if (mIs_running)
		{
			mEngine->step();
		}

		// Clear the framebuffer with black.
		mViewport_framebuffer->clear({ 0, 0, 0, 1 });

		mEngine->render_to(mViewport_framebuffer, math::vec2(0, 0), math::vec2(1.f, 1.f) * 100.f);
	}

	void on_gui()
	{
		if (ImGui::Begin("Game##GameViewport"))
		{
			if (mIs_loaded)
			{
				ImGui::Image(mViewport_framebuffer, ImVec2(500, 500));
				step();
			}
			else
			{
				ImGui::TextUnformatted("No game to display... yet");
			}
		}
		ImGui::End();
	}

private:
	bool mIs_running = false;
	bool mIs_loaded = false;
	graphics::framebuffer::ptr mViewport_framebuffer;

	core::engine* mEngine;
};

class application
{
public:
	application()
	{
		mContext.register_editor<sprite_editor>("texture");
		//mContext.register_editor<object_editor>("gameobject", mInspectors);
		mContext.register_editor<script_editor>("script");
		mContext.register_editor<scene_editor>("scene", mOn_game_run);
		mContext.register_editor<eventful_sprite_editor>("gameobject");
		mEngine.get_asset_manager().register_default_resource_factory<core::object_resource>("gameobject");
		mEngine.get_asset_manager().register_default_resource_factory<core::scene_resource>("scene");

		mOn_game_run.connect([this](const core::asset::ptr& pAsset) {
			mGame_viewport.open_scene(pAsset);
		});
	}

	int run()
	{
		init_graphics();
		init_imgui();
		mGame_viewport.init_viewport();

		load_project("project");

		mainloop();
		return 0;
	}

private:
	void init_graphics()
	{
		auto& g = mEngine.get_graphics();
		// Only glfw and opengl is supported for editing
		g.initialize(graphics::window_backend_type::glfw, graphics::backend_type::opengl);

		// Store the glfw backend for initializing imgui's glfw backend
		mGLFW_backend = std::dynamic_pointer_cast<graphics::glfw_window_backend>(g.get_window_backend());

		mDrop_import_handler.register_callback_to(mGLFW_backend->on_file_drop);
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

				if (ImGui::BeginMenu(ICON_FA_HOME " Project"))
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

			show_log();
			show_settings();
			//show_viewport();
			//show_objects();
			//show_inspector();
			mDrop_import_handler.on_gui();
			mAsset_manager_window.on_gui();
			mContext.show_editor_guis();
			mGame_viewport.on_gui();

			end_frame();
		}
		shutdown();
	}

	void shutdown()
	{
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
		if (fonts->AddFontFromFileTTF("./editor/Roboto-Regular.ttf", 18) == NULL)
		{
			log::error() << "Could not load RobotoMono-Regular font. Using default." << log::endm;
			fonts->AddFontDefault();
		}

		// Setup the icon font so we can fancy things up.
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 14;
		icons_config.GlyphMaxAdvanceX = 14;
		if (fonts->AddFontFromFileTTF("./editor/forkawesome-webfont.ttf", 18, &icons_config, icons_ranges) == NULL)
		{
			log::error() << "Could not load forkawesome-webfont.ttf font." << log::endm;
			fonts->AddFontDefault();
		}

		// Used in the code editor.
		if (fonts->AddFontFromFileTTF("./editor/RobotoMono-Regular.ttf", 18) == NULL)
		{
			log::error() << "Could not load RobotoMono-Regular font. Using default." << log::endm;
			fonts->AddFontDefault();
		}


		// Theme
		ImVec4* colors = ImGui::GetStyle().Colors;
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
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
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
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	void create_project(const filesystem::path& pPath)
	{

	}

	void load_project(const filesystem::path& pPath)
	{
		mEngine.close_game();
		mEngine.load_game(pPath);

		auto layer = mEngine.get_scene().add_layer();
		layer->set_name("Layer1");
		layer->add_system<graphics::renderer>();
		layer->add_system<scripting::script_system>();

		auto renderer = layer->get_system<graphics::renderer>();
		renderer->set_pixel_size(0.01f);

		auto obj = layer->add_object();
		obj.add_component<core::transform_component>();
		auto sprite = obj.add_component<graphics::sprite_component>();
		sprite->set_texture(mEngine.get_asset_manager().get_asset("mytex.png"));
		obj.add_component<scripting::event_state_component>();

		obj.add_component<scripting::event_components::on_create>();
		obj.add_component<scripting::event_components::on_update>();

		mEngine.get_script_engine().execute_global_scripts(mEngine.get_asset_manager());
	}

private:
	bool create_aabb_from_object(core::game_object& pObj, math::aabb& pAABB)
	{
		bool has_aabb = false;
		for (std::size_t comp_idx = 0; comp_idx < pObj.get_component_count(); comp_idx++)
		{
			auto comp = pObj.get_component_at(comp_idx);
			if (comp->has_aabb())
			{
				if (!has_aabb)
				{
					pAABB = comp->get_local_aabb();
					has_aabb = true;
				}
				else
					pAABB.merge(comp->get_local_aabb());
			}
		}
		return has_aabb;
	}

	void show_settings()
	{
		if (ImGui::Begin(ICON_FA_COG " Settings"))
		{
			ImGui::BeginTabBar("SettingsTabBar");
			if (ImGui::BeginTabItem("Style"))
			{
				ImGui::ShowStyleEditor();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}
	
	void show_log()
	{
		const std::size_t log_limit = 256;

		if (ImGui::Begin("Log", NULL, ImGuiWindowFlags_HorizontalScrollbar))
		{
			const auto& log = log::get_log();

			// Helps keep track of changes in the log
			static size_t last_log_size = 0;

			// If the window is scrolled to the bottom, keep it at the bottom.
			// To prevent it from locking the users mousewheel input, it will only lock the scroll
			// when the log actually changes.
			bool lock_scroll_at_bottom = ImGui::GetScrollY() == ImGui::GetScrollMaxY() && last_log_size != log.size();

			ImGui::Columns(2, 0, false);
			ImGui::SetColumnWidth(0, 60);

			// Limit the amount of items that can be shown in log.
			// This makes it more convenient to scroll and there is less to draw.
			const bool exceeds_limit = log.size() >= log_limit;
			const size_t start = exceeds_limit ? log.size() - log_limit : 0;
			for (size_t i = start; i < log.size(); i++)
			{
				switch (log[i].severity_level)
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
				ImGui::TextUnformatted(log[i].string.c_str());
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
	// ImGui needs access to some glfw specific objects
	graphics::glfw_window_backend::ptr mGLFW_backend;

	util::signal<void(const core::asset::ptr&)> mOn_game_run;

	context mContext;

	// Reference the game engine for convenience.
	core::engine& mEngine{ mContext.get_engine() };

	asset_manager_window mAsset_manager_window{ mContext, mEngine.get_asset_manager() };
	drop_import_handler mDrop_import_handler{ mEngine.get_asset_manager() };
	game_viewport mGame_viewport{ mEngine };

	component_inspector mInspectors;

	bool mUpdate{ false };
};

void GLAPIENTRY opengl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH: log::out << log::level::error; break;
	case GL_DEBUG_SEVERITY_MEDIUM: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_LOW: log::out << log::level::warning; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: log::out << log::level::info; break;
	default: log::out << log::level::unknown;
	}
	log::out << "OpenGL: " << message << log::endm;
}

int main(int argc, char ** argv)
{
	log::open_file("./editor/log.txt");
	application app;
	return app.run();
}

} // namespace wge::editor
