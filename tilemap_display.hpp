#ifndef RPG_TILEMAP_DISPLAY_HPP
#define RPG_TILEMAP_DISPLAY_HPP

#include "renderer.hpp"

namespace rpg {

class tilemap_display :
	public engine::render_client,
	public engine::node
{
public:
	void set_texture(engine::texture& pTexture);
	void set_tile(engine::fvector pPosition, const std::string& pAtlas, int pLayer, int pRotation);

	int draw(engine::renderer &pR);

	void update_animations();

	void clear();

private:
	engine::texture *mTexture;

	class tile
	{
	public:
		engine::vertex_reference mRef;

		tile() : mAnimation(nullptr) {}

		void set_animation(const engine::animation* pAnimation);
		void update_animation();

	private:
		int rotation;
		engine::timer mTimer;
		engine::frame_t mFrame;
		const engine::animation* mAnimation;
	};

	struct layer
	{
		engine::vertex_batch vertices;
		std::map<engine::fvector, tile> tiles;
	};

	std::map<int, layer> mLayers;
};

}

#endif