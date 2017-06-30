#ifndef RPG_ENTITY_MANAGER_HPP
#define RPG_ENTITY_MANAGER_HPP

#include <engine/renderer.hpp>
#include <rpg/entity.hpp>
#include <rpg/sprite_entity.hpp>
#include <rpg/character_entity.hpp>
#include <rpg/player_character.hpp>

namespace rpg {

class text_entity :
	public entity
{
public:
	text_entity();

	int draw(engine::renderer & pR);

	virtual type get_type() const
	{
		return type::text;
	}

	engine::formatted_text_node mText;
};

class dialog_text_entity :
	public text_entity
{
public:
	dialog_text_entity();

	void clear();

	int draw(engine::renderer& pR);

	bool is_revealing();
	void reveal(const std::string& pText, bool pAppend);
	void skip_reveal();

	void set_interval(float pMilliseconds);

	// Returns whether or not the text has revealed a
	// new letter in this frame
	bool has_revealed_character();

	void set_wordwrap(size_t pLength);
	void set_max_lines(size_t pLines);

private:
	void adjust_text();

	size_t mWord_wrap;
	size_t mMax_lines;
	bool        mNew_character;
	bool        mRevealing;
	size_t      mCount;
	engine::text_format mFull_text;
	engine::counter_clock mTimer;

	void do_reveal();
};

class entity_manager :
	public engine::render_proxy
{
public:
	entity_manager();

	void clean();

	void load_script_interface(script_system& pScript);
	void set_resource_manager(engine::resource_manager& pResource_manager);

	void set_root_node(engine::node& pNode);

private:

	template<typename T>
	T* create_entity()
	{
		if (mEntities.size() >= 1024)
		{
			logger::error("Reached upper limit of characters.");
			return nullptr;
		}
		auto new_entity = new T();
		mEntities.push_back(std::unique_ptr<entity>(dynamic_cast<entity*>(new_entity)));
		mRoot_node->add_child(*new_entity);
		get_renderer()->add_object(*new_entity);
		return new_entity;
	}

	engine::node* mRoot_node;

	void register_entity_type(script_system& pScript);

	engine::resource_manager*  mResource_manager;
	script_system* mScript_system;

	std::vector<std::unique_ptr<entity>> mEntities;

	bool check_entity(entity_reference& e);

	// General
	void             script_remove_entity(entity_reference& e);
	void             script_set_position(entity_reference& e, const engine::fvector& pos);
	engine::fvector  script_get_position(entity_reference& e);
	void             script_set_depth_direct(entity_reference& e, float pDepth);
	void             script_set_depth(entity_reference& e, float pDepth);
	void             script_set_depth_fixed(entity_reference& e, bool pFixed);
	void             script_set_anchor(entity_reference& e, int pAnchor);
	void             script_set_visible(entity_reference& e, bool pIs_visible);
	void             script_add_child(entity_reference& e1, entity_reference& e2);
	void             script_set_parent(entity_reference& e1, entity_reference& e2);
	void             script_detach_children(entity_reference& e);
	void             script_detach_parent(entity_reference& e);
	void             script_make_gui(entity_reference& e, float pOffset);
	void             script_set_z(entity_reference& e, float pZ);
	float            script_get_z(entity_reference& e);
	bool             script_is_character(entity_reference& e);
	void             script_set_parallax(entity_reference& e, float pParallax);

	// Text-based
	entity_reference script_add_text();
	void             script_set_text(entity_reference& e, const std::string& pText);
	void             script_set_font(entity_reference& e, const std::string& pName);

	// Sprite-based
	entity_reference script_add_entity(const std::string& tex);
	entity_reference script_add_entity_atlas(const std::string& tex, const std::string& atlas);
	engine::fvector  script_get_size(entity_reference& e);
	void             script_start_animation(entity_reference& e);
	void             script_stop_animation(entity_reference& e);
	void             script_pause_animation(entity_reference& e);
	void             script_set_animation_speed(entity_reference& e, float pSpeed);
	float            script_get_animation_speed(entity_reference& e);
	bool             script_is_animation_playing(entity_reference& e);
	void             script_set_atlas(entity_reference& e, const std::string& name);
	void             script_set_rotation(entity_reference& e, float pRotation);
	float            script_get_rotation(entity_reference& e);
	void             script_set_color(entity_reference& e, int r, int g, int b, int a);
	void             script_set_texture(entity_reference & e, const std::string & name);
	void             script_set_scale(entity_reference& e, const engine::fvector& pScale);
	engine::fvector  script_get_scale(entity_reference& e);

	// Character
	entity_reference script_add_character(const std::string& tex);
	void             script_set_direction(entity_reference& e, int dir);
	int              script_get_direction(entity_reference& e);
	void             script_set_cycle(entity_reference& e, const std::string& name);

	// Dialog Text
	entity_reference script_add_dialog_text();
	void             script_reveal(entity_reference& e, const std::string& pText, bool pAppend);
	bool             script_is_revealing(entity_reference& e);
	void             script_skip_reveal(entity_reference& e);
	void             script_set_interval(entity_reference& e, float pMilli);
	bool             script_has_displayed_new_character(entity_reference& e);
	void             script_dialog_set_wordwrap(entity_reference& e, unsigned int pLength);
	void             script_dialog_set_max_lines(entity_reference& e, unsigned int pLines);

	friend class scene_visualizer;
};

}

#endif // !RPG_ENTITY_MANAGER_HPP