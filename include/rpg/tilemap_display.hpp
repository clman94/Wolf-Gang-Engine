#ifndef RPG_TILEMAP_DISPLAY_HPP
#define RPG_TILEMAP_DISPLAY_HPP

#include <engine/renderer.hpp>

namespace rpg {

class tilemap_display :
	public engine::render_object
{
public:
	void set_texture(std::shared_ptr<engine::texture> pTexture);
	void set_tile(engine::fvector pPosition, const std::string& pAtlas, int pLayer, int pRotation);

	int draw(engine::renderer &pR);
	void update_animations();
	void clean();

	void set_color(engine::color pColor);

	void highlight_layer(int pLayer, engine::color pHighlight, engine::color pOthers);
	void remove_highlight();

private:
	std::shared_ptr<engine::texture> mTexture;

	class tile
	{
	public:
		engine::vertex_reference mRef;

		tile() : mAnimation(nullptr) {}

		void set_animation(std::shared_ptr<const engine::animation> pAnimation);
		void update_animation();

	private:
		int mRotation;
		engine::timer mTimer;
		engine::frame_t mFrame;
		std::shared_ptr<const engine::animation> mAnimation;
	};

	struct layer
	{
		engine::vertex_batch vertices;
		std::map<engine::fvector, tile> tiles;
	};

	std::vector<tile*> mAnimated_tiles;

	std::map<int, layer> mLayers;
};

}

#endif