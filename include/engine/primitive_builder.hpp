#pragma once

#include <memory>
#include <vector>

#include <engine/renderer.hpp> // TODO: replace with something like "render_client.hpp" when its finally created
#include <engine/node.hpp>
#include <engine/rect.hpp>
#include <engine/vector.hpp>
#include <engine/color.hpp>
#include <engine/texture.hpp>

#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>

namespace engine
{

class renderer;

class primitive_builder :
	public render_object
{
public:
	struct handle
	{
	private:
		std::size_t entry_index;
		std::size_t vertex_index;
		friend class primitive_builder;
	};

	void push_offset(fvector pOffset);
	void push_scale(fvector pScale);
	void push_rotation(float pRotation);
	void push_node(const node& pNode);
	void pop_transform(std::size_t pCount = 1);
	void pop_node();

	void add_rectangle(frect pRect, color pFill, color pOutline = { 0, 0, 0, 0 });
	void add_line(fvector pP0, fvector pP1, color pColor);
	void add_poly(std::vector<fvector> pPoints, color pFill);
	void add_poly_ouline(std::vector<fvector> pPoints, color pOutline);
	handle add_quad_texture(std::shared_ptr<texture> pTexture, frect pRect, frect pTexture_rect, color pTint = { 1, 1, 1, 1 });
	handle add_quad_texture(std::shared_ptr<texture> pTexture, fvector pPosition, frect pTexture_rect, color pTint = { 1, 1, 1, 1 }); // Uses the texture rect size for the displayed size
	handle add_quad_texture(std::shared_ptr<texture> pTexture, fvector pPosition, frect pTexture_rect, color pTint, int mRotate_corners); // Rotate 90*mRotate_corners degrees while maintaining the topleft corner position

	void change_texture_rect(handle pHandle, frect pTexture_rect);

	void clear();

	int draw(renderer& pR) override;
	int draw_and_clear(renderer& pR);

private:
	struct transform
	{
		transform();
		fvector offset;
		fvector scale;
		float rotation;
	};
	std::vector<transform> mTransform_stack;
	fvector apply_transform_stack(fvector pPoint) const;
	void apply_transform_stack(std::vector<sf::Vertex>& pVertices) const;

	struct entry
	{
		std::vector<sf::Vertex> vertices;
		sf::PrimitiveType type;
		std::shared_ptr<texture> tex;
	};
	std::vector<entry> mEntries;
	static entry generate_poly_entry(std::vector<fvector> pPoints, color pColor);
};

}