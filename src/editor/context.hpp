#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_stl.h>
#include "component_inspector.hpp"

namespace wge::editor
{

struct editor_context
{
	editor_context() :
		renderer(&game_context)
	{
	}

	void init()
	{
		core::component_factory& factory = game_context.get_component_factory();
		factory.add<core::transform_component>();
		factory.add<physics::physics_world_component>();
		factory.add<physics::physics_component>();
		factory.add<physics::box_collider_component>();
		factory.add<graphics::sprite_component>();

		asset_manager.add_loader("texture", texture_loader);
		asset_manager.add_loader("scene", config_loader);
		asset_manager.set_root_directory(".");
		asset_manager.load_assets();
		game_context.add_system(&asset_manager);

		renderer.initialize();
		renderer.set_pixel_size(0.01f);
		game_context.add_system(&renderer);

		setup_inspectors();
	}

	void setup_inspectors()
	{
		// Inspector for transform_component
		inspectors.add_inspector(core::transform_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto reset_context_menu = [](const char * pId)->bool
			{
				bool clicked = false;
				if (ImGui::BeginPopupContextItem(pId))
				{
					if (ImGui::Button("Reset"))
					{
						clicked = true;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				return clicked;
			};

			auto transform = dynamic_cast<core::transform_component*>(pComponent);
			math::vec2 position = transform->get_position();
			if (ImGui::DragFloat2("Position", position.components))
				transform->set_position(position);
			if (reset_context_menu("posreset"))
				transform->set_position(math::vec2(0, 0));

			float rotation = math::degrees(transform->get_rotation());
			if (ImGui::DragFloat("Rotation", &rotation, 1, 0, 0, "%.3f degrees"))
				transform->set_rotaton(math::degrees(rotation));
			if (reset_context_menu("rotreset"))
				transform->set_rotaton(0);

			math::vec2 scale = transform->get_scale();
			if (ImGui::DragFloat2("Scale", scale.components, 0.01f))
				transform->set_scale(scale);
			if (reset_context_menu("scalereset"))
				transform->set_scale(math::vec2(0, 0));
		});

		// Inspector for sprite_component
		inspectors.add_inspector(graphics::sprite_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto sprite = dynamic_cast<graphics::sprite_component*>(pComponent);
			math::vec2 offset = sprite->get_offset();
			if (ImGui::DragFloat2("Offset", offset.components))
				sprite->set_offset(offset);

			graphics::texture::ptr tex = sprite->get_texture();
			std::string inputtext = tex ? tex->get_path().string().c_str() : "None";
			ImGui::InputText("Texture", &inputtext, ImGuiInputTextFlags_ReadOnly);
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("textureAsset"))
				{
					core::asset_uid id = *(core::asset_uid*)payload->Data;
					sprite->set_texture(id);
				}
				ImGui::EndDragDropTarget();
			}
		});

		// Inspector for physics_world_component
		inspectors.add_inspector(physics::physics_world_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto physicsworld = dynamic_cast<physics::physics_world_component*>(pComponent);
			math::vec2 gravity = physicsworld->get_gravity();
			if (ImGui::DragFloat2("Gravity", gravity.components))
				physicsworld->set_gravity(gravity);
		});

		// Inspector for physics_box_collider
		inspectors.add_inspector(physics::box_collider_component::COMPONENT_ID,
			[](core::component* pComponent)
		{
			auto collider = dynamic_cast<physics::box_collider_component*>(pComponent);

			math::vec2 offset = collider->get_offset();
			if (ImGui::DragFloat2("Offset", offset.components))
				collider->set_offset(offset);

			math::vec2 size = collider->get_size();
			if (ImGui::DragFloat2("Size", size.components))
				collider->set_size(size);

			float rotation = math::degrees(collider->get_rotation());
			if (ImGui::DragFloat("Rotation", &rotation))
				collider->set_rotation(math::degrees(rotation));
		});
	}

	core::context game_context;
	filesystem::path game_path;

	core::config_asset_loader config_loader;
	graphics::texture_asset_loader texture_loader;
	core::asset_manager asset_manager;

	graphics::renderer renderer;

	editor_component_inspector inspectors;
};

}
