#ifndef RPG_TILEMAP_DISPLAY_HPP
#define RPG_TILEMAP_DISPLAY_HPP

#include <engine/renderer.hpp>

#include <list>

namespace rpg {

class tilemap_manipulator;

class tilemap_display :
	public engine::render_object
{
public:
	void set_texture(std::shared_ptr<engine::texture> pTexture);
	std::shared_ptr<engine::texture> get_texture();

	int draw(engine::renderer &pR);
	void clear();

	void highlight_layer(size_t pLayer, engine::color pHighlight, engine::color pOthers);
	void remove_highlight();

	void update(tilemap_manipulator& pTile_manipulator);

	void set_layer_visible(size_t pIndex, bool pIs_visible);
	bool is_layer_visible(size_t pIndex);

private:
	bool add_tile(engine::fvector pPosition, const std::string& pAtlas, int pLayer, int pRotation);

	void update_animations();

	std::shared_ptr<engine::texture> mTexture;

	class animated_tile
	{
	public:
		engine::vertex_reference mRef;

		animated_tile() : mAnimation(nullptr) {}

		void set_animation(std::shared_ptr<const engine::animation> pAnimation);
		void update_animation();

	private:
		engine::timer mTimer;
		engine::frame_t mFrame;
		std::shared_ptr<const engine::animation> mAnimation;
	};

	std::vector<animated_tile> mAnimated_tiles;
	std::list<engine::vertex_batch> mLayers;


};

}

#endif